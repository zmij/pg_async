/*
 * connection_lock.cpp
 *
 *  Created on: 14 июля 2015 г.
 *     @author: zmij
 */

#include <tip/db/pg/transaction.hpp>
#include <tip/db/pg/detail/basic_connection.hpp>
#include <tip/db/pg/log.hpp>

namespace tip {
namespace db {
namespace pg {

namespace {
/** Local logging facility */
using namespace tip::log;

const std::string LOG_CATEGORY = "PGTRAN";
logger::event_severity DEFAULT_SEVERITY = logger::TRACE;
local
local_log(logger::event_severity s = DEFAULT_SEVERITY)
{
	return local(LOG_CATEGORY, s);
}

}  // namespace
// For more convenient changing severity, eg local_log(logger::WARNING)
using tip::log::logger;


transaction::transaction(connection_ptr conn)
	: connection_(conn)
{
}

transaction::~transaction()
{
	local_log() << (util::MAGENTA | util::BRIGHT) << "transaction::~transaction()";
	if (connection_->in_transaction()) {
		local_log(logger::WARNING) << "Transaction object abandoned, rolling back";
		connection_->rollback();
	}
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
