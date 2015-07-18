/*
 * simple_query_state.h
 *
 *  Created on: 11 июля 2015 г.
 *     @author: zmij
 */

#ifndef TIP_DB_PG_DETAIL_SIMPLE_QUERY_STATE_HPP_
#define TIP_DB_PG_DETAIL_SIMPLE_QUERY_STATE_HPP_

#include <tip/db/pg/detail/basic_state.hpp>

namespace tip {
namespace db {
namespace pg {
namespace detail {

class result_impl;
typedef std::shared_ptr<result_impl> result_ptr;

class simple_query_state: public basic_state {
public:
	simple_query_state(connection_base& conn,
			std::string const& q,
			result_callback cb, query_error_callback err);
	virtual ~simple_query_state() {}
private:
	virtual bool
	do_handle_message(message_ptr);

	virtual std::string const
	get_name() const
	{ return "simple query"; }

	virtual connection::state_type
	get_state() const
	{ return connection::BUSY; }

	virtual void
	do_enter();

	virtual void
	do_execute_query(std::string const&q, result_callback cb, query_error_callback);

	virtual bool
	do_handle_error(notice_message const&);

	virtual void
	on_package_complete(size_t bytes);

	virtual void
	do_handle_unlocked();

	result_ptr
	result();
private:
	std::string exp_;
	result_callback callback_;
	query_error_callback error_;
	result_ptr result_;

	bool complete_;
};

} /* namespace detail */
} /* namespace pg */
} /* namespace db */
} /* namespace tip */

#endif /* TIP_DB_PG_DETAIL_SIMPLE_QUERY_STATE_HPP_ */
