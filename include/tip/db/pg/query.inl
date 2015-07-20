/*
 * query.inl
 *
 *  Created on: Jul 20, 2015
 *      Author: zmij
 */

#ifndef LIB_PG_ASYNC_INCLUDE_TIP_DB_PG_QUERY_INL_
#define LIB_PG_ASYNC_INCLUDE_TIP_DB_PG_QUERY_INL_

#include <tip/db/pg/query.inl>

namespace tip {
namespace db {
namespace pg {

template < typename ... T >
query::query(dbalias const& alias, std::string const& expression,
		bool start_tran, bool autocommit, T ... params)
{
	create_impl(alias, expression, start_tran, autocommit);
	bind_params(params ...);
}

template < typename ... T >
query::query(connection_lock_ptr c, std::string const& expression,
		T ... params)
{
	create_impl(c, expression);
	bind_params(params ...);
}

template < typename ... T >
void
query::bind_params(T ... params)
{
	// 1. Write format codes
	// 	- detect if all of param types have binary formatters
	//	- write a binary format code (1) if all have binary formatters
	//	- write format codes for each param
	//	- evaluate buffer length
	// 2. Params
	//  - write the number of params
	//  - write each param preceded by it's length
	// 3. Result columns
	//	- leave 0 by now.
}

}  // namespace pg
}  // namespace db
}  // namespace tip

#endif /* LIB_PG_ASYNC_INCLUDE_TIP_DB_PG_QUERY_INL_ */
