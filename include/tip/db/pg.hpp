/**
 *  @file tip/db/pg.hpp
 *
 *  @date Jul 16, 2015
 *  @author zmij
 */

/**
 *  @mainpage
 *
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
 *  * Query queuing
 *  * Multiple result sets for simple query mode
 *  * Data row extraction to tuples
 *  * Flexible datatype conversion
 *  * Compile-time statement parameter types binding and checking
 *  * Extensible datatypes input-output system
 *  * TCP or UNIX socket connection
 *
 *	### Short usage overview
 *
 *	All classes are located in namespace @ref tip::db::pg, if not specified
 *	other way.
 *
 *	#### Registering a connection
 *
 *	@code
 *	#include <tip/db/pg.hpp>
 *
 *	using namespace tip::db::pg;
 *	db_service::add_connection("main=tcp://user:pass@the-database-server:5432[databasename]");
 *	db_service::add_connection("logs=tcp://user:pass@the-database-server:5432[logs_db]");
 *
 *	db_service::run();
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
 *  @see @ref threads
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
 *  @see @ref errors
 *  @see @ref callbacks
 *	@see @ref valueio
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
 *	@see @ref valueio
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
 *  @see @ref results
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
 *	@see @ref valueio
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
 *  A resultset row mimics standard container interface for the fields.
 *  It provides index operators for accessing fields by their index or by
 *  their name. It also provides random access iterators for iterating the
 *  fields.
 *
 *  Result set provides interface for reading field definitions.
 *
 *	### Checking the resultset
 *
 *	@code
 *	resultset res;
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
 *	### Accessing rows and fields
 *
 *	@code
 *	resultset res;
 *	for (auto row : res) {
 *		for (auto field : row) {
 *			// Do something with the field value
 *		}
 *	}
 *
 *	auto first_row = res.front();
 *	auto last_row = res.back();
 *	auto mid_row = res[ res.size() / 2 ]; // Index access
 *
 *	auto first_field = first_row[0];      // Access field by index
 *	auto id_field = last_row["id"];       // Access field by name
 *
 *	@endcode
 *
 *	### Field descriptions
 *
 *	@code
 *	resultset res;
 *	// Get a reference to field descriptions
 *	row_description_type const& rd = res.row_description();
 *	@endcode
 *
 *	@see tip::db::pg::field_description for reference
 *
 *	### Accessing field data
 *
 *  Field values are extracted via the system of buffer parsers. Default
 *  format for PosgreSQL protocol is text, so most if a data type is
 *  extractable from a stream, it can be extracted from a text data buffer.
 *  If a special parsing routine is required, a text data parser must be
 *  provided. All datatypes can be extracted as strings.
 *
 *  For binary data format a data parser must be provided.
 *
 *  An exception tip::db::pg::value_is_null will be thrown if the field is null.
 *  A boost::optional can be used as a nullable datatype, the value will be
 *  cleared, and no exception will be thrown. A `coalesce` function can be used
 *  to get a default value if the field is null.
 *
 *	@code
 *	field f = row[0];
 *	std::string s = f.as<std::string>();
 *	f.to(s); // template parameter deduced from the paramter
 *	int i = f.as<int>();
 *	f.to(i);
 *
 *	if (f.is_null()) {
 *		boost::optional<int> nullable = f.as< boost::optional<int> >();
 *		int val = f.coalesce(100500);
 *	}
 *	@endcode
 *
 *	@see @ref valueio
 *
 *	#### Using tuples to extract row data
 *
 *	A std::tuple can be used to extract data from row fields
 *
 *	@code
 *
 *	struct data {
 *		int id;
 *		std::string name;
 *	};
 *
 *	query(alias, "select id, name from some_table")
 *	([](transaction_ptr t, resultset res, bool complete) {
 *		for (auto row : res) {
 *			data d;
 *			row.to(std::tie(d.id, d.name));
 *		}
 *	},
 *	[](error::db_error const&) {
 *	});
 *
 *	@endcode
 *
 *	@see @ref valueio
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
 * 	@page callbacks Callback types
 *
 *	There are transaction-level callbacks and query-level callbacks.
 *
 *	### Transaction Callbacks
 *
 *	Transaction callbacks are passed to tip::db::pg::db_service::begin function.
 *
 *	* tip::db::pg::transaction_callback is called when a connection
 *		becomes available in the connection pool and a transaction starts.
 *	* tip::db::pg::error_callback is called when a transaction
 *		cannot start for some reason or an exception happens inside the
 *		transaction. Also called if a transaction is rolled back.
 *
 *	@see @ref errors
 *
 *	@todo Commit callback
 *
 *	### Query callbacks
 *
 *	Query callbacks are passed to the tip::db::pg::query::run_async function
 *	( or it's shortcut variant - tip::db::pg::query::operator() ).
 *
 *	* tip::db::pg::query_result_callback is called when query result data
 *			becomes available. It can be called multiple times for a single
 *			resultset if the data volume is big. When the query completes,
 *			the complete flag passed is true.
 *			If an exception is thrown in the callback, all the queries that
 *			are queued after the one running are cancelled and the transaction
 *			is rolled back.
 *	* tip::db::pg::error_callback is called when PostgreSQL server sends an
 *			error in responce to a query.
 *
 *	@see @ref errors
 */

