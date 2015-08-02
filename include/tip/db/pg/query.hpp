/**
 * @file tip/db/pg/query.hpp
 *
 *    @date Jul 11, 2015
 *  @author zmij
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
	query(dbalias const&, std::string const& expression);
	/**
	 * Construct a prepared query with params to bind
	 * @param
	 * @param expression
	 * @param start_tran
	 * @param autocommit
	 * @param params
	 */
	template < typename ... T >
	query(dbalias const&, std::string const& expression, T const& ... params);
	/**
	 * Construct a query.
	 * @param connection
	 * @param expression
	 */
	query(transaction_ptr, std::string const& expression);
	/**
	 * Construct a prepared query with params to bind
	 * @param
	 * @param expression
	 * @param params
	 */
	template < typename ... T >
	query(transaction_ptr, std::string const& expression,
			T const& ... params);

	/**
	 * Bind parameters for the query
	 * @param params
	 */
	template < typename ... T >
	query&
	bind(T const& ... params);

	query&
	bind();
	/**
	 * Run a query in a database identified by the alias asynchronously.
	 * @pre Database alias must be registered with the @c database.
	 * @param result result callback
	 * @param error error callback
	 */
	void
	run_async(query_result_callback const&, error_callback const&);
	/**
	 * @see run_async(result_callback, error_callback)
	 * @param
	 * @param
	 */
	void
	operator()(query_result_callback const&, error_callback const&);

	transaction_ptr
	connection();
private:
	typedef std::vector<byte> params_buffer;

	void
	create_impl(dbalias const&, std::string const& expression);
	void
	create_impl(transaction_ptr, std::string const& expression);
	type_oid_sequence&
	param_types();
	params_buffer&
	buffer();
	struct impl;
	typedef std::shared_ptr<impl> pimpl;
	pimpl pimpl_;
};

}  // namespace pg
}  // namespace db
}  // namespace tip

#include <tip/db/pg/query.inl>

#endif /* TIP_DB_PG_QUERY_HPP_ */
