/**
 *  @file tip/db/pg/database.hpp
 *
 *  @date Jul 10, 2015
 *  @author zmij
 */

#ifndef TIP_DB_PG_DATABASE_HPP_
#define TIP_DB_PG_DATABASE_HPP_

#include <string>
#include <memory>
#include <functional>
#include <map>
#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>
#include <boost/optional.hpp>

#include <tip/db/pg/common.hpp>

namespace tip {
namespace db {
namespace pg {

namespace detail {
struct database_impl;
}
/**
 * Database connection manager.
 * Synopsis:
 * @code
 * // set up connection
 * database::add_connection(connection_string);
 * // get connection with a connection string
 * database::get_connection_async(
 * 		connection_string,
 * 		[](database::connection_ptr c) {
 * 			// run queries here
 * 		},
 * 		[](boost::system::error_code) {
 * 			// handle the connection error here
 * 		}
 * );
 * // get a connection created with alias
 * database::get_connection_async(
 * 		dbalias,
 * 		[](database::connection_ptr c) {
 * 			// run queries here
 * 		},
 * 		[](boost::system::error_code) {
 * 			// handle the connection error here
 * 		}
 * );
 * //
 * @endcode
 */
class db_service {
public:
	/** Opaque connection pointer. */
	typedef boost::system::error_code error_code;

	typedef std::map< std::string, std::string > connection_params;
	typedef boost::optional<size_t> optional_size;
public:
	static const size_t DEFAULT_POOOL_SIZE = 4;
public:
	/**
	 * Initialize the database service with the default pool_size per alias
	 * and default connection parameters.
	 * @param pool_size number of connections per alias
	 * @param defaults default settings for the connection
	 */
	static void
	initialize(size_t pool_size, connection_params const& defaults);
	/**
	 * Add a connection string.
	 * Will require an alias, for the database to be referenced by it later.
	 * @param connection_string
	 * @param pool_size A connection can have a pool size different from
	 * 		other connections.
	 */
	static void
	add_connection(std::string const& connection_string,
			optional_size = optional_size());

	/**
	 * Create a connection or retrieve a connection from the connection pool.
	 * Will also register a connection with an alias supplied or an alias
	 * generated from connection uri, user and database.
	 * If the alias is already registered, will search for idle connections
	 * associated with this alias.
	 * If there is an idle connection in the pool, will return it.
	 * If no idle connections are available, and the size of connection pool
	 * didn't reach it's limit, will create a new one.
	 * If the pool is full and no idle connections are available,
	 * will return the first connection that becomes idle.
	 * @param connection_string @see @c connection_options for details
	 * @param result callback function that will be called when a connection
	 * 	becomes available.
	 * @param error callback function that will be called in case of an error.
	 */
	static void
	get_connection_async(std::string const&, connection_lock_callback const&,
			error_callback const&);

	/**
	 * @see get_connection_async(std::string const&, result_callback, error_callback)
	 * Will lookup a connection by alias. If a new connection must be created,
	 * it will be created with the connection string associated with the alias.
	 * @param connection_string @see @c connection_options for details
	 * @param result callback function that will be called when a connection
	 * 	becomes available.
	 * @param error callback function that will be called in case of an error.
	 */
	static void
	get_connection_async(dbalias const&, connection_lock_callback const&,
			error_callback const&);

	static void
	run();
	static void
	stop();

	static boost::asio::io_service&
	io_service();
private:
	// No instances
	db_service() {}

	typedef std::shared_ptr<detail::database_impl> pimpl;
	static pimpl pimpl_;

	static pimpl
	impl(size_t pool_size = DEFAULT_POOOL_SIZE,
			connection_params const& defaults = {});
};

}  // namespace pg
}  // namespace db
}  // namespace tip

#endif /* TIP_DB_PG_DATABASE_HPP_ */
