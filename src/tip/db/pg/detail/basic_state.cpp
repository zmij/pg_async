/**
 * @file /tip-server/src/tip/db/pg/detail/connection_state.cpp
 * @brief
 * @date Jul 10, 2015
 * @author: zmij
 */

#include <tip/db/pg/detail/basic_state.hpp>
#include <tip/db/pg/detail/startup.hpp>
#include <tip/db/pg/detail/transaction_state.hpp>
#include <tip/db/pg/detail/terminated_state.hpp>
#include <tip/db/pg/detail/basic_connection.hpp>
#include <tip/db/pg/error.hpp>

#include <tip/db/pg/log.hpp>

#include <assert.h>

namespace tip {
namespace db {
namespace pg {
namespace detail {

namespace {
/** Local logging facility */
using namespace tip::log;

const std::string LOG_CATEGORY = "PGSTATE";
logger::event_severity DEFAULT_SEVERITY = logger::TRACE;
local
local_log(logger::event_severity s = DEFAULT_SEVERITY)
{
	return local(LOG_CATEGORY, s);
}

}  // namespace
// For more convenient changing severity, eg local_log(logger::WARNING)
using tip::log::logger;

basic_state::basic_state(connection_base& conn)
		: conn(conn), exited(false)
{
}

std::string const
basic_state::name() const
{
	return get_name();
}

connection::state_type
basic_state::state() const
{
	return get_state();
}

void
basic_state::enter()
{
	#ifdef WITH_TIP_LOG
	{
		auto local = local_log();
		local << "Enter "
				<< (util::CLEAR) << (util::RED | util::BRIGHT)
				<< name()
				<< logger::severity_color()
				<< " state";
	}
	#endif
	do_enter();
}

void
basic_state::exit()
{
	#ifdef WITH_TIP_LOG
	{
		auto local = local_log();
		local << "Exit "
				<< (util::CLEAR) << (util::RED | util::BRIGHT)
				<< name()
				<< logger::severity_color()
				<< " state";
	}
	#endif
	exited = true;
	do_exit();
}

bool
basic_state::handle_message(message_ptr m)
{
	return do_handle_message(m);
}

bool
basic_state::handle_error(notice_message const& m)
{
	return do_handle_error(m);
}

bool
basic_state::handle_complete(command_complete_message const& m)
{
	return do_handle_complete(m);
}

void
basic_state::package_complete(size_t bytes)
{
	on_package_complete(bytes);
}

void
basic_state::handle_unlocked()
{
	do_handle_unlocked();
}

void
basic_state::send_command(message& m)
{
}

void
basic_state::next_state(state_ptr s)
{
}

void
basic_state::begin_transaction(simple_callback const& cb,
		error_callback const& err, bool autocommit)
{
	do_begin_transaction(cb, err, autocommit);
}

void
basic_state::commit_transaction(simple_callback const& cb,
		error_callback const& err)
{
	do_commit_transaction(cb, err);
}

void
basic_state::rollback_transaction(simple_callback const& cb,
		error_callback const& err)
{
	do_rollback_transaction(cb, err);
}

void
basic_state::do_begin_transaction(simple_callback const& cb,
		error_callback const& err, bool autocommit)
{
	std::ostringstream msg;
	msg << "Cannot start transaction in " << name() << " state";
	local_log(logger::ERROR) << msg.str();
	if (!err)
		throw query_error(msg.str());
	else
		err(query_error(msg.str()));
}

void
basic_state::do_commit_transaction(simple_callback const& cb,
		error_callback const& err)
{
	std::ostringstream msg;
	msg << "Cannot commit transaction in " << name() << " state";
	local_log(logger::ERROR) << msg.str();
	if (!err)
		throw query_error(msg.str());
	else
		err(query_error(msg.str()));
}

void
basic_state::do_rollback_transaction(simple_callback const& cb,
		error_callback const& err)
{
	std::ostringstream msg;
	msg << "Cannot rollback transaction in " << name() << " state";
	local_log(logger::ERROR) << msg.str();
	if (!err)
		throw query_error(msg.str());
	else
		err(query_error(msg.str()));
}

void
basic_state::execute_query(std::string const& q,
		result_callback const& cb, query_error_callback const& err)
{
	do_execute_query(q, cb, err);
}

void
basic_state::do_execute_query(std::string const&,
		result_callback const&, query_error_callback const& )
{
	#ifdef WITH_TIP_LOG
	local_log() << "Query executing is not available in " << name() << " state";
	#endif
}

void
basic_state::execute_prepared(std::string const& query,
		type_oid_sequence const& param_types,
		buffer_type const& params,
		result_callback const& cb,
		query_error_callback const& err)
{
	do_execute_prepared(query, param_types, params, cb, err);
}

void
basic_state::do_execute_prepared(std::string const&,
		type_oid_sequence const&,
		buffer_type const&,
		result_callback const&, query_error_callback const&)
{
	local_log() << "Query prepare is not available in " << name() << " state";
}

void
basic_state::terminate(simple_callback const& cb)
{
	do_terminate(cb);
}

void
basic_state::do_terminate(simple_callback const& cb)
{
	#ifdef WITH_TIP_LOG
	local_log() << "Terminate state "
			<< (util::CLEAR) << (util::RED | util::BRIGHT)
			<< name()
			<< logger::severity_color();
	#endif
	conn.pop_state(this);
	if (cb)
		cb();
}

//----------------------------------------------------------------------------
//	state_stack implementation
//----------------------------------------------------------------------------
namespace {
const std::string STACK_STATE_NAME = "state_stack";
}

state_stack::state_stack(connection_base& conn)
	: basic_state(conn)//, last_()
{
	stack_.push(state_ptr(new startup_state(conn)));
}

state_stack::~state_stack()
{
}

void
state_stack::push_state(state_ptr state)
{
	if (state) {
		{
			lock_type lock(mutex_);
			stack_.push(state);
		}
		state->enter();
	#ifdef WITH_TIP_LOG
	} else {
		local_log(logger::ERROR) << "Attempt to push an empty state";
	#endif
	}
}

void
state_stack::pop_state(basic_state* sender)
{
	assert(!stack_.empty() && "State stack is not empty");
	state_ptr state;
	bool empty = false;
	std::string top_name;
	{
		lock_type lock(mutex_);
		state = stack_.top();
		if (state.get() == sender) {
			stack_.pop();
		} else {
			local_log(logger::WARNING) << sender->name() << " trying to pop "
					<< state->name();
			state.reset();
		}
		//last_ = state->name();
		empty = stack_.empty();
		if (!empty)
			top_name = stack_.top()->name();
	}
	if (state)
		state->exit();
	#ifdef WITH_TIP_LOG
	if (empty) {
		local_log() << "#### State stack is empty";
	} else {
		local_log() << "Top state is " << top_name;
	}
	#endif
}

void
state_stack::transit_state(state_ptr state)
{
	if (state) {
		lock_type lock(mutex_);
		while (!stack_.empty()) {
			state_ptr s = stack_.top();
			pop_state(s.get());
		}
		push_state(state);
	#ifdef WITH_TIP_LOG
	} else {
		local_log(logger::ERROR) << "Attempt to transit to an empty state";
	#endif
	}
}

basic_state::state_ptr
state_stack::get()
{
	return current();
}

basic_state::state_const_ptr
state_stack::get() const
{
	return current();
}


basic_state::state_ptr
state_stack::current()
{
	lock_type lock(mutex_);
	assert(!stack_.empty() && "State stack is empty");
	local_log() << "State stack size " << stack_.size();
	return stack_.top();
}

basic_state::state_const_ptr
state_stack::current() const
{
	lock_type lock(mutex_);
	assert(!stack_.empty() && "State stack is empty");
	return stack_.top();
}

std::string const
state_stack::get_name() const
{
	return current()->name();
}

connection::state_type
state_stack::get_state() const
{
	return current()->state();
}

bool
state_stack::do_handle_message(message_ptr m)
{
	return current()->handle_message(m);
}

bool
state_stack::do_handle_error(notice_message const& m)
{
	return current()->handle_error(m);
}

bool
state_stack::do_handle_complete(command_complete_message const& m)
{
	return current()->handle_complete(m);
}

void
state_stack::on_package_complete(size_t bytes)
{
	current()->package_complete(bytes);
}

void
state_stack::do_handle_unlocked()
{
	current()->handle_unlocked();
}

void
state_stack::do_begin_transaction(simple_callback const& cb,
		error_callback const& err, bool autocommit)
{
	current()->begin_transaction(cb, err, autocommit);
}

void
state_stack::do_commit_transaction(simple_callback const& cb,
		error_callback const& err)
{
	while (!stack_.empty()) {
		state_ptr state = stack_.top();
		if (std::dynamic_pointer_cast<transaction_state>(state)) {
			break;
		} else {
			pop_state(state.get());
		}
	}

	current()->commit_transaction(cb, err);
}

void
state_stack::do_rollback_transaction(simple_callback const& cb,
		error_callback const& err)
{
	while (!stack_.empty()) {
		state_ptr state = stack_.top();
		if (std::dynamic_pointer_cast<transaction_state>(state)) {
			break;
		} else {
			pop_state(state.get());
		}
	}

	current()->rollback_transaction(cb, err);
}

void
state_stack::do_terminate(simple_callback const& cb)
{
	if (!stack_.empty()) {
		state_ptr top = current();
		if (std::dynamic_pointer_cast<terminated_state>(top)) {
			#ifdef WITH_TIP_LOG
			local_log() << "Already terminated";
			#endif
		} else {
			current()->terminate(
			[this, cb]() {
				terminate(cb);
			});
		}
	} else if (cb) {
		cb();
	}
}

bool
state_stack::in_transaction() const
{
	state_container states(stack_);
	while (!states.empty()) {
		state_ptr state = states.top();
		if (std::dynamic_pointer_cast<transaction_state>(state)) {
			return true;
		}
		states.pop();
	}
	return false;
}

void
state_stack::do_execute_query(std::string const& q,
		result_callback const& cb, query_error_callback const& err)
{
	current()->execute_query(q, cb, err);
}

void
state_stack::do_execute_prepared(std::string const& q,
		type_oid_sequence const& param_types,
		buffer_type const& params,
		result_callback const& cb, query_error_callback const& err)
{
	current()->execute_prepared(q, param_types, params, cb, err);
}

}  // namespace detail
}  // namespace pg
}  // namespace db
}  // namespace tip

