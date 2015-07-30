/*
 * fsm_tests.cpp
 *
 *  Created on: Jul 29, 2015
 *      Author: zmij
 */

#include <gtest/gtest.h>
#include <boost/msm/back/state_machine.hpp>
#include <boost/msm/front/state_machine_def.hpp>
#include <boost/msm/front/functor_row.hpp>
#include <boost/msm/front/euml/operator.hpp>

#include <tip/db/pg/log.hpp>

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


namespace msm = boost::msm;

namespace {

struct connect {
	// connection string
};
struct transport_connected {};

struct complete {};
struct error {};

struct begin {};
struct commit {};
struct rollback {};

struct execute {};
struct execute_prepared{};
struct ready_for_query {};

struct row_description {};
struct row_data {};

struct terminate {};

struct connection_ : public boost::msm::front::state_machine_def< connection_ > {
	template < typename ... T >
	using Row = boost::msm::front::Row< T ... >;
	template < typename ... T >
	using Internal = boost::msm::front::Internal< T ... >;
	typedef boost::msm::front::none none;
	template < typename T >
	using Not = boost::msm::front::euml::Not_< T >;
	//@{
	/** @name States */
	struct unplugged : public boost::msm::front::state<> {
		template < typename Event, typename FSM >
		void
		on_entry(Event const&, FSM&)
		{ local_log() << "entering: unplugged"; }
		template < typename Event, typename FSM >
		void
		on_exit(Event const&, FSM&)
		{ local_log() << "leaving: unplugged"; }
	};

	struct terminated : boost::msm::front::terminate_state<> {
		template < typename Event, typename FSM >
		void
		on_entry(Event const&, FSM&)
		{ local_log(logger::DEBUG) << "entering: terminated"; }
		template < typename Event, typename FSM >
		void
		on_exit(Event const&, FSM&)
		{ local_log(logger::DEBUG) << "leaving: terminated"; }
	};

	struct t_conn : public boost::msm::front::state<> {
		template < typename Event, typename FSM >
		void
		on_entry(Event const&, FSM&)
		{ local_log() << "entering: transport connecting"; }
		template < typename Event, typename FSM >
		void
		on_exit(Event const&, FSM&)
		{ local_log() << "leaving:  transport connecting"; }
	};

	struct authn : public boost::msm::front::state<> {
		template < typename Event, typename FSM >
		void
		on_entry(Event const&, FSM&)
		{ local_log() << "entering: authenticating"; }
		template < typename Event, typename FSM >
		void
		on_exit(Event const&, FSM&)
		{ local_log() << "leaving: authenticating"; }
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
		no_transition(Event const& e, FSM, int state)
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
	struct start_connect {
		template < typename FSM, typename SourceState, typename TargetState >
		void
		operator() (connect const&, FSM&, SourceState&, TargetState&)
		{
			local_log() << "connection::start_connect";
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
	typedef connection_ c;
	struct transition_table : boost::mpl::vector<
		/*		Start			Event		Next			Action						Guard				  */
		/*  +-----------------+-----------+---------------+---------------------------+---------------------+ */
	    Row <	unplugged,		connect,	t_conn,			start_connect,				none				>,
	    Row <	t_conn,			complete,	authn,			none,						none				>,
	    Row <	t_conn,			error,		terminated,		none,						none				>,
	    Row <	authn,			complete,	idle,			none,						none				>,
	    Row <	authn,			error,		terminated,		none,						none				>,
	    /*									Transitions from idle											  */
		/*  +-----------------+-----------+---------------+---------------------------+---------------------+ */
	    Row	<	idle,			begin,		transaction,	start_transaction,			none				>,
	    Row <	idle,			terminate,	terminated,		none,						none				>,
		/*  +-----------------+-----------+---------------+---------------------------+---------------------+ */
	    Row <	transaction,	complete,	idle,			finish_transaction,			none				>
	> {};
	//@}
	template < typename Event, typename FSM >
	void
	no_transition(Event const& e, FSM, int state)
	{
		local_log(logger::ERROR) << "No transition from state " << state
				<< " on event " << typeid(e).name();
	}
};

typedef boost::msm::back::state_machine< connection_ > connection;

}  // namespace

TEST(FSM, NormalFlow)
{
	connection c;
	c.start();
	//	Connection
	c.process_event(connect());		// unplugged 	-> t_conn
	c.process_event(complete());	// t_conn		-> authn
	c.process_event(complete());	// authn		-> idle

	// Begin transaction
	c.process_event(begin());		// idle			-> transaction::starting
	c.process_event(complete());	// transaction::starting -> transaction::idle

	// Commit transaction
	c.process_event(commit());		// transaction::idle	-> transaction::exiting
	c.process_event(complete());	// transaction::exiting	-> idle

	// Begin transaction
	c.process_event(begin());		// idle			-> transaction::starting
	c.process_event(complete());	// transaction::starting -> transaction::idle

	c.process_event(complete());	// transaction::exiting	-> idle

	// Rollback transaction
	c.process_event(rollback());	// transaction::idle	-> transaction::exiting
	c.process_event(complete());	// transaction::exiting	-> idle

	// Terminate connection
	c.process_event(terminate());	// idle -> X
}

TEST(FSM, TerminateTran)
{
	connection c;
	c.start();
	//	Connection
	c.process_event(connect());		// unplugged 	-> t_conn
	c.process_event(complete());	// t_conn		-> authn
	c.process_event(complete());	// authn		-> idle

	// Begin transaction
	c.process_event(begin());		// idle			-> transaction::starting
	c.process_event(complete());	// transaction::starting -> transaction::idle

	c.process_event(terminate());	// deferred event

	c.process_event(rollback());	// transaction::idle -> transaction::exiting
	c.process_event(complete());	// transaction::exiting -> idle
}

TEST(FSM, SimpleQueryMode)
{
	connection c;
	c.start();
	//	Connection
	c.process_event(connect());		// unplugged 	-> t_conn
	c.process_event(complete());	// t_conn		-> authn
	c.process_event(complete());	// authn		-> idle

	// Begin transaction
	c.process_event(begin());		// idle			-> transaction::starting
	c.process_event(complete());	// transaction::starting -> transaction::idle

	c.process_event(execute());		// transaction -> simple query
	c.process_event(row_description()); // waiting -> fetch_data
	for (int i = 0; i < 10; ++i) {
		c.process_event(row_data());
	}
	c.process_event(complete());	// fetch_data -> waiting

	c.process_event(row_description()); // waiting -> fetch_data
	for (int i = 0; i < 10; ++i) {
		c.process_event(row_data());
	}
	c.process_event(complete());	// fetch_data -> waiting

	c.process_event(ready_for_query()); // simple query -> transaction::idle
	c.process_event(commit());		// transaction::idle -> transaction::exiting
	c.process_event(complete());	// transaction -> idle
}

TEST(FSM, ExtendedQueryMode)
{
	connection c;
	c.start();
	//	Connection
	c.process_event(connect());		// unplugged 	-> t_conn
	c.process_event(complete());	// t_conn		-> authn
	c.process_event(complete());	// authn		-> idle

	// Begin transaction
	c.process_event(begin());		// idle			-> transaction::starting
	c.process_event(complete());	// transaction::starting -> transaction::idle

	// Start extended query mode
	c.process_event(execute_prepared()); // transaction::idle -> eqm::prepare -> parse
	c.process_event(complete());	// parse -> bind
	c.process_event(complete());	// bind -> exec
}
