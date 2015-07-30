/*
 * basic_connection.new.hpp
 *
 *  Created on: Jul 30, 2015
 *      Author: zmij
 */

#ifndef LIB_PG_ASYNC_SRC_TIP_DB_PG_DETAIL_BASIC_CONNECTION_NEW_HPP_
#define LIB_PG_ASYNC_SRC_TIP_DB_PG_DETAIL_BASIC_CONNECTION_NEW_HPP_

#include <boost/noncopyable.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <map>
#include <stack>
#include <set>
#include <memory>

#include <boost/msm/back/state_machine.hpp>
#include <boost/msm/front/state_machine_def.hpp>
#include <boost/msm/front/functor_row.hpp>
#include <boost/msm/front/euml/operator.hpp>

#include <tip/db/pg/common.hpp>
#include <tip/db/pg/detail/protocol.hpp>
#include <tip/db/pg/error.hpp>

#include <tip/db/pg/detail/md5.hpp>

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
local_log(logger::event_severity s = DEFAULT_SEVERITY)
{
	return local(LOG_CATEGORY, s);
}

}  // namespace
// For more convenient changing severity, eg local_log(logger::WARNING)
using tip::log::logger;

struct transport_connected {};

struct authn_event {
	auth_states state;
	message_ptr message;
};

struct complete {};

struct begin {};
struct commit {};
struct rollback {};

struct execute {};
struct execute_prepared{};
struct ready_for_query {};

struct row_description {};

struct terminate {};

