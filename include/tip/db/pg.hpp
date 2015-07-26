/**
 *  @file tip/db/pg.hpp
 *
 *  @date Jul 16, 2015
 *  @author zmij
 */

/**
 *  @mainpage
 *
 *  # Overview
 *
 *  pg_async is a PostgreSQL database server asynchronous client library
 *  written in modern C++ (std=c++11) on top of boost::asio library
 *  for asynchronous network input-output.
 *
 *  It uses several connections to the same database in a connection pool
 *  to carry out concurrent queries. A connection must be registered with
 *  the database service, and a run() function must be called for the
 *  asynchronous operations to be processed. The run() function blocks
 *  the thread it is called in. All the query operations are carried out
 *  in the threads that call run() static method of db_service.
 *
 *  @todo Features
 *  @todo Description of connection transports
 *
 *	## Short usage overview
 *
 *	### Registering a connection
 *
 *	@code
 *	#include <tip/db/pg.hpp>
 *
 *	using namespace tip::db::pg;
 *	db_service::add_connection("main=tcp://user:pass@the-database-server:5432[databasename]");
 *	@endcode
 *
 *	@see tip::db::pg::db_service for more information
 *
 *	### Querying
 *
 *  @code
 *  #include <tip/db/pg.hpp>
 *
 *  query("main"_db, "select * from pg_catalog.pg_types").run_async(
 *  [&](connection_lock_ptr c, resultset res, bool complete) {
 *  	// Process the query results
 *  },
 *  [&](db_error const& err) {
 *  	// Handle query error
 *  });
 *  @endcode
 *
 *  @see @ref querying for more information
 *  @see tip::db::pg::query for reference
 *
 *  ### Processing query results
 *
 *	@code
 *	resultset res; // Passed to a callback from query
 *	if (res) {
 *		for (auto row : res) { // Row iteration
 *			for (auto field : row) {
 *				std::string value = field.coalesce("default value");
 *			}
 *		}
 *	}
 *	@endcode
 *
 *  @see @ref results for more information
 *	@see tip::db::pg::resultset for reference
 *	@see tip::db::pg::resultset::row for reference
 *	@see tip::db::pg::resultset::field for reference
 */

#ifndef TIP_DB_PG_HPP_
#define TIP_DB_PG_HPP_

#include <tip/db/pg/database.hpp>
#include <tip/db/pg/query.hpp>
#include <tip/db/pg/resultset.hpp>
#include <tip/db/pg/error.hpp>

#endif /* TIP_DB_PG_HPP_ */
