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

transaction::transaction(connection_ptr conn)
	: connection_(conn)
{
}

transaction::~transaction()
{
}

bool
transaction::in_transaction() const
{
	//return connection_->in_transaction();
	return true;
}

void
transaction::commit()
{

	//connection_->commit_transaction( shared_from_this(), tcb, err );
}

void
transaction::rollback()
{

	//connection_->rollback_transaction( shared_from_this(), tcb, err );
}

} /* namespace pg */
} /* namespace db */
} /* namespace tip */