template < typename TransportType >
struct connection_fsm_ :
		public boost::msm::front::state_machine_def< connection_fsm_< TransportType > >,
		public std::enable_shared_from_this< connection_fsm_< TransportType > > {

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
	typedef boost::msm::back::state_machine< connection_fsm_< transport_type > > fsm_type;
	typedef std::shared_ptr< message > message_ptr;
	//@}
	//@{
	/** @name States */
	struct unplugged : public boost::msm::front::state<> {};

	struct terminated : boost::msm::front::terminate_state<> {
		template < typename Event, typename FSM >
		void
		on_entry(Event const&, FSM&)
		{ local_log(logger::DEBUG) << "entering: terminated"; }
	};

	struct t_conn : public boost::msm::front::state<> {
		void
		on_entry(connection_options const& opts, fsm_type& fsm)
		{
			local_log() << "entering: transport connecting";
			fsm.connect_transport(opts);
		}

		template < typename Event, typename FSM >
		void
		on_entry(Event const&, FSM&)
		{ local_log() << "entering: transport connecting - unknown variant"; }

		template < typename Event >
		void
		on_exit(Event const&, fsm_type& fsm)
		{
			local_log() << "leaving:  transport connecting";
			fsm.start_read();
		}
	};

	struct authn : public boost::msm::front::state<> {
		template < typename Event >
		void
		on_entry(Event const&, fsm_type& fsm)
		{
			local_log() << "entering: authenticating";
			fsm.send_startup_message();
		}
		template < typename Event, typename FSM >
		void
		on_exit(Event const&, FSM&)
		{ local_log() << "leaving: authenticating"; }

		struct handle_authn_event {
			template < typename SourceState, typename TargetState >
			void
			operator() (authn_event const& evt, fsm_type& fsm, SourceState&, TargetState&)
			{
				local_log() << "authn: handle auth_event";
				switch (evt.state) {
					case OK: {
						fsm.process_event(complete());
						break;
					}
					case Cleartext : {
						local_log() << "Cleartext password requested";
						message pm(password_message_tag);
						pm.write(fsm.options().password);
						fsm.send(pm);
						break;
					}
					case MD5Password: {
						#ifdef WITH_TIP_LOG
						local_log() << "MD5 password requested";
						#endif
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
						fsm.process_event(connection_error(err.str()));
					}
				}
			}
		};

		struct internal_transition_table : boost::mpl::vector<
			Internal< authn_event, handle_authn_event,	none >
		> {};
	};

	struct idle : public boost::msm::front::state<> {
		template < typename Event, typename FSM >
		void
		on_entry(Event const&, FSM&)
		{ local_log() << "entering: idle"; }
		template < typename Event, typename FSM >
		void
		on_exit(Event const&, FSM&)
		{ local_log() << "leaving: idle"; }
	};

	struct transaction_ : public boost::msm::front::state_machine_def<transaction_> {
		template < typename Event, typename FSM >
		void
		on_entry(Event const&, FSM&)
		{ local_log(logger::DEBUG) << "entering: transaction"; }
		template < typename Event, typename FSM >
		void
		on_exit(Event const&, FSM&)
		{ local_log(logger::DEBUG) << "leaving: transaction"; }

		typedef boost::mpl::vector< terminate > deferred_events;

		//@{
		/** @name Transaction sub-states */
		struct starting : public boost::msm::front::state<> {
			template < typename Event, typename FSM >
			void
			on_entry(Event const&, FSM&)
			{ local_log() << "entering: starting"; }
			template < typename Event, typename FSM >
			void
			on_exit(Event const&, FSM&)
			{ local_log() << "leaving: starting"; }
		};

		struct idle : public boost::msm::front::state<> {
			template < typename Event, typename FSM >
			void
			on_entry(Event const&, FSM&)
			{ local_log() << "entering: idle (transaction)"; }
			template < typename Event, typename FSM >
			void
			on_exit(Event const&, FSM&)
			{ local_log() << "leaving: idle (transaction)"; }

			struct internal_transition_table : boost::mpl::vector<
				Internal< complete,	none,	none >
			> {};
		};

		struct exiting : public boost::msm::front::state<> {
			template < typename Event, typename FSM >
			void
			on_entry(Event const&, FSM&)
			{ local_log() << "entering: exit transaction"; }
			template < typename Event, typename FSM >
			void
			on_exit(Event const&, FSM&)
			{ local_log() << "leaving: exit transaction"; }
		};

		struct simple_query_ : public boost::msm::front::state_machine_def<simple_query_> {
			template < typename Event, typename FSM >
			void
			on_entry(Event const&, FSM&)
			{ local_log() << tip::util::MAGENTA << "entering: simple query"; }
			template < typename Event, typename FSM >
			void
			on_exit(Event const&, FSM&)
			{ local_log() << tip::util::MAGENTA << "leaving: simple query"; }

			typedef boost::mpl::vector< execute, execute_prepared, commit, rollback > deferred_events;

			//@{
			/** @name Simple query sub-states */
			struct waiting : public boost::msm::front::state<> {
				template < typename Event, typename FSM >
				void
				on_entry(Event const&, FSM&)
				{ local_log() << "entering: waiting"; }
				template < typename Event, typename FSM >
				void
				on_exit(Event const&, FSM&)
				{ local_log() << "leaving: waiting"; }
			};

			struct fetch_data : public boost::msm::front::state<> {
				typedef boost::mpl::vector< ready_for_query > deferred_events;
				template < typename Event, typename FSM >
				void
				on_entry(Event const&, FSM&)
				{ local_log() << "entering: fetch_data"; }
				template < typename Event, typename FSM >
				void
				on_exit(Event const&, FSM&)
				{ local_log() << "leaving: fetch_data"; }

				struct parse_data_row {
					template < typename FSM, typename SourceState, typename TargetState >
					void
					operator() (row_data const&, FSM&, SourceState&, TargetState&)
					{
						local_log() << "fetch_data: row_data";
					}
				};

				struct internal_transition_table : boost::mpl::vector<
					Internal< row_data,	parse_data_row,	none >
				> {};
			};
			typedef waiting initial_state;
			//@}
			//@{
			/** @name Transitions */
			struct transition_table : boost::mpl::vector<
				/*		Start			Event				Next			Action			Guard			  */
				/*  +-----------------+-------------------+---------------+---------------+-----------------+ */
				 Row<	waiting,		row_description,	fetch_data,		none,			none			>,
				 Row<	fetch_data,		complete,			waiting,		none,			none			>
			> {};
		};
		typedef boost::msm::back::state_machine< simple_query_ > simple_query;

		struct extended_query_ : public boost::msm::front::state_machine_def<extended_query_> {
			template < typename Event, typename FSM >
			void
			on_entry(Event const&, FSM&)
			{ local_log() << tip::util::MAGENTA << "entering: extended query"; }
			template < typename Event, typename FSM >
			void
			on_exit(Event const&, FSM&)
			{ local_log() << tip::util::MAGENTA << "leaving: extended query"; }


			//@{
			/** @name Extended query sub-states */
			struct prepare : public boost::msm::front::state<> {
				template < typename Event, typename FSM >
				void
				on_entry(Event const&, FSM&)
				{ local_log() << "entering: prepare"; }
				template < typename Event, typename FSM >
				void
				on_exit(Event const&, FSM&)
				{ local_log() << "leaving: prepare"; }
			};

			struct parse : public boost::msm::front::state<> {
				template < typename Event, typename FSM >
				void
				on_entry(Event const&, FSM&)
				{ local_log() << "entering: parse"; }
				template < typename Event, typename FSM >
				void
				on_exit(Event const&, FSM&)
				{ local_log() << "leaving: parse"; }
			};

			struct bind : public boost::msm::front::state<> {
				template < typename Event, typename FSM >
				void
				on_entry(Event const&, FSM&)
				{ local_log() << "entering: bind"; }
				template < typename Event, typename FSM >
				void
				on_exit(Event const&, FSM&)
				{ local_log() << "leaving: bind"; }
			};

			struct exec : public boost::msm::front::state<> {
				template < typename Event, typename FSM >
				void
				on_entry(Event const&, FSM&)
				{ local_log() << "entering: execute"; }
				template < typename Event, typename FSM >
				void
				on_exit(Event const&, FSM&)
				{ local_log() << "leaving: execute"; }
			};

			typedef prepare initial_state;
			//@}

	        struct is_prepared
	        {
	            template <class EVT,class FSM,class SourceState,class TargetState>
	            bool
				operator()(EVT const& evt,FSM& fsm,SourceState& src,TargetState& tgt)
	            {
	                return false;
	            }
	        };
			//@{
			/** Transitions */
			struct transition_table : boost::mpl::vector<
				/*		Start			Event				Next			Action			Guard			      */
				/*  +-----------------+-------------------+---------------+---------------+---------------------+ */
				 Row<	prepare,		none,				parse,			none,			Not<is_prepared>	>,
				 Row<	prepare,		none,				bind,			none,			is_prepared			>,
				 Row<	parse,			complete,			bind,			none,			none				>,
				 Row<	bind,			complete,			exec,			none,			none				>
			>{};
			//@}
		};
		typedef boost::msm::back::state_machine< extended_query_ > extended_query;

		typedef starting initial_state;
		//@}

		//@{
		/** @name Actions */
		struct  commit_transaction {
			template < typename FSM, typename SourceState, typename TargetState >
			void
			operator() (commit const&, FSM&, SourceState&, TargetState&)
			{
				local_log() << "transaction::commit_transaction";
			}
		};
		struct  rollback_transaction {
			template < typename FSM, typename SourceState, typename TargetState >
			void
			operator() (rollback const&, FSM&, SourceState&, TargetState&)
			{
				local_log() << "transaction::rollback_transaction";
			}
		};
		//@}

		//@{
		/** @name Transitions */
		struct transition_table : boost::mpl::vector<
			/*		Start			Event				Next			Action						Guard				  */
			/*  +-----------------+-------------------+-----------+---------------------------+---------------------+ */
			 Row<	starting,		complete,			idle,			none,						none			>,
			/*  +-----------------+-------------------+-----------+---------------------------+---------------------+ */
			 Row<	idle,			commit,				exiting,		commit_transaction,			none			>,
			 Row<	idle,			rollback,			exiting,		rollback_transaction,		none			>,
			/*  +-----------------+-------------------+-----------+---------------------------+---------------------+ */
			 Row<	idle,			execute,			simple_query,	none,						none			>,
			 Row<	simple_query,	ready_for_query,	idle,			none,						none			>,
			/*  +-----------------+-------------------+-----------+---------------------------+---------------------+ */
			 Row<	idle,			execute_prepared,	extended_query,	none,						none			>,
			 Row<	extended_query,	ready_for_query,	idle,			none,						none			>
		> {};

		template < typename Event, typename FSM >
		void
		no_transition(Event const& e, FSM&, int state)
		{
			local_log(logger::ERROR) << "No transition from state " << state
					<< " on event " << typeid(e).name() << " (in transaction)";
		}
		//@}
	};
	typedef boost::msm::back::state_machine< transaction_ > transaction;

	typedef unplugged initial_state;
	//@}
	//@{
	/** @name Actions */
	struct on_connection_error {
		template < typename SourceState, typename TargetState >
		void
		operator() (connection_error const& err, fsm_type& fsm, SourceState&, TargetState&)
		{
			local_log(logger::ERROR) << "connection::on_connection_error Error: "
					<< err.what();
		}
	};
	struct start_authn {
		template < typename FSM, typename SourceState, typename TargetState >
		void
		operator() (complete const&, FSM&, SourceState&, TargetState&)
		{
			local_log() << "connection::start_authn";
		}
	};
	struct start_idle {
		template < typename FSM, typename SourceState, typename TargetState >
		void
		operator() (complete const&, FSM&, SourceState&, TargetState&)
		{
			local_log() << "connection::start_idle";
		}
	};
	struct start_transaction {
		template < typename FSM, typename SourceState, typename TargetState >
		void
		operator() (begin const&, FSM&, SourceState&, TargetState&)
		{
			local_log() << "connection::start_transaction";
		}
	};
	struct finish_transaction {
		template < typename FSM, typename SourceState, typename TargetState >
		void
		operator() (complete const&, FSM&, SourceState&, TargetState&)
		{
			local_log() << "connection::finish_transaction";
		}
	};
	//@}
	//@{
	/** @name Transition table */
	struct transition_table : boost::mpl::vector<
		/*		Start			Event				Next			Action						Guard					  */
		/*  +-----------------+-------------------+---------------+---------------------------+---------------------+ */
	    Row <	unplugged,		connection_options,	t_conn,			none,						none				>,
	    Row <	t_conn,			complete,			authn,			none,						none				>,
	    Row <	t_conn,			connection_error,	terminated,		on_connection_error,		none				>,
	    Row <	authn,			complete,			idle,			none,						none				>,
	    Row <	authn,			connection_error,	terminated,		on_connection_error,		none				>,
	    /*									Transitions from idle													  */
		/*  +-----------------+-------------------+---------------+---------------------------+---------------------+ */
	    Row	<	idle,			begin,				transaction,	start_transaction,			none				>,
	    Row <	idle,			terminate,			terminated,		none,						none				>,
	    Row <	idle,			connection_error,	terminated,		on_connection_error,		none				>,
		/*  +-----------------+-------------------+---------------+---------------------------+---------------------+ */
	    Row <	transaction,	complete,			idle,			finish_transaction,			none				>,
	    Row <	transaction,	connection_error,	terminated,		on_connection_error,		none				>
	> {};
	//@}
	template < typename Event, typename FSM >
	void
	no_transition(Event const& e, FSM&, int state)
	{
		local_log(logger::ERROR) << "No transition from state " << state
				<< " on event " << typeid(e).name();
	}

	//@{
	typedef boost::asio::io_service io_service;
	typedef std::map< std::string, std::string > client_options_type;
	typedef connection_fsm_< transport_type > this_type;
	typedef std::enable_shared_from_this< this_type > shared_base;

	typedef std::function< void (boost::system::error_code const& error,
			size_t bytes_transferred) > asio_io_handler;
	//@}

	//@{
	connection_fsm_(io_service& svc, client_options_type const& co)
		: transport_(svc), client_opts_(co), strand_(svc),
		  serverPid_(0), serverSecret_(0)
	{
		incoming_.prepare(8192); // FIXME Magic number, move to configuration
	}
	//@}

	void
	connect_transport(connection_options const& opts)
	{
		if (opts.uri.empty()) {
			throw connection_error("No connection uri!");
		}
		if (opts.database.empty()) {
			throw connection_error("No database!");
		}
		if (opts.user.empty()) {
			throw connection_error("User not specified!");
		}
		conn_opts_ = opts;
		transport_.connect_async(conn_opts_,
		[&]( boost::system::error_code const& ec ) {
			if (!ec) {
				fsm().process_event(complete());
			} else {
				fsm().process_event( connection_error(ec.message()) );
			}
		});
	}
	void
	start_read()
	{
		transport_.async_read(incoming_,
			strand_.wrap( boost::bind(
				&connection_fsm_::handle_read, shared_base::shared_from_this(),
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred )
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
	send(message const& m, asio_io_handler handler = asio_io_handler())
	{
		if (transport_.connected()) {
			auto data_range = m.buffer();
			if (!handler) {
				handler = boost::bind(
						&this_type::handle_write, shared_base::shared_from_this(),
						boost::asio::placeholders::error,
						boost::asio::placeholders::bytes_transferred );
			}
			transport_.async_write(
				boost::asio::buffer(&*data_range.first,
						data_range.second - data_range.first),
				strand_.wrap(handler)
			);
		}
	}

	connection_options const&
	options() const
	{ return conn_opts_; }
private:
	fsm_type&
	fsm()
	{
		return static_cast< fsm_type& >(*this);
	}
	void
	handle_read(boost::system::error_code const& ec, size_t bytes_transferred)
	{
		if (!ec) {
			// read message
			std::istreambuf_iterator<char> in(&incoming_);
			read_message(in, bytes_transferred);
			// start async operation again
			start_read();
		} else {
			// Socket error - force termination
			fsm().process_event(connection_error(ec.message()));
		}
	}
	void
	handle_write(boost::system::error_code const& ec, size_t bytes_transferred)
	{
		if (ec) {
			// Socket error - force termination
			fsm().process_event(connection_error(ec.message()));
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
	            local_log(logger::OFF) << loop_beg - max_bytes
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
				case parameter_status_tag: {
					std::string key;
					std::string value;

					m->read(key);
					m->read(value);

					local_log() << "Parameter " << key << " = " << value;
					client_opts_[key] = value;
					break;
				}
				default: {
				    {
				        local_log(logger::TRACE) << "Handle message "
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
	transport_type transport_;

	connection_options conn_opts_;
	client_options_type client_opts_;

	boost::asio::io_service::strand strand_;
	boost::asio::streambuf incoming_;

	message_ptr message_;

	integer serverPid_;
	integer	serverSecret_;
};

class basic_connection;
typedef std::shared_ptr< basic_connection > basic_connection_ptr;

class basic_connection : public boost::noncopyable {
public:
	typedef boost::asio::io_service io_service;
	typedef std::map< std::string, std::string > client_options_type;
public:
	static basic_connection_ptr
	create(io_service& svc, connection_options const&, client_options_type const&);
	virtual ~basic_connection() {}

protected:
	basic_connection();
private:
};


template < typename TransportType >
class concrete_connection : public basic_connection,
	public boost::msm::back::state_machine< connection_fsm_< TransportType > >  {
public:
	typedef TransportType transport_type;
	typedef boost::msm::back::state_machine< connection_fsm_< transport_type > > state_machine_type;
public:
	concrete_connection(io_service& svc,
			client_options_type const& co)
		: basic_connection(), state_machine_type(std::ref(svc), co)
	{
	}
};

}  // namespace detail
}  // namespace pg
}  // namespace db
}  // namespace tip


#endif /* LIB_PG_ASYNC_SRC_TIP_DB_PG_DETAIL_BASIC_CONNECTION_NEW_HPP_ */
