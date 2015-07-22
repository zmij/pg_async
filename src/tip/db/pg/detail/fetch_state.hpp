/*
 * fetch_state.hpp
 *
 *  Created on: Jul 20, 2015
 *      Author: zmij
 */

#ifndef LIB_PG_ASYNC_SRC_TIP_DB_PG_DETAIL_FETCH_STATE_HPP_
#define LIB_PG_ASYNC_SRC_TIP_DB_PG_DETAIL_FETCH_STATE_HPP_

#include <tip/db/pg/detail/basic_state.hpp>

namespace tip {
namespace db {
namespace pg {
namespace detail {

class result_impl;
typedef std::shared_ptr<result_impl> result_ptr;

class fetch_state: public basic_state {
public:
	fetch_state(connection_base& conn,
			result_callback const& cb,
			query_error_callback const& err);
	virtual ~fetch_state() {}
protected:
	virtual bool
	do_handle_message(message_ptr);
private:
	virtual std::string const
	get_name() const
	{ return "fetch query"; }

	virtual void
	do_exit();

	virtual connection::state_type
	get_state() const
	{ return connection::BUSY; }
protected:
	virtual void
	on_package_complete(size_t bytes);

	result_ptr
	result();
protected:
	result_callback callback_;
	query_error_callback error_;
	result_ptr result_;

	bool complete_;
	int result_no_;
	ubigint bytes_read_;
};

} /* namespace detail */
} /* namespace pg */
} /* namespace db */
} /* namespace tip */

#endif /* LIB_PG_ASYNC_SRC_TIP_DB_PG_DETAIL_FETCH_STATE_HPP_ */
