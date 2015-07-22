/*
 * idle_state.hpp
 *
 *  Created on: Jul 10, 2015
 *      Author: zmij
 */

#ifndef TIP_DB_PG_DETAIL_IDLE_STATE_HPP_
#define TIP_DB_PG_DETAIL_IDLE_STATE_HPP_

#include "tip/db/pg/detail/basic_state.hpp"

namespace tip {
namespace db {
namespace pg {
namespace detail {

class idle_state: public tip::db::pg::detail::basic_state {
public:
	idle_state(connection_base& conn);
	virtual ~idle_state() {}
private:
	virtual std::string const
	get_name() const;

	virtual connection::state_type
	get_state() const
	{ return connection::IDLE; }

	virtual bool
	do_handle_message(message_ptr);

	virtual void
	do_handle_unlocked();

	virtual void
	do_begin_transaction(simple_callback const&, error_callback const&,
			bool autocommit);

	virtual void
	do_execute_query(std::string const& q, result_callback const& cb,
			query_error_callback const& err);

	virtual void
	do_execute_prepared(std::string const& q, buffer_type const& params,
			result_callback const&, query_error_callback const&);

	virtual void
	do_terminate(simple_callback const&);
};

} /* namespace detail */
} /* namespace pg */
} /* namespace db */
} /* namespace tip */

#endif /* TIP_DB_PG_DETAIL_IDLE_STATE_HPP_ */
