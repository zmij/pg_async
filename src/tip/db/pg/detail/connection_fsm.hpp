/*
 * connection_fsm.hpp
 *
 *  Created on: Jul 30, 2015
 *      Author: zmij
 */

#ifndef LIB_PG_ASYNC_SRC_TIP_DB_PG_DETAIL_BASIC_CONNECTION_NEW_HPP_
#define LIB_PG_ASYNC_SRC_TIP_DB_PG_DETAIL_BASIC_CONNECTION_NEW_HPP_

#include <boost/noncopyable.hpp>
#include <boost/bind.hpp>
#include <map>
#include <stack>
#include <set>
#include <memory>

#include <boost/msm/back/state_machine.hpp>
#include <boost/msm/front/state_machine_def.hpp>
#include <boost/msm/front/functor_row.hpp>
#include <boost/msm/front/euml/operator.hpp>

#include <tip/db/pg/asio_config.hpp>
#include <tip/db/pg/common.hpp>
#include <tip/db/pg/error.hpp>
#include <tip/db/pg/transaction.hpp>
#include <tip/db/pg/resultset.hpp>

#include <tip/db/pg/detail/basic_connection.hpp>
#include <tip/db/pg/detail/protocol.hpp>
#include <tip/db/pg/detail/md5.hpp>
#include <tip/db/pg/detail/result_impl.hpp>

#include <tip/db/pg/log.hpp>

