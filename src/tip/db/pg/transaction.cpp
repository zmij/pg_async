/*
 * connection_lock.cpp
 *
 *  Created on: 14 июля 2015 г.
 *     @author: zmij
 */

#include <tip/db/pg/connection.hpp>
#include <tip/db/pg/transaction.hpp>

namespace tip {
namespace db {
namespace pg {

transaction::transaction(connection_ptr conn,release_func rel)
	: connection_(conn), release_(rel)
{
}

transaction::~transaction()
{
	if (release_)
		release_();
}


} /* namespace pg */
} /* namespace db */
} /* namespace tip */
