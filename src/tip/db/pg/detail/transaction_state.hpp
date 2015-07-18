/*
 * transaction_state.hpp
 *
 *  Created on: Jul 16, 2015
 *      Author: zmij
 */

#ifndef TIP_DB_PG_DETAIL_TRANSACTION_STATE_HPP_
#define TIP_DB_PG_DETAIL_TRANSACTION_STATE_HPP_

#include <tip/db/pg/detail/basic_state.hpp>

namespace tip {
namespace db {
namespace pg {
namespace detail {

class transaction_state: public tip::db::pg::detail::basic_state {
public:
	transaction_state(connection_base&, simple_callback, error_callback, bool autocommit);
	virtual ~transaction_state()
	{}

private:
	virtual bool
	do_handle_message(message_ptr);

	virtual bool
	do_handle_error(notice_message const& msg);

	virtual std::string const
	get_name() const
	{ return "transaction"; }

	virtual connection::state_type
	get_state() const
	{ return connection::BUSY; }

	virtual void
	do_enter();
	virtual void
	do_exit();

	virtual void
	do_handle_unlocked();

	virtual void
	do_commit_transaction(simple_callback, error_callback);
	virtual void
	do_rollback_transaction(simple_callback, error_callback);

	virtual void
	do_execute_query(std::string const& q, result_callback cb, query_error_callback err);

	virtual void
	do_terminate(simple_callback);

	void
	dirty_exit(simple_callback);

	bool autocommit_;
	bool message_pending_;
	bool complete_;

	simple_callback command_complete_;
	error_callback error_;

};

} /* namespace detail */
} /* namespace pg */
} /* namespace db */
} /* namespace tip */

#endif /* TIP_DB_PG_DETAIL_TRANSACTION_STATE_HPP_ */
