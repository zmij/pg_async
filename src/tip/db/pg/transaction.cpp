/*
 * connection_lock.cpp
 *
 *  Created on: 14 июля 2015 г.
 *     @author: zmij
 */

#include <tip/db/pg/transaction.hpp>
#include <tip/db/pg/detail/basic_connection.hpp>

namespace tip {
namespace db {
namespace pg {

transaction::transaction(connection_ptr conn)
	: connection_(conn)
{
}

transaction::~transaction()
{
	if (connection_->in_transaction())
		connection_->rollback();
}

bool
transaction::in_transaction() const
{
	return connection_->in_transaction();
}

void
transaction::commit()
{

	connection_->commit();
}

void
transaction::rollback()
{
	connection_->rollback();
}

} /* namespace pg */
} /* namespace db */
} /* namespace tip */
