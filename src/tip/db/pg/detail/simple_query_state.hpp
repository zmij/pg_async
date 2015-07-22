/*
 * simple_query_state.h
 *
 *  Created on: 11 июля 2015 г.
 *     @author: zmij
 */

#ifndef TIP_DB_PG_DETAIL_SIMPLE_QUERY_STATE_HPP_
#define TIP_DB_PG_DETAIL_SIMPLE_QUERY_STATE_HPP_

#include <tip/db/pg/detail/fetch_state.hpp>

namespace tip {
namespace db {
namespace pg {
namespace detail {

class result_impl;
typedef std::shared_ptr<result_impl> result_ptr;

class simple_query_state: public fetch_state {
public:
	simple_query_state(connection_base& conn,
			std::string const& q,
			result_callback const& cb,
			query_error_callback const& err);
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
	do_execute_query(std::string const&q, result_callback const& cb,
			query_error_callback const&);
	virtual void
	do_execute_prepared(std::string const& q, buffer_type const& params,
			result_callback const&, query_error_callback const&);

	virtual bool
	do_handle_error(notice_message const&);

	virtual void
	do_handle_unlocked();
private:
	std::string exp_;
};

} /* namespace detail */
} /* namespace pg */
} /* namespace db */
} /* namespace tip */

#endif /* TIP_DB_PG_DETAIL_SIMPLE_QUERY_STATE_HPP_ */
