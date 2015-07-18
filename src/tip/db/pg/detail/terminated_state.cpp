/*
 * terminated_state.cpp
 *
 *  Created on: Jul 13, 2015
 *      Author: zmij
 */

#include <tip/db/pg/detail/terminated_state.hpp>
#include <tip/db/pg/detail/basic_connection.hpp>

namespace tip {
namespace db {
namespace pg {
namespace detail {

terminated_state::terminated_state(connection_base& conn)
	: basic_state(conn)
{
}

void
terminated_state::do_enter()
{
	conn.notify_terminated();
}

} /* namespace detail */
} /* namespace pg */
} /* namespace db */
} /* namespace tip */
