/*
 * connection_pool.h
 *
 *  Created on: 11 июля 2015 г.
 *     @author: zmij
 */

#ifndef TIP_DB_PG_DETAIL_CONNECTION_POOL_H_
#define TIP_DB_PG_DETAIL_CONNECTION_POOL_H_

#include <memory>

#include <boost/noncopyable.hpp>
#include <tip/db/pg/asio_config.hpp>

#include <tip/db/pg/database.hpp>

#include <vector>
#include <queue>
#include <mutex>

namespace tip {
namespace db {
namespace pg {

namespace detail {

/**
 * Container of connections to the same database
 */
class connection_pool : public std::enable_shared_from_this<connection_pool>,
		private boost::noncopyable {
public:
	typedef asio_config::io_service_ptr io_service_ptr;

	typedef std::vector<connection_ptr> connections_container;
	typedef std::queue<connection_ptr> connections_queue;

	typedef std::pair<transaction_callback, error_callback> request_callbacks;
	typedef std::queue< request_callbacks > request_callbacks_queue;

	typedef std::recursive_mutex mutex_type;
	typedef std::lock_guard<mutex_type> lock_type;

	typedef std::shared_ptr<connection_pool> connection_pool_ptr;
public:
	// TODO Error handlers
private:
	connection_pool(io_service_ptr service, size_t pool_size,
			connection_options const& co,
			client_options_type const&);
public:
	static connection_pool_ptr
	create(io_service_ptr service, size_t pool_size,
			connection_options const& co,
			client_options_type const& = client_options_type());

	~connection_pool();

	dbalias const&
	alias() const
	{ return co_.alias; }

	void
	get_connection(transaction_callback const&, error_callback const&);

	void
	close(simple_callback); // FIXME Add a close callback
private:
	void
	create_new_connection();
	void
	connection_ready(connection_ptr c);
	void
	connection_terminated(connection_ptr c);
	void
	connection_error(connection_ptr c, error::connection_error const& ec);
private:
	io_service_ptr 			service_;
	size_t					pool_size_;
	connection_options		co_;
	client_options_type		params_;

	mutex_type				mutex_;

	connections_container	connections_;
	connections_queue		ready_connections_;

	request_callbacks_queue	waiting_;

	bool					closed_;
	simple_callback			closed_callback_;
};

} /* namespace detail */
} /* namespace pg */
} /* namespace db */
} /* namespace tip */

#endif /* TIP_DB_PG_DETAIL_CONNECTION_POOL_H_ */