/**
 *	@page errors Errors and exceptions
 *
 *	Due to asynchronous nature of the library, there are few places where an
 *	exception can be thrown. Most of error are passed to callbacks.
 *
 *	Places where an exception can be thrown:
 *	* tip::db::pg::db_service::add_connection - tip::db::pg::error::connection_error,
 *		when the connection string is invalid or lacks some information
 *	* tip::db::pg::db_service::begin - tip::db::pg::error::connection_error, when
 *		the database alias is not configured.
 *	* when trying to extract a value from a null field -
 *		tip::db::pg::error::value_is_null
 *
 *	The rest of errors are passed via callbacks. If an error occurs inside
 *	a transaction, it is rolled back. If the error is an error sent by the server,
 *	it is passed to the query error callback. If an exception is thrown by
 *	any callback, it is passed to the transaction error callback. If the
 *	transaction error callback throws an error, it is silently caught and
 *	suppressed.
 */

/**
 *	@page threads Threads and thread safety
 *
 * 	pg_async runs all the requests to the database servers in the threads that
 * 	call db_service::run. db_service static methods are thread safe and can be
 * 	called from any thread. All requests and their callbacks in a single
 * 	transaction are guaranteed to run in a single thread.
 *
 * 	@warning A tip::db::pg::transaction object shoudn't be stored and accessed
 * 	concurrently.
 */

