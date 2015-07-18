/*
 * connection_lock.cpp
 *
 *  Created on: 14 июля 2015 г.
 *     @author: zmij
 */

#include <tip/db/pg/connection.hpp>
#include <tip/db/pg/detail/connection_lock.hpp>

namespace tip {
namespace db {
namespace pg {
namespace detail {

connection_lock::connection_lock(connection_ptr conn,release_func rel)
	: connection_(conn), release_(rel)
{
}

connection_lock::~connection_lock()
{
	if (release_)
		release_();
}


} /* namespace detail */
} /* namespace pg */
} /* namespace db */
} /* namespace tip */