namespace tip {
namespace db {
namespace pg {

class resultset;

namespace detail {

namespace {
/** Local logging facility */
using namespace tip::log;

const std::string LOG_CATEGORY = "PGFSM";
logger::event_severity DEFAULT_SEVERITY = logger::TRACE;
local
fsm_log(logger::event_severity s = DEFAULT_SEVERITY)
{
	return local(LOG_CATEGORY, s);
}

}  // namespace
// For more convenient changing severity, eg fsm_log(logger::WARNING)
using tip::log::logger;

struct transport_connected {};

struct authn_event {
	auth_states state;
	message_ptr message;
};

struct complete {};

struct ready_for_query {
	char status;
};

struct row_description {
	mutable std::vector<field_description> fields;
};

struct no_data {}; // Prepared query doesn't return data

struct terminate {};

namespace flags {
struct in_transaction{};
}  // namespace flags

template < typename TransportType, typename SharedType >
struct connection_fsm_ :
		public boost::msm::front::state_machine_def< connection_fsm_< TransportType, SharedType > >,
		public std::enable_shared_from_this< SharedType > {

	//@{
	/** @name Typedefs for MSM types */
	template < typename ... T >
	using Row = boost::msm::front::Row< T ... >;
	template < typename ... T >
	using Internal = boost::msm::front::Internal< T ... >;
	typedef boost::msm::front::none none;
	template < typename T >
	using Not = boost::msm::front::euml::Not_< T >;
	//@}
	//@{
	/** @name Misc typedefs */
	typedef TransportType transport_type;
	typedef SharedType shared_type;
	typedef boost::msm::back::state_machine< connection_fsm_< transport_type, shared_type > > connection;
	typedef std::shared_ptr< message > message_ptr;
	typedef std::map< std::string, row_description > prepared_statements_map;
	typedef std::shared_ptr< result_impl > result_ptr;
	//@}
	//@{
	/** @name Actions */
	struct on_connection_error {
		template < typename SourceState, typename TargetState >
		void
		operator() (error::connection_error const& err, connection& fsm,
				SourceState&, TargetState&)
		{
			fsm_log(logger::ERROR) << "connection::on_connection_error Error: "
					<< err.what();
			fsm.notify_error(err);
		}
	};
	struct disconnect {
		template < typename SourceState, typename TargetState >
		void
		operator() (terminate const&, connection& fsm, SourceState&, TargetState&)
		{
			fsm_log() << "connection: disconnect";
			fsm.send(message(terminate_tag));
			fsm.close_transport();
		}
	};
	//@}
	//@{
	/** @name States */
	struct unplugged : public boost::msm::front::state<> {
		typedef boost::mpl::vector<
				terminate,
				events::begin,
				events::commit,
				events::rollback,
				events::execute,
				events::execute_prepared
			> deferred_events;
	};

	struct terminated : boost::msm::front::terminate_state<> {
		template < typename Event >
		void
		on_entry(Event const&, connection& fsm)
		{
			fsm_log(logger::DEBUG) << "entering: terminated";
			fsm.send(message(terminate_tag));
			fsm.notify_terminated();
		}
	};

	struct t_conn : public boost::msm::front::state<> {
		typedef boost::mpl::vector<
				terminate,
				events::begin,
				events::commit,
				events::rollback,
				events::execute,
				events::execute_prepared
			> deferred_events;
		void
		on_entry(connection_options const& opts, connection& fsm)
		{
			fsm_log() << "entering: transport connecting";
			fsm.connect_transport(opts);
		}

		template < typename Event, typename FSM >
		void
		on_entry(Event const&, FSM&)
		{ fsm_log(logger::WARNING) << "entering: transport connecting - unknown variant"; }

		template < typename Event >
		void
		on_exit(Event const&, connection& fsm)
		{
			fsm_log() << "leaving:  transport connecting";
			fsm.start_read();
		}
	};

	struct authn : public boost::msm::front::state<> {
		typedef boost::mpl::vector<
				terminate,
				events::begin,
				events::commit,
				events::rollback,
				events::execute,
				events::execute_prepared
			> deferred_events;
		template < typename Event >
		void
		on_entry(Event const&, connection& fsm)
		{
			fsm_log() << "entering: authenticating";
			fsm.send_startup_message();
		}
		template < typename Event, typename FSM >
		void
		on_exit(Event const&, FSM&)
		{ fsm_log() << "leaving: authenticating"; }

		struct handle_authn_event {
			template < typename SourceState, typename TargetState >
			void
			operator() (authn_event const& evt, connection& fsm, SourceState&, TargetState&)
			{
				fsm_log() << "authn: handle auth_event";
				switch (evt.state) {
					case OK: {
						fsm_log() << "Authenticated with postgre server";
						break;
					}
					case Cleartext : {
						fsm_log() << "Cleartext password requested";
						message pm(password_message_tag);
						pm.write(fsm.options().password);
						fsm.send(pm);
						break;
					}
					case MD5Password: {
						fsm_log() << "MD5 password requested";
						// Read salt
						std::string salt;
						evt.message->read(salt, 4);
						connection_options const& co = fsm.options();
						// Calculate hash
						std::string pwdhash = boost::md5((co.password + co.user).c_str()).digest().hex_str_value();
						std::string md5digest = std::string("md5") + boost::md5( (pwdhash + salt).c_str() ).digest().hex_str_value();
						// Construct and send message
						message pm(password_message_tag);
						pm.write(md5digest);
						fsm.send(pm);
						break;
					}
					default : {
						std::stringstream err;
						err << "Unsupported authentication scheme "
								<< evt.state << " requested by server";
						fsm.process_event(error::connection_error(err.str()));
					}
				}
			}
		};

		struct internal_transition_table : boost::mpl::vector<
			Internal< authn_event, handle_authn_event,	none >
		> {};
	};

	struct idle : public boost::msm::front::state<> {
		template < typename Event >
		void
		on_entry(Event const&, connection& fsm)
		{
			fsm_log() << "entering: idle";
			fsm.notify_idle();
		}
		template < typename Event, typename FSM >
		void
		on_exit(Event const&, FSM&)
		{ fsm_log() << "leaving: idle"; }

		struct internal_transition_table : boost::mpl::vector<
		/*				Event			Action		Guard	 */
		/*			+-----------------+-----------+---------+*/
			Internal< ready_for_query,	none,		none 	>
		> {};
	};

	struct transaction_ : public boost::msm::front::state_machine_def<transaction_> {
		typedef boost::mpl::vector< terminate > deferred_events;
		typedef boost::mpl::vector< flags::in_transaction > flag_list;
		typedef boost::msm::back::state_machine< transaction_ > tran_fsm;

		typedef std::shared_ptr< pg::transaction > transaction_ptr;
		typedef std::weak_ptr< pg::transaction > transaction_weak_ptr;

		//@{
		/** @name Transaction entry-exit */
		void
		on_entry(events::begin const& evt, connection& fsm)
		{
			fsm_log(logger::DEBUG) << "entering: transaction";
			connection_ = &fsm;
			connection_->in_transaction_ = true;
			callbacks_ = evt;
		}
		template < typename Event, typename FSM >
		void
		on_exit(Event const&, FSM&)
		{
			fsm_log(logger::DEBUG) << "leaving: transaction";
			tran_object_.reset();
			connection_->in_transaction_ = false;
			callbacks_ = events::begin{};
		}
		//@}

		//@{
		/** State forwards */
		struct tran_error;
		//@}
		//@{
		/** @name Actions */
		struct transaction_started {
			template < typename Event, typename SourceState, typename TargetState >
			void
			operator() (Event const&, tran_fsm& tran, SourceState&, TargetState&)
			{
				fsm_log() << "transaction::transaction_started";
				tran.notify_started();
			}
		};
		struct commit_transaction {
			template < typename SourceState, typename TargetState >
			void
			operator() (events::commit const&, tran_fsm& tran, SourceState&, TargetState&)
			{
				fsm_log() << "transaction::commit_transaction";
				tran.connection_->send_commit();
			}
		};
		struct rollback_transaction {
			template < typename SourceState, typename TargetState >
			void
			operator() (events::rollback const&, tran_fsm& tran, SourceState&, TargetState&)
			{
				fsm_log() << "transaction::rollback_transaction";
				tran.connection_->send_rollback();
				tran.notify_error(error::query_error("Transaction rolled back"));
			}

			template < typename Event, typename SourceState, typename TargetState >
			void
			operator() (Event const&, tran_fsm& tran, SourceState&, TargetState&)
			{
				fsm_log() << "transaction::rollback_transaction (on error)";
				tran.connection_->send_rollback();
				tran.notify_error(error::query_error("Transaction rolled back"));
			}
		};
		//@}
		//@{
		/** @name Transaction sub-states */
		struct starting : public boost::msm::front::state<> {
			typedef boost::mpl::vector<
					events::execute,
					events::execute_prepared,
					events::commit,
					events::rollback
				> deferred_events;

			//template < typename Event >
			void
			on_entry(events::begin const& evt, tran_fsm& tran)
			{
				fsm_log() << "entering: start transaction";
				tran.connection_->send_begin();
			}
			template < typename Event >
			void
			on_exit(Event const&, tran_fsm& tran)
			{ fsm_log() << "leaving: start transaction"; }
			struct internal_transition_table : boost::mpl::vector<
			/*				Event				Action		Guard	 */
			/*			+---------------------+-----------+---------+*/
				Internal< command_complete,		none,		none 	>
			> {};
		};

		struct idle : public boost::msm::front::state<> {
			template < typename Event, typename FSM >
			void
			on_entry(Event const&, FSM&)
			{ fsm_log() << "entering: idle (transaction)"; }
			template < typename Event, typename FSM >
			void
			on_exit(Event const&, FSM&)
			{ fsm_log() << "leaving: idle (transaction)"; }
			void
			on_exit(error::query_error const& err, tran_fsm& tran)
			{
				fsm_log(logger::DEBUG) << tip::util::MAGENTA
						<< "leaving: idle (transaction) (query_error)";
				tran.notify_error(err);
			}
			void
			on_exit(error::client_error const& err, tran_fsm& tran)
			{
				fsm_log(logger::DEBUG) << tip::util::MAGENTA
						<< "leaving: idle (transaction) (client_error)";
				tran.notify_error(err);
			}

			struct internal_transition_table : boost::mpl::vector<
			/*				Event				Action					Guard	 */
			/*			+---------------------+-----------------------+---------+*/
				Internal< command_complete,		none,					none 	>,
				Internal< ready_for_query,		none,					none 	>
			> {};
		};

		struct tran_error : public boost::msm::front::state<> {
			template < typename Event, typename FSM >
			void
			on_entry(Event const&, FSM&)
			{ fsm_log() << "entering: tran_error"; }
			template < typename Event, typename FSM >
			void
			on_exit(Event const&, FSM&)
			{ fsm_log() << "leaving: tran_error"; }
			struct internal_transition_table : boost::mpl::vector<
			/*				Event				Action					Guard	 */
			/*			+---------------------+-----------------------+---------+*/
				Internal< events::commit,		none,					none	>,
				Internal< events::rollback,		none,					none	>
			> {};
		};
		struct exiting : public boost::msm::front::state<> {
			template < typename Event, typename FSM >
			void
			on_entry(Event const&, FSM&)
			{ fsm_log() << "entering: exit transaction"; }
			template < typename Event, typename FSM >
			void
			on_exit(Event const&, FSM&)
			{ fsm_log() << "leaving: exit transaction"; }
			struct internal_transition_table : boost::mpl::vector<
			/*				Event				Action					Guard	 */
			/*			+---------------------+-----------------------+---------+*/
				Internal< command_complete,		none,					none 	>,
				Internal< events::commit,		none,					none	>,
				Internal< events::rollback,		none,					none	>
			> {};
		};

		struct simple_query_ : public boost::msm::front::state_machine_def<simple_query_> {
			typedef boost::msm::back::state_machine< simple_query_ > simple_query;
			template < typename Event >
			void
			on_entry(Event const& q, tran_fsm& tran)
			{
				fsm_log(logger::WARNING)
						<< tip::util::MAGENTA << "entering: simple query (unexpected event)";
				connection_ = tran.connection_;
			}
			void
			on_entry(events::execute const& q, tran_fsm& tran)
			{
				fsm_log(logger::DEBUG) << tip::util::MAGENTA << "entering: simple query (execute event)";
				fsm_log(logger::DEBUG) << "Execute query: " << q.expression;
				connection_ = tran.connection_;
				tran_ = &tran;
				query_ = q;
				message m(query_tag);
				m.write(q.expression);
				connection_->send(m);
			}
			template < typename Event, typename FSM >
			void
			on_exit(Event const&, FSM&)
			{
				fsm_log(logger::DEBUG) << tip::util::MAGENTA << "leaving: simple query";
				query_ = events::execute();
			}
			template < typename FSM >
			void
			on_exit(error::query_error const& err, FSM&)
			{
				fsm_log(logger::DEBUG) << tip::util::MAGENTA << "leaving: simple query (query_error)";
				tran_->notify_error(*this, err);
				query_ = events::execute();
			}
			template < typename FSM >
			void
			on_exit(error::client_error const& err, FSM&)
			{
				fsm_log(logger::DEBUG) << tip::util::MAGENTA << "leaving: simple query (client_error)";
				tran_->notify_error(err);
				query_ = events::execute();
			}

			typedef boost::mpl::vector<
					events::execute,
					events::execute_prepared,
					events::commit,
					events::rollback
				> deferred_events;

			//@{
			/** @name Simple query sub-states */
			struct waiting : public boost::msm::front::state<> {
				template < typename Event, typename FSM >
				void
				on_entry(Event const&, FSM&)
				{ fsm_log() << "entering: waiting"; }
				template < typename Event, typename FSM >
				void
				on_exit(Event const&, FSM&)
				{ fsm_log() << "leaving: waiting"; }
				struct non_select_result {
					void
					operator()(command_complete const& evt, simple_query& fsm,
							waiting&, waiting&)
					{
						fsm_log() << "Non-select query complete "
								<< evt.command_tag;
						// FIXME Non-select query result
						fsm.tran_->notify_result(fsm, result_ptr(new result_impl), true);
					}
				};

				struct internal_transition_table : boost::mpl::vector<
					Internal< command_complete,	non_select_result,	none >
				> {};
			};

			struct fetch_data : public boost::msm::front::state<> {
				typedef boost::mpl::vector< ready_for_query > deferred_events;

				fetch_data() : result_( new result_impl ) {}

				template < typename Event, typename FSM >
				void
				on_entry(Event const&, FSM&)
				{ fsm_log() << "entering: fetch_data (unexpected event)"; }

				template < typename FSM >
				void
				on_entry(row_description const& rd, FSM&)
				{
					fsm_log() << "entering: fetch_data column count: "
							<< rd.fields.size();
					result_.reset(new result_impl); // TODO Number of resultset
					result_->row_description().swap(rd.fields);
				}

				template < typename Event >
				void
				on_exit(Event const&, simple_query& fsm)
				{
					fsm_log() << "leaving: fetch_data result size: "
							<< result_->size();
					fsm.tran_->notify_result(fsm, resultset(result_), true);
				}

				void
				on_exit(error::db_error const& err, simple_query& fsm)
				{
					fsm_log() << "leaving: fetch_data on error";
				}

				struct parse_data_row {
					template < typename FSM, typename TargetState >
					void
					operator() (row_data const& row, FSM&, fetch_data& fetch, TargetState&)
					{
						fetch.result_->rows().push_back(row);
					}
				};

				struct internal_transition_table : boost::mpl::vector<
					Internal< row_data,	parse_data_row,	none >
				> {};

				result_ptr result_;
			};
			typedef waiting initial_state;
			//@}
			//@{
			/** @name Transitions */
			struct transition_table : boost::mpl::vector<
				/*		Start			Event				Next			Action			Guard			  */
				/*  +-----------------+-------------------+---------------+---------------+-----------------+ */
				 Row<	waiting,		row_description,	fetch_data,		none,			none			>,
				 Row<	fetch_data,		command_complete,	waiting,		none,			none			>
			> {};

			connection* connection_;
			tran_fsm* tran_;
			events::execute query_;
		};
		typedef boost::msm::back::state_machine< simple_query_ > simple_query;

		struct extended_query_ : public boost::msm::front::state_machine_def<extended_query_> {
			typedef boost::msm::back::state_machine< extended_query_ > extended_query;

			extended_query_() : connection_(nullptr), tran_(nullptr), row_limit_(0),
					result_(new result_impl) {}

			template < typename Event, typename FSM >
			void
			on_entry(Event const&, FSM&)
			{ fsm_log(logger::WARNING) << tip::util::MAGENTA << "entering: extended query (unexpected event)"; }
			void
			on_entry(events::execute_prepared const& q, tran_fsm& tran)
			{
				fsm_log(logger::DEBUG) << tip::util::MAGENTA << "entering: extended query";
				connection_ = tran.connection_;
				tran_ = &tran;
				query_ = q;
				std::ostringstream os;
				os << query_.expression;
				if (!query_.param_types.empty()) {
					os << "{";
					std::ostream_iterator< oids::type::oid_type > out(os, ",");
					std::copy( query_.param_types.begin(), query_.param_types.end() - 1, out );
					os << query_.param_types.back() << "}";
				}
				fsm_log() << "query signature " << os.str();
				query_name_ = "q_" +
					std::string( boost::md5( os.str().c_str() ).digest().hex_str_value() );
			}
			template < typename Event, typename FSM >
			void
			on_exit(Event const&, FSM&)
			{
				fsm_log(logger::DEBUG) << tip::util::MAGENTA << "leaving: extended query";
				query_ = events::execute_prepared();
			}
			template < typename FSM >
			void
			on_exit(error::query_error const& err, FSM&)
			{
				fsm_log(logger::DEBUG) << tip::util::MAGENTA << "leaving: extended query (query_error)";
				tran_->notify_error(*this, err);
				query_ = events::execute_prepared();
			}
			template < typename FSM >
			void
			on_exit(error::client_error const& err, FSM&)
			{
				fsm_log(logger::DEBUG) << tip::util::MAGENTA << "leaving: extended query (client_error)";
				tran_->notify_error(err);
				query_ = events::execute_prepared();
			}

			typedef boost::mpl::vector<
					events::execute,
					events::execute_prepared,
					events::commit,
					events::rollback
				> deferred_events;

			//@{
			struct store_prepared_desc {
				template < typename SourceState, typename TargetState >
				void
				operator() (row_description const& row, extended_query& fsm,
						SourceState&, TargetState&)
				{
					fsm.result_.reset(new result_impl);
					fsm.result_->row_description() = row.fields; // copy!
					fsm.connection_->set_prepared(fsm.query_name_, row);
				}
				template < typename SourceState, typename TargetState >
				void
				operator() (no_data const&, extended_query& fsm,
						SourceState&, TargetState&)
				{
					fsm.result_.reset(new result_impl);
					row_description row;
					fsm.connection_->set_prepared(fsm.query_name_, row);
				}
			};
			struct parse_data_row {
				template < typename SourceState, typename TargetState >
				void
				operator() (row_data const& row, extended_query& fsm,
						SourceState&, TargetState&)
				{
					fsm.result_->rows().push_back(row);
				}
			};
			struct complete_execution {
				template < typename SourceState, typename TargetState >
				void
				operator() (command_complete const& complete, extended_query& fsm,
						SourceState&, TargetState&)
				{
					fsm_log() << "Execute complete " << complete.command_tag
							<< " resultset columns "
							<< fsm.result_->row_description().size()
							<< " rows " << fsm.result_->size();
					fsm.tran_->notify_result(fsm, resultset(fsm.result_), true);
				}
			};
			//@}
			//@{
			/** @name Extended query sub-states */
			struct prepare : public boost::msm::front::state<> {};

			struct parse : public boost::msm::front::state<> {
				template < typename Event, typename FSM >
				void
				on_entry(Event const&, FSM&)
				{ fsm_log() << "entering: parse (unexpected fsm)"; }
				template < typename Event >
				void
				on_entry(Event const&, extended_query& fsm)
				{
					fsm_log() << "entering: parse";
					fsm_log(logger::DEBUG) << "Parse query " << fsm.query_.expression;
					message parse(parse_tag);
					parse.write(fsm.query_name_);
					parse.write(fsm.query_.expression);
					parse.write( (smallint)fsm.query_.param_types.size() );
					for (oids::type::oid_type oid : fsm.query_.param_types) {
						parse.write( (integer)oid );
					}

					message describe(describe_tag);
					describe.write('S');
					describe.write(fsm.query_name_);
					parse.pack(describe);

					parse.pack(message(sync_tag));

					fsm.connection_->send(parse);
				}
				template < typename Event, typename FSM >
				void
				on_exit(Event const&, FSM&)
				{ fsm_log() << "leaving: parse"; }

				struct internal_transition_table : boost::mpl::vector<
					Internal< row_description,	store_prepared_desc,	none >,
					Internal< no_data,			store_prepared_desc,	none >
				> {};
			};

			struct bind : public boost::msm::front::state<> {
				template < typename Event, typename FSM >
				void
				on_entry(Event const&, FSM&)
				{ fsm_log() << "entering: bind (unexpected fsm)"; }
				template < typename Event >
				void
				on_entry( Event const&, extended_query& fsm )
				{
					fsm_log() << "entering: bind";
					message bind(bind_tag);
					bind.write(fsm.portal_name_);
					bind.write(fsm.query_name_);
					if (!fsm.query_.params.empty()) {
						auto out = bind.output();
						std::copy(fsm.query_.params.begin(), fsm.query_.params.end(), out);
					} else {
						bind.write((smallint)0); // parameter format codes
						bind.write((smallint)0); // number of parameters
					}
					if (fsm.connection_->is_prepared(fsm.query_name_)) {
						row_description const& row =
								fsm.connection_->get_prepared(fsm.query_name_);
						bind.write((smallint)row.fields.size());
						for (auto fd : row.fields) {
							bind.write((smallint)fd.format_code);
						}
					} else {
						bind.write((smallint)0); // no row description
					}

					bind.pack(message(sync_tag));

					fsm.connection_->send(bind);
				}
				template < typename Event, typename FSM >
				void
				on_exit(Event const&, FSM&)
				{ fsm_log() << "leaving: bind"; }
			};

			struct exec : public boost::msm::front::state<> {
				template < typename Event, typename FSM >
				void
				on_entry(Event const&, FSM&)
				{ fsm_log() << "entering: execute (unexpected fsm)"; }
				template < typename Event >
				void
				on_entry( Event const&, extended_query& fsm )
				{
					fsm_log() << "entering: execute";
					fsm_log(logger::DEBUG) << "Execute prepared query: " << fsm.query_.expression;
					message execute(execute_tag);
					execute.write(fsm.portal_name_);
					execute.write(fsm.row_limit_);
					execute.pack(message(sync_tag));
					fsm.connection_->send(execute);
				}
				template < typename Event, typename FSM >
				void
				on_exit(Event const&, FSM&)
				{ fsm_log() << "leaving: execute"; }

				struct internal_transition_table : boost::mpl::vector<
					Internal< row_data,			parse_data_row,			none >,
					Internal< command_complete, complete_execution, 	none >
				> {};
			};

			typedef prepare initial_state;
			//@}

	        struct is_prepared
	        {
	            template < class EVT, class SourceState, class TargetState>
	            bool
				operator()(EVT const& evt, extended_query& fsm,
						SourceState& src,TargetState& tgt)
	            {
	            	if (fsm.connection_) {
	            		return fsm.connection_->is_prepared(fsm.query_name_);
	            	}
	                return false;
	            }
	        };
			//@{
			/** Transitions for extended query  */
	        /** @todo Row limits and portal suspended state */
	        /** @todo Exit on error handling */
			struct transition_table : boost::mpl::vector<
				/*		Start			Event				Next			Action			Guard			      */
				/*  +-----------------+-------------------+---------------+---------------+---------------------+ */
				 Row<	prepare,		none,				parse,			none,			Not<is_prepared>	>,
				 Row<	prepare,		none,				bind,			none,			is_prepared			>,
				 Row<	parse,			ready_for_query,	bind,			none,			none				>,
				 Row<	bind,			ready_for_query,	exec,			none,			none				>
			>{};
			//@}

			connection* connection_;
			tran_fsm* tran_;

			events::execute_prepared query_;
			std::string query_name_;
			std::string portal_name_;
			integer row_limit_;

			result_ptr result_;
		};  // extended_query_
		typedef boost::msm::back::state_machine< extended_query_ > extended_query;

		typedef starting initial_state;
		//@}


		//@{
		/** @name Transition table for transaction */
		struct transition_table : boost::mpl::vector<
			/*		Start			Event				Next			Action						Guard				  */
			/*  +-----------------+-------------------+-----------+---------------------------+---------------------+ */
			 Row<	starting,		ready_for_query,	idle,			transaction_started,		none			>,
			/*  +-----------------+-------------------+-----------+---------------------------+---------------------+ */
			 Row<	idle,			events::commit,		exiting,		commit_transaction,			none			>,
			 Row<	idle,			events::rollback,	exiting,		rollback_transaction,		none			>,
			 Row<	idle,			error::query_error,	exiting,		rollback_transaction,		none			>,
			 Row<	idle,			error::client_error,exiting,		rollback_transaction,		none			>,
			/*  +-----------------+-------------------+-----------+---------------------------+---------------------+ */
			 Row<	idle,			events::execute,	simple_query,	none,						none			>,
			 Row<	simple_query,	ready_for_query,	idle,			none,						none			>,
			 Row<	simple_query,	error::query_error,	tran_error,		none,						none			>,
			 Row<	simple_query,	error::client_error,tran_error,		none,						none			>,
			/*  +-----------------+-------------------+-----------+---------------------------+---------------------+ */
			 Row<	idle,			events::
			 	 	 	 	 	 	 execute_prepared,	extended_query,	none,						none			>,
			 Row<	extended_query,	ready_for_query,	idle,			none,						none			>,
		 	 Row<	extended_query,	error::query_error,	tran_error,		none,						none			>,
		 	 Row<	extended_query,	error::client_error,tran_error,		none,						none			>,
			/*  +-----------------+-------------------+-----------+---------------------------+---------------------+ */
			 Row< 	tran_error,		ready_for_query,	exiting,		rollback_transaction,		none			>
		> {};

		template < typename Event, typename FSM >
		void
		no_transition(Event const& e, FSM&, int state)
		{
			fsm_log(logger::DEBUG) << "No transition from state " << state
					<< " on event " << typeid(e).name() << " (in transaction)";
		}
		//@}

		void
		notify_started()
		{
			if (callbacks_.started) {
				transaction_ptr t(new pg::transaction( connection_->shared_from_this() ));
				tran_object_ = t;
				try {
					callbacks_.started(t);
				} catch (error::query_error const& e) {
					fsm_log(logger::ERROR)
							<< "Transaction started handler throwed a query_error:"
							<< e.what();
					connection_->process_event(e);
				} catch (error::db_error const& e) {
					fsm_log(logger::ERROR)
							<< "Transaction started handler throwed a db_error: "
							<< e.what();
					connection_->process_event(e);
				} catch (std::exception const& e) {
					fsm_log(logger::ERROR)
							<< "Transaction started handler throwed an exception: "
							<< e.what();
					connection_->process_event(error::client_error(e));
				} catch (...) {
					fsm_log(logger::ERROR)
							<< "Transaction started handler throwed an unknown exception";
					connection_->process_event(error::client_error("Unknown exception"));
				}
			}
		}
		template < typename Source >
		void
		notify_result(Source& state, resultset res, bool complete)
		{
			if (state.query_.result) {
				try {
					state.query_.result(res, complete);
				} catch (error::query_error const& e) {
					fsm_log(logger::ERROR)
							<< "Query result handler throwed a query_error: "
							<< e.what();
					connection_->process_event(e);
				} catch (error::db_error const& e) {
					fsm_log(logger::ERROR)
							<< "Query result handler throwed a db_error: "
							<< e.what();
					connection_->process_event(e);
				} catch (std::exception const& e) {
					fsm_log(logger::ERROR)
							<< "Query result handler throwed an exception: "
							<< e.what();
					connection_->process_event(error::client_error(e));
				} catch (...) {
					fsm_log(logger::ERROR)
							<< "Query result handler throwed an unknown exception";
					connection_->process_event(error::client_error("Unknown exception"));
				}
			}
		}

		void
		notify_error(error::db_error const& qe)
		{
			if (callbacks_.error) {
				// If the async error handler throws an exception
				// all we can do - log the error.
				try {
					callbacks_.error(qe);
				} catch (std::exception const& e) {
					fsm_log(logger::ERROR)
							<< "Transaction error handler throwed an exception: "
							<< e.what();
				} catch (...) {
					fsm_log(logger::ERROR)
							<< "Transaction error handler throwed an unknown exception.";
				}
			}
		}
		template < typename State >
		void
		notify_error(State& state, error::query_error const& qe)
		{
			if (state.query_.error) {
				try {
					state.query_.error(qe);
				} catch (std::exception const& e) {
					fsm_log(logger::DEBUG)
							<< "Query error handler throwed an exception: "
							<< e.what();
					notify_error(qe);
					notify_error(error::client_error(e));
				} catch (...) {
					fsm_log(logger::DEBUG)
							<< "Query error handler throwed an unexpected exception";
					notify_error(qe);
					notify_error(error::client_error("Unknown exception"));
				}
			} else {
				fsm_log(logger::WARNING) << "No query error handler";
				notify_error(qe);
			}
		}

		connection* connection_;
		events::begin callbacks_;
		transaction_weak_ptr tran_object_;
	}; // transaction state machine
	typedef boost::msm::back::state_machine< transaction_ > transaction;

	typedef unplugged initial_state;
	//@}
	//@{
	/** @name Transition table */
	struct transition_table : boost::mpl::vector<
		/*		Start			Event				Next			Action						Guard					  */
		/*  +-----------------+-------------------+---------------+---------------------------+---------------------+ */
	    Row <	unplugged,		connection_options,	t_conn,			none,						none				>,
	    Row <	t_conn,			complete,			authn,			none,						none				>,
	    Row <	t_conn,			error::
	    						connection_error,	terminated,		on_connection_error,		none				>,
	    Row <	authn,			ready_for_query,	idle,			none,						none				>,
	    Row <	authn,			error::
	    						connection_error,	terminated,		on_connection_error,		none				>,
	    /*									Transitions from idle													  */
		/*  +-----------------+-------------------+---------------+---------------------------+---------------------+ */
	    Row	<	idle,			events::begin,		transaction,	none,						none				>,
	    Row <	idle,			terminate,			terminated,		disconnect,					none				>,
	    Row <	idle,			error::
	    						connection_error,	terminated,		on_connection_error,		none				>,
		/*  +-----------------+-------------------+---------------+---------------------------+---------------------+ */
	    Row <	transaction,	ready_for_query,	idle,			none,						none				>,
	    Row <	transaction,	error::
	    						connection_error,	terminated,		on_connection_error,		none				>
	> {};
	//@}
	template < typename Event, typename FSM >
	void
	no_transition(Event const& e, FSM&, int state)
	{
		fsm_log(logger::DEBUG) << "No transition from state " << state
				<< " on event " << typeid(e).name();
	}

	//@{
	typedef asio_config::io_service_ptr io_service_ptr;
	typedef std::map< std::string, std::string > client_options_type;
	typedef connection_fsm_< transport_type, shared_type > this_type;
	typedef std::enable_shared_from_this< shared_type > shared_base;

	typedef std::function< void (asio_config::error_code const& error,
			size_t bytes_transferred) > asio_io_handler;
	//@}

	//@{
	connection_fsm_(io_service_ptr svc, client_options_type const& co)
		: shared_base(), transport_(svc), client_opts_(co), strand_(*svc),
		  serverPid_(0), serverSecret_(0), in_transaction_(false)
	{
		incoming_.prepare(8192); // FIXME Magic number, move to configuration
	}
	virtual ~connection_fsm_() {}
	//@}

	void
	connect_transport(connection_options const& opts)
	{
		if (opts.uri.empty()) {
			throw error::connection_error("No connection uri!");
		}
		if (opts.database.empty()) {
			throw error::connection_error("No database!");
		}
		if (opts.user.empty()) {
			throw error::connection_error("User not specified!");
		}
		conn_opts_ = opts;
		transport_.connect_async(conn_opts_,
            boost::bind(&connection_fsm_::handle_connect,
                shared_base::shared_from_this(),
				_1));
	}
	void
	close_transport()
	{
		transport_.close();
	}

	void
	start_read()
	{
		transport_.async_read(incoming_,
			strand_.wrap( boost::bind(
				&connection_fsm_::handle_read, shared_base::shared_from_this(),
				_1, _2 )
		));
	}

	void
	send_startup_message()
	{
		message m(empty_tag);
		create_startup_message(m);
		send(m);
	}
	void
	send_begin()
	{
		message m(query_tag);
		m.write("begin");
		send(m);
	}
	void
	send_commit()
	{
		message m(query_tag);
		m.write("commit");
		send(m);
	}
	void
	send_rollback()
	{
		message m(query_tag);
		m.write("rollback");
		send(m);
	}
	void
	send(message const& m, asio_io_handler handler = asio_io_handler())
	{
		if (transport_.connected()) {
			auto data_range = m.buffer();
			if (!handler) {
				handler = boost::bind(
						&this_type::handle_write, shared_base::shared_from_this(),
						_1, _2 );
			}
			transport_.async_write(
				ASIO_NAMESPACE::buffer(&*data_range.first,
						data_range.second - data_range.first),
				strand_.wrap(handler)
			);
		}
	}

	connection_options const&
	options() const
	{ return conn_opts_; }

	//@{
	/** @name Prepared queries */
	bool
	is_prepared ( std::string const& query )
	{
		return prepared_.count(query);
	}

	void
	set_prepared( std::string const& query, row_description const& row )
	{
		prepared_.insert(std::make_pair(query, row));
	}
	row_description const&
	get_prepared( std::string const& query_name ) const
	{
		auto f = prepared_.find(query_name);
		if (f != prepared_.end()) {
			return f->second;
		}
		throw error::db_error("Query is not prepared");
	}
	//@}

	//@{
	bool
	in_transaction() const
	{
		return in_transaction_;
	}
	//@}
	//@{
	/** @connection events notifications */
	void
	notify_idle() { do_notify_idle(); }
	void
	notify_terminated()
	{
		do_notify_terminated();
	}
	void
	notify_error(error::connection_error const& e) { do_notify_error(e); }
	//@}
private:
	virtual void do_notify_idle() {};
	virtual void do_notify_terminated() {};
	virtual void do_notify_error(error::connection_error const& e) {};
private:
	connection&
	fsm()
	{
		return static_cast< connection& >(*this);
	}
	connection const&
	fsm() const
	{
		return static_cast< connection const& >(*this);
	}

    void
    handle_connect(asio_config::error_code const& ec)
    {
        if (!ec) {
            fsm().process_event(complete());
        } else {
            fsm().process_event( error::connection_error(ec.message()) );
        }
    }
	void
	handle_read(asio_config::error_code const& ec, size_t bytes_transferred)
	{
		if (!ec) {
			// read message
			std::istreambuf_iterator<char> in(&incoming_);
			read_message(in, bytes_transferred);
			// start async operation again
			start_read();
		} else {
			// Socket error - force termination
			fsm().process_event(error::connection_error(ec.message()));
		}
	}
	void
	handle_write(asio_config::error_code const& ec, size_t bytes_transferred)
	{
		if (ec) {
			// Socket error - force termination
			fsm().process_event(error::connection_error(ec.message()));
		}
	}

	template < typename InputIter, typename OutputIter >
	InputIter
	copy(InputIter in, InputIter end, size_t max, OutputIter out)
	{
		for (int i = 0; i < max && in != end; ++i) {
			*out++ = *in++;
		}
		return in;
	}

	void
	read_message( std::istreambuf_iterator< char > in, size_t max_bytes )
	{
		const size_t header_size = sizeof(integer) + sizeof(byte);
	    while (max_bytes > 0) {
	        size_t loop_beg = max_bytes;
	        if (!message_) {
	            message_.reset(new detail::message);
	        }
	        auto out = message_->output();

	        std::istreambuf_iterator<char> eos;
	        if (message_->buffer_size() < header_size) {
	            // Read the header
	            size_t to_read = std::min((header_size - message_->buffer_size()), max_bytes);
	            in = copy(in, eos, to_read, out);
	            max_bytes -= to_read;
	        }
	        if (message_->length() > message_->size()) {
	            // Read the message body
	            size_t to_read = std::min(message_->length() - message_->size(), max_bytes);
	            in = copy(in, eos, to_read, out);
	            max_bytes -= to_read;
	        	assert(message_->size() <= message_->length()
	        			&& "Read too much from the buffer" );
	        }
	        if (message_->size() >= 4 && message_->length() == message_->size()) {
	            message_ptr m = message_;
	            m->reset_read();
	            handle_message(m);
	            message_.reset();
	        }
	        {
	            fsm_log(logger::OFF) << loop_beg - max_bytes
	            		<< " bytes consumed, " << max_bytes << " bytes left";
	        }
	    }
	}

	void
	create_startup_message(message& m)
	{
		m.write(PROTOCOL_VERSION);
		// Create startup packet
		m.write(options::USER);
		m.write(conn_opts_.user);
		m.write(options::DATABASE);
		m.write(conn_opts_.database);

		for (auto opt : client_opts_) {
			m.write(opt.first);
			m.write(opt.second);
		}
		// trailing terminator
		m.write('\0');
	}

	void
	handle_message(message_ptr m)
	{
		message_tag tag = m->tag();
	    if (message::backend_tags().count(tag)) {
	    	switch (tag) {
	    		case authentication_tag: {
	    			integer auth_state(-1);
	    			m->read(auth_state);
	    			fsm().process_event(authn_event{ (auth_states)auth_state, m });
	    			break;
	    		}
				case command_complete_tag: {
					command_complete cmpl;
					m->read(cmpl.command_tag);
					fsm_log() << "Command complete ("
							<< cmpl.command_tag << ")";
					fsm().process_event(cmpl);
					break;
				}
				case backend_key_data_tag: {
					m->read(serverPid_);
					m->read(serverSecret_);
					break;
				}
				case error_response_tag: {
					notice_message msg;
					m->read(msg);

					fsm_log(logger::ERROR) << "Error " << msg ;
					error::query_error err(msg.message, msg.severity,
							msg.sqlstate, msg.detail);
					fsm().process_event(err);
					break;
				}
				case parameter_status_tag: {
					std::string key;
					std::string value;

					m->read(key);
					m->read(value);

					fsm_log() << "Parameter " << key << " = " << value;
					client_opts_[key] = value;
					break;
				}
				case notice_response_tag : {
					notice_message msg;
					m->read(msg);
					fsm_log(logger::INFO) << "Notice " << msg;
					break;
				}
				case ready_for_query_tag: {
					char stat(0);
					m->read(stat);
					fsm_log() << "Database "
						<< (util::CLEAR) << (util::RED | util::BRIGHT)
						<< conn_opts_.uri
						<< "[" << conn_opts_.database << "]"
						<< logger::severity_color()
						<< " is ready for query (" << stat << ")";
					fsm().process_event(ready_for_query{ stat });
					break;
				}
				case row_description_tag: {
					row_description rd;
					smallint col_cnt;
					m->read(col_cnt);
					rd.fields.reserve(col_cnt);
					for (int i =0; i < col_cnt; ++i) {
						field_description fd;
						if (m->read(fd)) {
							rd.fields.push_back(fd);
						} else {
							fsm_log(logger::ERROR)
									<< "Failed to read field description " << i;
							// FIXME Process error
						}
					}
					fsm().process_event(rd);
					break;
				}
				case data_row_tag: {
					row_data row;
					if (m->read(row)) {
						fsm().process_event(row);
					} else {
						// FIXME Process error
						fsm_log(logger::ERROR) << "Failed to read data row";
					}
					break;
				}
				case parse_complete_tag: {
					fsm_log() << "Parse complete";
					break;
				}
				case parameter_desription_tag: {
					fsm_log() << "Parameter descriptions";
					break;
				}
				case bind_complete_tag: {
					fsm_log() << "Bind complete";
					break;
				}
				case no_data_tag: {
					fsm().process_event(no_data());
					break;
				}
				default: {
				    {
				        fsm_log(logger::TRACE) << "Unhandled message "
				        		<< (util::MAGENTA | util::BRIGHT)
				        		<< (char)tag
				        		<< logger::severity_color();
				    }
					break;
				}
			}

	    }
	}
private:
	friend class transaction_;
	transport_type transport_;

	client_options_type client_opts_;

	asio_config::io_service::strand strand_;
	ASIO_NAMESPACE::streambuf incoming_;

	message_ptr message_;

	integer serverPid_;
	integer	serverSecret_;

	prepared_statements_map prepared_;

	bool in_transaction_;
protected:
	connection_options conn_opts_;
};

template < typename TransportType >
class concrete_connection : public basic_connection,
	public boost::msm::back::state_machine< connection_fsm_< TransportType,
		concrete_connection< TransportType > > > {
public:
	typedef TransportType transport_type;
	typedef concrete_connection< transport_type > this_type;
private:
	typedef boost::msm::back::state_machine< connection_fsm_< transport_type,
			this_type > > fsm_type;
public:
	concrete_connection(io_service_ptr svc,
			client_options_type const& co,
			connection_callbacks const& callbacks)
		: basic_connection(), fsm_type(svc, co),
		  callbacks_(callbacks)
	{
		fsm_type::start();
	}
	virtual ~concrete_connection() {}
private:
	//@{
	/** @name State machine abstract interface implementation */
	virtual void
	do_notify_idle()
	{
		if (callbacks_.idle) {
			callbacks_.idle(fsm_type::shared_from_this());
		} else {
			fsm_log(logger::WARNING) << "No connection idle callback";
		}
	}
	virtual void
	do_notify_terminated()
	{
		if (callbacks_.terminated) {
			callbacks_.terminated(fsm_type::shared_from_this());
		} else {
			fsm_log() << "No connection terminated callback";
		}
		callbacks_ = connection_callbacks(); // clean up callbacks, no work further.
	}
	virtual void
	do_notify_error(error::connection_error const& e)
	{
		fsm_log(logger::ERROR) << "Connection error " << e.what();
		if (callbacks_.error) {
			callbacks_.error(connection_ptr(), e);
		} else {
			fsm_log(logger::ERROR) << "No connection_error callback";
		}
	}
	//@}

	virtual void
	do_connect(connection_options const& co)
	{
		fsm_type::process_event(co);
	}

	virtual dbalias const&
	get_alias() const
	{
		return fsm_type::conn_opts_.alias;
	}

	virtual bool
	is_in_transaction() const
	{
		return fsm_type::in_transaction();
	}
	virtual void
	do_begin(events::begin const& evt)
	{
		if (fsm_type::in_transaction()) {
			fsm_log(logger::ERROR) << "Cannot begin transaction: already in transaction";
			throw error::db_error("Already in transaction");
		}
		fsm_type::process_event(evt);
	}

	virtual void
	do_commit()
	{
		if (!fsm_type::in_transaction()) {
			fsm_log(logger::ERROR) << "Cannot commit transaction: not in transaction";
			throw error::db_error("Not in transaction");
		}
		fsm_type::process_event(events::commit{});
	}

	virtual void
	do_rollback()
	{
		if (!fsm_type::in_transaction()) {
			fsm_log(logger::ERROR) << "Cannot rollback transaction: not in transaction";
			throw error::db_error("Not in transaction");
		}
		fsm_type::process_event(events::rollback{});
	}

	virtual void
	do_execute(events::execute const& query)
	{
		fsm_type::process_event(query);
	}

	virtual void
	do_execute(events::execute_prepared const& query)
	{
		fsm_type::process_event(query);
	}

	virtual void
	do_terminate()
	{
		fsm_type::process_event(detail::terminate{});
	}
private:
	connection_callbacks callbacks_;
};

}  // namespace detail
}  // namespace pg
}  // namespace db
}  // namespace tip


#endif /* LIB_PG_ASYNC_SRC_TIP_DB_PG_DETAIL_BASIC_CONNECTION_NEW_HPP_ */