/**
 *	@page valueio Values input/output
 *
 *	There are two types of formats for data on-the-wire with PostgreSQL:
 *	text and binary. The text format is more universal, but the binary format
 *	is more compact and sometimes closer to the native client data formats.
 *
 *	### Value Input (read query results)
 *
 *	Query results in simple query mode are always in text format.
 *
 *	In extended query mode the resultset can contain fields in text and in
 *	binary formats. When a request is prepared pg_async specifies data formats
 *	for each of the fields. A PostgreSQL type oid is determined by a
 *	@ref tip::db::pg::io::traits::cpppg_data_mapping specialization for a type.
 *	A binary format for a field will be requested if the presence of a binary
 *	parser was explicitly specified.
 *	(@ref tip::db::pg::io::traits::register_binary_parser )
 *
 *	#### `protocol_parser` Concept
 *
 *	A protocol parser is a functor that gets an lvalue reference to a value
 *	in constructor and defines a template operator() that takes two iterators
 *	to a char buffer (begin and end iterators) and uses that data range to
 *	parse the value. If the parse was a success return the new position of
 *	input iterator. If it fails, returns the starting position of the input
 *	iterator. It must define a value_type.
 *
 *	Protocol parsers are used by the result set to extract values from data
 *	row buffers.
 *
 *	#### Adding Support for a Data Type Input
 *
 *	To support parsing of a data type from input buffer a structure conforming
 *	to `protocol_parser` concept must be implemented. It can be either a
 *	specialization of @ref tip::db::pg::io::protocol_parser or any other
 *	structure. In the latter case tip::db::pg::io::protocol_io_traits must be
 *	specialized for the type using the parser class as a typedef for
 *	`parser_type`.
 *
 *	A text parser for a data type is mandatory, binary parser is optional. To
 *	make pg_async request the data type in a binary format, the PostgreSQL type
 *	oid must be registered as having a binary parser.
 *
 *	Adding text data format parser:
 *
 *	* Implement a parser
 *
 *	Adding a binary data parser:
 *
 *	* Implement a parser
 *	* Specialize tip::db::pg::io::traits::has_parser for the type, to enable
 *		field reader implementation that uses the binary parser.
 *	* Call tip::db::pg::io::traits::register_binary_parser for pg_async runtime
 *		to request fields with the type oid in binary format.
 *
 *	@code
 *	// Example specialization of a parser for boolean type
 *	namespace tip { namespace db { namespace pg { namespace io {
 *
 *	template < >
 *	struct protocol_parser < bool, BINARY_DATA_FORMAT > {
 *		typedef bool value_type;
 *
 *		value_type& value;
 *		protocol_parser(value_type& value) : value(value) {}
 *
 *		template < typename InputIterator >
 *		InputIterator
 *		operator()(InputIterator begin, InputIterator end)
 *		{
 *			// input, parse, assign to the value
 *		}
 *	};
 *	}}}} // namespace tip::db::pg::io
 *
 *	// Example separate structure
 *	struct bool_parser {
 *		typedef bool value_type;
 *		value_type& value;
 *
 *		bool_parser(value_type& value) : value(value) {}
 *
 *		template < typename InputIterator >
 *		InputIterator
 *		operator()(InputIterator begin, InputIterator end)
 *		{
 *			// input, parse, assign to the value
 *		}
 *	};
 *
 *	namespace tip { namespace db { namespace pg { namespace io {
 *	// Specialization of protocol_io_traits
 *	template <>
 *	struct protocol_io_traits < bool, BINARY_DATA_FORMAT > {
 *		// ...
 *		typedef bool_parser parser_type;
 *		// ...
 *	};
 *
 *	namespace traits {
 *		// Enable binary parser for the type
 *		template <> struct has_parser<bool, BINARY_DATA_FORMAT > : std::true_type {};
 *	} // namespace traits
 *	}}}} // namespace tip::db::pg::io
 *
 *	// Somewhere in initialization code
 *	using namespace tip::db::pg;
 *	io::traits::register_binary_parser( oids::type::boolean );
 *
 *	@endcode
 *
 *	@see tip::db::pg::io::protocol_read
 *	@see tip::db::pg::io::protocol_io_traits
 *	@see tip::db::pg::io::protocol_parser
 *
 *	### Value Output (write query parameters)
 *
 *	When there is no specialization of a protocol_formatter for a type,
 *	default implementation based on standard C++ iostreams is used. Protocol
 *	data format is text in this case.
 *
 *	#### `protocol_formatter` Concept
 *
 *	A `protocol_fomatter` is a functor that gets an rvalue reference to data
 *	that needs to be output and defines an operator() that takes a reference to
 *	a data buffer (`std::vector<byte>`) for output.
 *
 *	Protocol formatters are used by query to write parameters sent to PostgreSQL
 *	server.
 *
 *	#### Adding Support for a Data Type Output
 *
 *	To support formatting of a data type to send data to PostgreSQL server a
 *	structure conforming to `protocol_formatter` concept must be implemented.
 *	It can be either a specialization of @ref tip::db::pg::io::protocol_formatter
 *	or any other structure. In the latter case tip::db::pg::io::protocol_io_traits
 *	must be specialized for the type using the formatter class as a typedef
 *	for `formatter_type`.
 *
 *	A text formatter for a data type is mandatory, binary formaater is optional.
 *
 *	Adding text data format parser:
 *
 *	* Implement a formatter
 *
 *	Adding a binary data parser:
 *
 *	* Implement a formatter
 *	* Specialize tip::db::pg::io::traits::has_formatter for the type, to enable
 *		parameter writer to use it.
 *
 *	@code
 *	// Example specialization of a formatter for boolean type
 *	namespace tip { namespace db { namespace pg { namespace io {
 *
 *	template < >
 *	struct protocol_formatter < bool, BINARY_DATA_FORMAT > {
 *		typedef bool value_type;
 *
 *		value_type const& value;
 *		protocol_formatter(value_type const& value) : value(value) {}
 *
 *		bool
 *		operator()(std::vector<byte>& buffer)
 *		{
 *			buffer.push_back(value ? 1 : 0);
 *			return true;
 *		}
 *	};
 *	}}}} // namespace tip::db::pg::io
 *
 *	// Example separate structure
 *	struct bool_formatter {
 *		typedef bool value_type;
 *		value_type const& value;
 *
 *		bool_formatter(value_type const& value) : value(value) {}
 *
 *		bool
 *		operator()(std::vector<byte>& buffer)
 *		{
 *			buffer.push_back(value ? 1 : 0);
 *			return true;
 *		}
 *	};
 *
 *	namespace tip { namespace db { namespace pg { namespace io {
 *	// Specialization of protocol_io_traits
 *	template <>
 *	struct protocol_io_traits < bool, BINARY_DATA_FORMAT > {
 *		// ...
 *		typedef bool_formatter formatter_type;
 *		// ...
 *	};
 *
 *	namespace traits {
 *		// Enable binary formatter for the type
 *		template <> struct has_formatter<bool, BINARY_DATA_FORMAT > : std::true_type {};
 *	} // namespace traits
 *	}}}} // namespace tip::db::pg::io
 *	@endcode
 *
 *	@see tip::db::pg::io::protocol_write
 *	@see tip::db::pg::io::protocol_io_traits
 *	@see tip::db::pg::io::protocol_formatter
 */

#ifndef TIP_DB_PG_HPP_
#define TIP_DB_PG_HPP_

#include <tip/db/pg/database.hpp>
#include <tip/db/pg/transaction.hpp>
#include <tip/db/pg/query.hpp>
#include <tip/db/pg/resultset.hpp>
#include <tip/db/pg/error.hpp>

#endif /* TIP_DB_PG_HPP_ */
