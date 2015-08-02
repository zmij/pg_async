/**
 *  @file tip/db/pg.hpp
 *
 *  @date Jul 16, 2015
 *  @author zmij
 */

/**
 *  @mainpage
 *
 *  ## Motivation
 *
 *  When developing software we deal a lot with external network resources.
 *  Most of them can be accessed through an asynchronous interface, so we don't
 *  have to block to wait for a request to finish. The lack of modern C++
 *  asynchronous interface was the reason for writing this library.
 *
 *  ## Overview
 *
 *  pg_async is an unofficial PostreSQL database asynchronous client library
 *  written in modern C++ (std=c++11) on top of boost::asio library used for
 *  asynchronous network input-output.
 *
 *  ### Features
 *
 *  * Asynchronous operations
 *  * Connection pooling
 *  * Database aliases
 *  * Standard container-compliant resultset interface
 *  * Execution of prepared statements
 *  * Multiple result sets for simple query mode
 *  * Data row extraction to tuples
 *  * Flexible datatype conversion
 *  * Compile-time statement parameter types binding and checking
 *  * Extensible datatypes input-output system
 *  * TCP or UNIX socket connection
 *
 *	### Short usage overview
 *
 *	#### Registering a connection
 *
 *	@code
 *	#include <tip/db/pg.hpp>
 *
 *	using namespace tip::db::pg;
 *	db_service::add_connection("main=tcp://user:pass@the-database-server:5432[databasename]");
 *	db_service::add_connection("logs=tcp://user:pass@the-database-server:5432[logs_db]");
 *	@endcode
 *
 *  @see @ref connstring
 *	@see tip::db::pg::db_service reference
 *
 *	#### Transactions
 *
 *	@code
 *	db_service::begin(
 *		"main"_db, // Database alias
 *		// Asynchronous callback, will be called as soon as a connection becomes
 *		// idle, and a new transaction will be started in it.
 *		[] (transaction_ptr tran)
 *		{
 *			// Run the queries here
 *			// A transaction MUST be explicitly commited or it will be rolled back
 *			tran->commit();
 *		},
 *		// Transaction error handler.
 *		[](db_error const& error)
 *		{
 *		}
 *	);
 *	@endcode
 *
 *	@see @ref transactions
 *  @see @ref connstring
 *	@see tip::db::pg::transaction for reference
 *
 *	#### Querying
 *
 *  @code
 *  #include <tip/db/pg.hpp>
 *
 *  query("main"_db, "select * from pg_catalog.pg_types").run_async(
 *  [&](transaction_ptr t, resultset res, bool complete) {
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
 *  #### Processing query results
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

/**
 *  @page transactions Transactions
 *
 *  All the work with the database is wrapped into transactions. A transaction
 *  MUST be explicitly committed or it well be rolled back when the transaction
 *  handle object goes out of scope.
 *
 *  If a transaction is rolled back, the error callback is called in any case,
 *  either the transaction was rolled back explicitly by the user or if it was
 *  rolled back as the result of abandoned transaction object.
 *
 *  @code
 *  #include <tip/db/pg.hpp>
 *
 *	using namespace tip::db::pg;
 *  // Start a transaction explicitly
 *  db_service::begin(
 *  	// Database alias
 *  	"main"_db,
 *  	// Function to be called in the transaction
 *  	[]( transaction_ptr tran )
 *  	{
 *  		// run queries
 *  		tran->commit();
 *  	},
 *  	// Transaction error handler
 *  	[](db_error const& e)
 *  	{
 *  	}
 *  );
 *
 *  // Start a transaction implicitly
 *  query("main"_db, "sql statement")(
 *  [](transaction_ptr tran, resultset res, bool complete)
 *  {
 *  	// Process results, run more queries
 *  	tran->commit();
 *  },
 *  // This handler will receive both the query and the transaction errors
 *  [](db_error const&)
 *  {
 *  });
 *  @endcode
 *
 *	@see tip::db::pg::transaction
 *	@see @ref errors
 */

