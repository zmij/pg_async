/*
 * transaction_state.hpp
 *
 *  Created on: Jul 16, 2015
 *      Author: zmij
 */

#ifndef TIP_DB_PG_DETAIL_TRANSACTION_STATE_HPP_
#define TIP_DB_PG_DETAIL_TRANSACTION_STATE_HPP_

#include <tip/db/pg/detail/idle_state.hpp>

namespace tip {
namespace db {
namespace pg {
namespace detail {

class transaction_state: public idle_state {
public:
	transaction_state(connection_base&, simple_callback const&,
			error_callback const&, bool autocommit);
	virtual ~transaction_state()
	{}

private:
	virtual bool
	do_handle_message(message_ptr);
	virtual bool
	do_handle_complete( command_complete const& );

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
	do_commit_transaction(simple_callback const&, error_callback const&);
	virtual void
	do_rollback_transaction(simple_callback const&, error_callback const&);

	virtual void
	do_terminate(simple_callback const&);

	void
	dirty_exit(simple_callback const&);

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
