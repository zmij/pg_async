/*
 * query.hpp
 *
 *  Created on: 11 июля 2015 г.
 *      Author: brysin
 */

#ifndef TIP_DB_PG_QUERY_HPP_
#define TIP_DB_PG_QUERY_HPP_

#include <tip/db/pg/common.hpp>

#include <memory>
#include <functional>
#include <boost/system/error_code.hpp>

namespace tip {
namespace db {
namespace pg {

/**
 * Asynchronous query class.
 * Synopsis:
 * @code
 * query q(alias, "select * from pg_catalog.pg_tables");
 * q(
 * 	[](result_ptr res, bool complete)
 * 	{
 * 		// process the query results
 * 	},
 * 	[](boost::system::error_code ec)
 * 	{
 * 		// handle the error here
 * );
 * @endcode
 */
class query {
public:
	/**
	 * Construct a query.
	 * @param alias Database connection alias.
	 * @param expression SQL query expression
	 */
	query(dbalias const&, std::string const& expression,
			bool start_tran = false, bool autocommit = false);
	/**
	 * Construct a query.
	 * @param connection
	 * @param expression
	 */
	query(connection_lock_ptr, std::string const& expression);
	/**
	 * Run a query in a database identified by the alias asynchronously.
	 * @pre Database alias must be registered with the @c database.
	 * @param result result callback
	 * @param error error callback
	 */
	void
	run_async(query_result_callback, error_callback);
	/**
	 * @see run_async(result_callback, error_callback)
	 * @param
	 * @param
	 */
	void
	operator()(query_result_callback, error_callback);

	connection_lock_ptr
	connection();
private:

	struct impl;
	typedef std::shared_ptr<impl> pimpl;
	pimpl pimpl_;
};

}  // namespace pg
}  // namespace db
}  // namespace tip

#endif /* TIP_DB_PG_QUERY_HPP_ */