/**
 *	@page connstring Connection string and database alias
 *
 *	### Connection string
 *
 *	A connection string looks like follows:
 *	`alias=schema://user:password@url[database]`, where:
 *	* alias is a database alias, a unique string used to refer the database
 *	* schema is a connection schema. Currently supported is tcp and socket
 *	* user is a database user name
 *	* password is the users's password
 *	* url for tcp is in form host:port, for unix socket - file path
 *	* database is a database name
 *
 *	@see tip::db::pg::db_service
 *	@see tip::db::pg::connection_options
 *
 *	### Database alias
 *
 *	A short unique string that can be used to refer a database. It is passed
 *	as a part of a connection string when registering a connection and is
 *	used later to retrieve a connection to a database via the
 *	tip::db::pg::db_service interface or the tip::db::pg::query interface.
 *
 *	@see tip::db::pg::dbalias
 *	@see tip::db::pg::db_service
 *	@see tip::db::pg::query
 */

/**
 *  @page querying Running queries
 *
 *  There are two query modes: simple query mode and extended query mode.
 *  Simple query mode executes arbitrary parameterless SQL scripts. Extended
 *  query mode prepares a query and executes the query with (or without)
 *  parameters.
 *
 *  ## Simple queries
 *
 *  A simple query is an arbitrary script. It can have several statements,
 *  each statement will generate a resultset callback.
 *
 *	### Running a single query with implicit transaction start
 *
 *	When a single query needs to be made, it might be easier not to wrap it
 *	into a `begin` call, but to pass the query the alias of the database to
 *	use. Don't forget to commit the transaction after handling the result.
 *
 *  @code
 *	query( "main"_db,		// Database alias
 *	[](transaction_ptr tran, resultset res, bool complete)
 *	{
 *		// Process the result
 *		tran->commit();
 *	},
 *	[](db_error const& error)
 *	{
 *	});
 *  @endcode
 *
 *	### Running several independent queries in one transaction
 *
 *	Independent queries (i.e. queries that do not depend on each others'
 *	results can be enqueued in a single begin callback together with the
 *	call to `commit`. The queries will be run one by one in a single
 *	transaction, their results callbacks will be called as they will be
 *	completed. After all statements finish, the transaction will be committed.
 *
 *	@warning When the transaction `commit` or `rollback` is enqueued, no other
 *	queries can be started, neither from the block containing the commit call,
 *	nor from the queries' results callbacks.
 *
 *	@code
 *	db_service::begin("main"_db,	// Database alias
 *	[](transaction_ptr tran) {
 *		query(tran, "statement 1")(
 *		[](transaction_ptr, resultset r1, bool) {
 *			// Result 1
 *		},
 *		[](db_error const&) {
 *		});
 *		query(tran, "statement 2")(
 *		[](transaction_ptr, resultset r1, bool) {
 *			// Result 2
 *		},
 *		[](db_error const&) {
 *		});
 *		query(tran, "statement 3")(
 *		[](transaction_ptr, resultset r1, bool) {
 *			// Result 3
 *		},
 *		[](db_error const&) {
 *		});
 *		query(tran, "statement 4")(
 *		[](transaction_ptr, resultset r1, bool) {
 *			// Result 4
 *		},
 *		[](db_error const&) {
 *		});
 *		// Yes, the commit statement can be issued here.
 *		tran->commit();
 *	},
 *	[](db_error const&) {
 *	});
 *	@endcode
 *
 *	@see @ref errors
 *
 *	###  Running a query dependent on the results of another one
 *
 *	@code
 *	query("main"_db, "statement")(
 *	[](transaction_ptr tran, resultset res, bool complete)
 *	{
 *		for (auto row : res) {
 *			// Run a prepared statement with a parameter
 *			query(tran, "statement $1", row["id"].as<bigint>())(
 *			[](transaction_ptr t, resultset res, bool complete) {
 *				// Dependent result
 *			},
 *			[](db_error const&){
 *			});
 *		}
 *		// All queries has been enqueued, can commit here
 *		tran->commit();
 *	},
 *	[](db_error const&)
 *	{
 *	});
 *	@endcode
 *
 *  ## Prepared statements (extended query mode)
 *
 *  Prepared statements is an efficient way to run repeated queries (the most
 *  frequent situation). `pg_async` uses PostgreSQL extended query mode to
 *  parse the statement, bind parameters and execute the query.
 *
 *  @code
 *  // Full syntax
 *  query("main"_db,	// Database alias
 *  	"select * from some_table where amount > $1 and price < $2",	// Statement with placeholders
 *  ).bind(
 *  	100, 500		// Query parameters
 *  )(
 *  [](transaction_ptr tran, resultset res, bool complete) {
 *  },
 *  [](db_error const&)
 *  {
 *  });
 *  // Short syntax
 *  query("main"_db,	// Database alias
 *  	"select * from some_table where amount > $1 and price < $2",	// Statement with placeholders
 *  	100, 500		// Query parameters
 *  )(
 *  [](transaction_ptr tran, resultset res, bool complete) {
 *  },
 *  [](db_error const&)
 *  {
 *  });
 *  // Query without parameters
 *	query("main"_db,	// Database alias
 *		"select * from some_table"	// Statement without parameters
 *	).bind()(			// Call the bind method to mark the query for preparing
 *		// Handle results and errors
 *	);
 *  @endcode
 *
 *	@see tip::db::pg::query
 *	@see @ref transactions
 *	@see @ref results
 *	@see @ref errors
 *	@see @ref callbacks
 */

