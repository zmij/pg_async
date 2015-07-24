/**
 * pgconnection.hpp
 *
 *  Created on: 07 июля 2015 г.
 *      Author: zmij
 */

#ifndef TIP_DB_PGCONNECTION_HPP_
#define TIP_DB_PGCONNECTION_HPP_

#include <string>
#include <memory>
#include <map>
#include <functional>

#include <boost/asio.hpp>
#include <boost/noncopyable.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <tip/db/pg/common.hpp>

namespace tip {
namespace db {
namespace pg {

namespace detail {
struct connection_base;
}  // namespace detail

/**
 * Connection interface
 */
class connection : public std::enable_shared_from_this<connection>,
		private boost::noncopyable {
public:
	typedef boost::asio::io_service io_service;

	typedef boost::asio::ip::tcp::endpoint tcp_endpoint;
	typedef boost::asio::local::stream_protocol::endpoint local_endpoint;

	typedef std::map<std::string, std::string> connection_params;

	typedef query_result_callback result_callback;

	typedef std::vector< oids::type::oid_type > type_oid_sequence;
	typedef std::vector<byte> buffer_type;

	enum state_type {
		DISCONNECTED,
		CONNECTING,
		IDLE,
		BUSY,
		LOCKED
	};

private:
	// No stack allocation
	connection();
	void
	init(io_service&,
			   connection_event_callback const& ready,
			   connection_event_callback const& terminated,
			   connection_error_callback const& err,
			   connection_options const& opts,
	           connection_params const&);
public:
	static connection_ptr
	create(io_service&,
			connection_event_callback const& ready,
			connection_event_callback const& terminated,
			connection_error_callback const& err,
			connection_options const& opts,
			connection_params const& = connection_params());
	virtual
	~connection();

	state_type
	state() const;

	void
	terminate();

	connection_lock_ptr
	lock();

	void
	begin_transaction(connection_lock_callback const&, error_callback const&,
			bool autocommit = false);
	void
	commit_transaction(connection_lock_ptr, connection_lock_callback const&,
			error_callback const&);
	void
	rollback_transaction(connection_lock_ptr, connection_lock_callback const&,
			error_callback const&);

	bool
	in_transaction() const;

	void
	execute_query(std::string const& query,
			result_callback const& cb,
			error_callback const& err,
			connection_lock_ptr l = connection_lock_ptr());
	void
	execute_prepared(std::string const& query,
			type_oid_sequence const& param_types,
			buffer_type const& params,
			result_callback const& cb,
			error_callback const& err,
			connection_lock_ptr l = connection_lock_ptr());
private:
	typedef std::shared_ptr<detail::connection_base> pimpl;

	void
	unlock();

	void
	implementation_event(connection_event_callback ready, pimpl);
	void
	implementation_error(connection_error_callback err,
			connection_error const& ec);
	void
	transaction_started(connection_lock_callback);
	void
	transaction_finished(connection_lock_ptr, connection_lock_callback);
	void
	query_executed(result_callback cb, resultset r,
			bool complete, connection_lock_ptr);

	pimpl pimpl_;
};

}  // namespace pg
} /* namespace db */
} /* namespace tip */


#endif /* TIP_DB_PGCONNECTION_HPP_ */
