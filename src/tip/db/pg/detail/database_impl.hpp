/*
 * database_impl.hpp
 *
 *  Created on: Jul 13, 2015
 *      Author: zmij
 */

#ifndef TIP_DB_PG_DETAIL_DATABASE_IMPL_HPP_
#define TIP_DB_PG_DETAIL_DATABASE_IMPL_HPP_

#include <tip/db/pg/common.hpp>
#include <tip/db/pg/database.hpp>
#include <tip/db/pg/asio_config.hpp>

#include <boost/noncopyable.hpp>

#include <map>

namespace tip {
namespace db {
namespace pg {
namespace detail {

struct connection_pool;

class database_impl : private boost::noncopyable {
	typedef std::shared_ptr<connection_pool> connection_pool_ptr;
	typedef std::map<dbalias, connection_pool_ptr> connections_map;
public:
	database_impl(size_t pool_size, client_options_type const& defaults);
	virtual ~database_impl();

	void
	set_defaults(size_t pool_size, client_options_type const& defaults);

	void
	add_connection(std::string const& connection_string,
			db_service::optional_size pool_size = db_service::optional_size(),
			client_options_type const& params = client_options_type());
	void
	add_connection(connection_options options,
			db_service::optional_size pool_size = db_service::optional_size(),
			client_options_type const& params = client_options_type());

	void
	get_connection(dbalias const&, transaction_callback const&,
			error_callback const&);

	void
	run();

	void
	stop();

	asio_config::io_service&
	io_service()
	{
		return service_;
	}
private:
	connection_pool_ptr
	add_pool(connection_options const&,
			db_service::optional_size = db_service::optional_size(),
			client_options_type const& = {});

	asio_config::io_service		service_;
	size_t						pool_size_;

	connections_map				connections_;
	client_options_type			defaults_;
};

} /* namespace detail */
} /* namespace pg */
} /* namespace db */
} /* namespace tip */

#endif /* TIP_DB_PG_DETAIL_DATABASE_IMPL_HPP_ */