/**
 *  @page results Processing query results
 *
 *  Result set (@ref tip::db::pg::resultset) is passed to client via supplied
 *  callback when a query receives results from the server.
 *  A @ref tip::db::pg::resultset is an object that provides an interface
 *  to reading and converting underlying data buffers. It is lightweight
 *  and is designed to be copied around by value. It can be stored in memory
 *  for later use.
 *
 *  A resultset object mimics standard container interface for the data rows.
 *  It provides random access to data rows via indexing operator and random
 *  access iterators.
 *
 *  Result set provides interface for reading field definitions.
 *
 *	### Checking the resultset
 *
 *	@code
 *	if (res.size() > 0) {
 *		// Process the result set
 *	}
 *	if (!result.empty()) {
 *		// Process the result set
 *	}
 *	if (res) {
 *		// Process the result set
 *	}
 *	@endcode
 *
 *	@todo document iterating row and fields
 *	@todo document random access to rows in a resultset
 *	@todo document field descriptions
 *	@todo document row tie variadic interface
 *	@todo document access by index and by name to fields in a row
 *	@todo document field buffer conversion to other datatypes
 *
 *	@todo references to data parsers. documentation on data parsing and adding
 *		datatype support.
 *
 *	@todo concept of an interface for a datatype that can be stored/retrieved
 *		from the database. Template functions enabled for such an interface.
 *	@todo non-select command results
 *
 *	@see @ref querying
 *	@see tip::db::pg::resultset
 *	@see tip::db::pg::query
 *  @see tip::db::pg::field_definition
 */

/**
 * @page callbacks Callback types
 */

/**
 * @page errors Errors and exceptions
 */

/**
 * @page threads Threads and thread safety
 */

#ifndef TIP_DB_PG_HPP_
#define TIP_DB_PG_HPP_

#include <tip/db/pg/database.hpp>
#include <tip/db/pg/transaction.hpp>
#include <tip/db/pg/query.hpp>
#include <tip/db/pg/resultset.hpp>
#include <tip/db/pg/error.hpp>

#endif /* TIP_DB_PG_HPP_ */
