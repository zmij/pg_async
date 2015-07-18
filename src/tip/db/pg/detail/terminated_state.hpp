/*
 * terminated_state.hpp
 *
 *  Created on: Jul 13, 2015
 *      Author: zmij
 */

#ifndef TIP_DB_PG_DETAIL_TERMINATED_STATE_HPP_
#define TIP_DB_PG_DETAIL_TERMINATED_STATE_HPP_

#include <tip/db/pg/detail/basic_state.hpp>

namespace tip {
namespace db {
namespace pg {
namespace detail {

class terminated_state: public basic_state {
public:
	terminated_state(connection_base& conn);
	virtual ~terminated_state() {}
private:
	virtual void
	do_enter();

	virtual bool
	do_handle_message(message_ptr)
	{ return false; }

	virtual std::string const
	get_name() const
	{ return "terminated"; }

	virtual connection::state_type
	get_state() const
	{ return connection::DISCONNECTED; }
};

} /* namespace detail */
} /* namespace pg */
} /* namespace db */
} /* namespace tip */

#endif /* TIP_DB_PG_DETAIL_TERMINATED_STATE_HPP_ */
