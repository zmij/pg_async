/*
 * query.cpp
 *
 *  Created on: 11 июля 2015 г.
 *     @author: zmij
 */

#include <tip/db/pg/query.hpp>
#include <tip/db/pg/resultset.hpp>
#include <tip/db/pg/database.hpp>
#include <tip/db/pg/transaction.hpp>

#include <tip/db/pg/log.hpp>

#include <functional>

namespace tip {
namespace db {
namespace pg {

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

struct query::impl : std::enable_shared_from_this<query::impl> {
	dbalias alias_;
	transaction_ptr conn_;
	std::string expression_;

	bool start_tran_;
	bool autocommit_;

	type_oid_sequence param_types_;
	params_buffer params_;

	impl(dbalias const& alias, std::string const& expression,
			bool start_tran, bool autocommit)
		: alias_(alias), conn_(), expression_(expression),
		  start_tran_(start_tran), autocommit_(autocommit)
	{
	}

	impl(transaction_ptr conn, std::string const& expression)
		: alias_{}, conn_(conn), expression_(expression),
		  start_tran_(false), autocommit_(false)
	{
	}

	void
	clear_params()
	{
		params_buffer params;
		protocol_write<BINARY_DATA_FORMAT>(params, (smallint)0); // format codes
		protocol_write<BINARY_DATA_FORMAT>(params, (smallint)0); // number of parameters
	}

	void
	run_async(query_result_callback const& res, error_callback const& err)
	{
		if (!conn_) {
			db_service::begin(
				alias_,
				std::bind(&impl::handle_get_transaction,
						shared_from_this(), std::placeholders::_1, res, err),
				std::bind(&impl::handle_get_connection_error,
						shared_from_this(), std::placeholders::_1, err)
			);
		} else {
			handle_get_transaction(conn_, res, err);
		}
	}
//	void
//	handle_get_connection(transaction_ptr c,
//			query_result_callback const& res,
//			error_callback const& err)
//	{
//		if (start_tran_) {
//			(*c)->begin_transaction(
//				std::bind(&impl::handle_get_transaction,
//						shared_from_this(), std::placeholders::_1, res, err),
//				std::bind(&impl::handle_get_connection_error,
//						shared_from_this(), std::placeholders::_1, err),
//				autocommit_
//			);
//		} else {
//			handle_get_transaction(c, res, err);
//		}
//	}

	void
	handle_get_transaction(transaction_ptr c,
			query_result_callback const& res,
			error_callback const& err)
	{
		using namespace std::placeholders;
		if (params_.empty()) {
			{
				local_log() << "Execute query "
						<< (util::MAGENTA | util::BRIGHT)
						<< expression_
						<< logger::severity_color();
			}
			conn_ = c;
//			(*conn_)->execute_query(expression_,
//				std::bind(&impl::handle_get_results,
//						shared_from_this(),
//						std::placeholders::_1,
//						std::placeholders::_2,
//						std::placeholders::_3,
//						res),
//				err, c);
		} else {
			{
				local_log() << "Execute prepared query "
						<< (util::MAGENTA | util::BRIGHT)
						<< expression_
						<< logger::severity_color();
			}
			conn_ = c;
//			(*conn_)->execute_prepared(expression_, param_types_, params_,
//				std::bind(&impl::handle_get_results,
//						shared_from_this(),
//						std::placeholders::_1,
//						std::placeholders::_2,
//						std::placeholders::_3,
//						res),
//				err, c);
		}
	}

	void
	handle_get_connection_error(db_error const& ec, error_callback const& err)
	{
		err(ec);
	}

	void
	handle_get_results(transaction_ptr c, resultset r, bool complete,
			query_result_callback const& res)
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
query::query(transaction_ptr c, std::string const& expression)
	: pimpl_(new impl(c, expression))
{
}

query&
query::bind()
{
	pimpl_->clear_params();
	return *this;
}

void
query::run_async(query_result_callback const& res, error_callback const& err)
{
	pimpl_->run_async(res, err);
}

void
query::operator ()(query_result_callback const& res, error_callback const& err)
{
	run_async(res, err);
}

transaction_ptr
query::connection()
{
	return pimpl_->conn_;
}

void
query::create_impl(dbalias const& alias, std::string const& expression,
		bool start_tran, bool autocommit)
{
	pimpl_.reset(new impl(alias, expression, start_tran, autocommit));
}

void
query::create_impl(transaction_ptr c, std::string const& expression)
{
	pimpl_.reset(new impl(c, expression));
}

query::params_buffer&
query::buffer()
{
	return pimpl_->params_;
}

type_oid_sequence&
query::param_types()
{
	return pimpl_->param_types_;
}

}  // namespace pg
}  // namespace db
}  // namespace tip


