/**
 * @file /tip-server/src/tip/db/pg/detail/connection_state.hpp
 * @brief
 * @date Jul 10, 2015
 * @author: zmij
 */

#ifndef TIP_DB_PG_DETAIL_CONNECTION_STATE_HPP_
#define TIP_DB_PG_DETAIL_CONNECTION_STATE_HPP_

#include <memory>
#include <functional>
#include <string>
#include <boost/noncopyable.hpp>
#include <tip/db/pg/connection.hpp>

#include <stack>
#include <mutex>

namespace tip {
namespace db {
namespace pg {

class resultset;

namespace detail {

struct message;
struct notice_message;

typedef std::shared_ptr<message> message_ptr;

struct connection_base;


class basic_state : public std::enable_shared_from_this< basic_state >,
		private boost::noncopyable {
public:
	typedef std::shared_ptr<basic_state>	state_ptr;
	typedef std::shared_ptr<basic_state const>	state_const_ptr;
	typedef internal_result_callback result_callback;
public:
	basic_state(connection_base&);
	virtual ~basic_state() {}

	std::string const
	name() const;

	connection::state_type
	state() const;

	void
	enter();
	void
	exit();

	bool
	handle_message(message_ptr);
	bool
	handle_error(notice_message const&);
	void
	package_complete(size_t bytes);
	void
	handle_unlocked();

	//@{
	/** @name Transactions interface */
	void
	begin_transaction(simple_callback, error_callback, bool autocommit);
	void
	commit_transaction(simple_callback, error_callback);
	void
	rollback_transaction(simple_callback, error_callback);
	//@}
	//@{
	/** @name Querying interface */
	/**
	 * Execute a simple query
	 * @param q SQL script
	 * @param result callback
	 * @param error callback
	 */
	void
	execute_query(std::string const& q, result_callback, query_error_callback);
	/**
	 * Execute a query with prepare and bind
	 * @param q SQL script
	 * @param result callback
	 * @param error callback
	 */
	void
	execute_prepared(std::string const& query, result_callback, query_error_callback );
	//@}

	void
	terminate(simple_callback);

	template <typename T>
	std::shared_ptr<T>
	shared_this()
	{
		return std::dynamic_pointer_cast<T>(shared_from_this());
	}
protected:
	void
	send_command(message&);
	void
	next_state(state_ptr);
private:
	virtual std::string const
	get_name() const = 0;
	virtual connection::state_type
	get_state() const = 0;

	virtual void
	do_enter() {}
	virtual void
	do_exit() {}

	virtual bool
	do_handle_message(message_ptr) = 0;
	virtual bool
	do_handle_error(notice_message const&)
	{ return false; }

	virtual void
	on_package_complete(size_t bytes) {}

	virtual void
	do_handle_unlocked() {}

	virtual void
	do_begin_transaction(simple_callback, error_callback, bool autocommit);
	virtual void
	do_commit_transaction(simple_callback, error_callback);
	virtual void
	do_rollback_transaction(simple_callback, error_callback);

	virtual void
	do_execute_query(std::string const& q, result_callback, query_error_callback);
	virtual void
	do_execute_prepared(std::string const& q, result_callback, query_error_callback);

	virtual void
	do_terminate(simple_callback);
protected:
	connection_base& conn;
	bool exited;
};

class state_stack : public basic_state {
public:
	typedef std::stack<state_ptr> state_container;
	typedef std::recursive_mutex mutex_type;
	typedef std::lock_guard<mutex_type> lock_type;
public:
	state_stack(connection_base&);
	virtual ~state_stack();


	void
	push_state(state_ptr);
	void
	pop_state(basic_state*);

	void
	transit_state(state_ptr);

	state_ptr
	get();

	state_const_ptr
	get() const;

	bool
	in_transaction() const;

private:
	virtual std::string const
	get_name() const;
	virtual connection::state_type
	get_state() const;

	virtual bool
	do_handle_message(message_ptr);
	virtual bool
	do_handle_error(notice_message const&);

	virtual void
	on_package_complete(size_t bytes);

	virtual void
	do_handle_unlocked();

	virtual void
	do_begin_transaction(simple_callback, error_callback, bool autocommit);
	virtual void
	do_commit_transaction(simple_callback, error_callback);
	virtual void
	do_rollback_transaction(simple_callback, error_callback);

	virtual void
	do_execute_query(std::string const& q, result_callback, query_error_callback);
	virtual void
	do_execute_prepared(std::string const& q, result_callback, query_error_callback);

	virtual void
	do_terminate(simple_callback);

	state_ptr
	current();
	state_const_ptr
	current() const;
private:
	state_container	stack_;
	//std::string 	last_;
	mutable mutex_type		mutex_;
};

}  // namespace detail
}  // namespace pg
}  // namespace db
}  // namespace tip

#endif /* TIP_DB_PG_DETAIL_CONNECTION_STATE_HPP_ */
