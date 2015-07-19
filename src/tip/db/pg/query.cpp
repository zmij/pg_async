/*
 * query.cpp
 *
 *  Created on: 11 июля 2015 г.
 *     @author: zmij
 */

#include <tip/db/pg/query.hpp>
#include <tip/db/pg/connection.hpp>
#include <tip/db/pg/resultset.hpp>
#include <tip/db/pg/database.hpp>
#include <tip/db/pg/detail/connection_lock.hpp>

#include <tip/db/pg/log.hpp>

#include <functional>

namespace tip {
namespace db {
namespace pg {

#ifdef WITH_TIP_LOG
namespace {
/** Local logging facility */
using namespace tip::log;

const std::string LOG_CATEGORY = "PGQUERY";
logger::event_severity DEFAULT_SEVERITY = logger::DEBUG;
local
local_log(logger::event_severity s = DEFAULT_SEVERITY)
{
	return local(LOG_CATEGORY, s);
}

}  // namespace
// For more convenient changing severity, eg local_log(logger::WARNING)
using tip::log::logger;
#endif

struct query::impl : std::enable_shared_from_this<query::impl> {
	dbalias alias_;
	connection_lock_ptr conn_;
	std::string expression_;

	bool start_tran_;
	bool autocommit_;

	impl(dbalias const& alias, std::string const& expression,
			bool start_tran, bool autocommit)
		: alias_(alias), conn_(), expression_(expression),
		  start_tran_(start_tran), autocommit_(autocommit)
	{
	}

	impl(connection_lock_ptr conn, std::string const& expression)
		: alias_{}, conn_(conn), expression_(expression),
		  start_tran_(false), autocommit_(false)
	{
	}

	void
	run_async(query_result_callback res, error_callback err)
	{
		if (!conn_) {
			db_service::get_connection_async(
				alias_,
				std::bind(&impl::handle_get_connection,
						shared_from_this(), std::placeholders::_1, res, err),
				std::bind(&impl::handle_get_connection_error,
						shared_from_this(), std::placeholders::_1, err)
			);
		} else {
			handle_get_transaction(conn_, res, err);
		}
	}
	void
	handle_get_connection(connection_lock_ptr c,
			query_result_callback res,
			error_callback err)
	{
		if (start_tran_) {
			(*c)->begin_transaction(
				std::bind(&impl::handle_get_transaction,
						shared_from_this(), std::placeholders::_1, res, err),
				std::bind(&impl::handle_get_connection_error,
						shared_from_this(), std::placeholders::_1, err),
				autocommit_
			);
		} else {
			handle_get_transaction(c, res, err);
		}
	}

	void
	handle_get_transaction(connection_lock_ptr c,
			query_result_callback res,
			error_callback err)
	{
		using namespace std::placeholders;
		#ifdef WITH_TIP_LOG
		{
			local_log() << "Execute query "
					<< (util::MAGENTA | util::BRIGHT)
					<< expression_
					<< logger::severity_color();
		}
		#endif
		conn_ = c;
		(*conn_)->execute_query(expression_,
			std::bind(&impl::handle_get_results,
					shared_from_this(),
					std::placeholders::_1,
					std::placeholders::_2,
					std::placeholders::_3,
					res),
			err, c);
	}

	void
	handle_get_connection_error(db_error const& ec, error_callback err)
	{
		err(ec);
	}

	void
	handle_get_results(connection_lock_ptr c, resultset r, bool complete, query_result_callback res)
	{
		conn_ = c;
		res(c, r, complete);
	}
};

query::query(dbalias const& alias, std::string const& expression,
		bool start_tran, bool autocommit)
	: pimpl_(new impl(alias, expression, start_tran, autocommit))
{
}
query::query(connection_lock_ptr c, std::string const& expression)
	: pimpl_(new impl(c, expression))
{
}

void
query::run_async(query_result_callback res, error_callback err)
{
	pimpl_->run_async(res, err);
}

void
query::operator ()(query_result_callback res, error_callback err)
{
	run_async(res, err);
}

connection_lock_ptr
query::connection()
{
	return pimpl_->conn_;
}

}  // namespace pg
}  // namespace db
}  // namespace tip


