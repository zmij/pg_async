/*
 * connection_base.hpp
 *
 *  Created on: 11 июля 2015 г.
 *     @author: zmij
 */

#ifndef TIP_DB_PG_DETAIL_CONNECTION_BASE_HPP_
#define TIP_DB_PG_DETAIL_CONNECTION_BASE_HPP_

#include <boost/noncopyable.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <map>
#include <stack>
#include <set>

#include <tip/db/pg/connection.hpp>
#include <tip/db/pg/detail/protocol.hpp>
#include <tip/db/pg/detail/basic_state.hpp>

#include <boost/msm/back/state_machine.hpp>
#include <boost/msm/front/state_machine_def.hpp>
#include <boost/msm/front/functor_row.hpp>
#include <boost/msm/front/euml/operator.hpp>

namespace tip {
namespace db {
namespace pg {

class resultset;

namespace detail {

namespace asio = boost::asio;

class basic_state;
typedef std::shared_ptr<basic_state> connection_state_ptr;

class connection_base : public std::enable_shared_from_this< connection_base >,
	private boost::noncopyable {
public:
		//@{
		/** @name Typedefs */
		typedef std::shared_ptr< message > message_ptr;

		typedef std::map< std::string, std::string > options_type;
		typedef asio::ip::tcp tcp;
		typedef asio::io_service io_service;
		typedef boost::system::error_code error_code;
		typedef std::shared_ptr< connection_base > impl_ptr;
		typedef std::function< void (impl_ptr) > event_callback;
		typedef std::function< void (connection_error const&) > connection_error_callback;
		typedef internal_result_callback result_callback;
		typedef std::function< void (boost::system::error_code const&,
				size_t bytes)> api_handler;
		typedef std::vector< oids::type::oid_type > type_oid_sequence;
		typedef std::vector<byte> buffer_type;
		typedef std::vector<field_description> row_description;
		//@}
public:
	connection_base(io_service& service, connection_options const& co,
					event_callback const& ready,
					event_callback const& terminated,
					connection_error_callback const& err,
					options_type const& aux);
	virtual ~connection_base();
public:
	connection::state_type
	connection_state() const;

	void
	transit_state(connection_state_ptr state);
	void
	push_state(connection_state_ptr state);
	void
	pop_state(basic_state* sender);

	connection_state_ptr
	state();
	//@{
	/** @name Accessors */
	std::string const&
	uri() const
	{
		return conn.uri;
	}
	std::string const&
	database() const
	{
		return conn.database;
	}
	connection_options const&
	options() const
	{
		return conn;
	}
	//@}
	//@{
	/** @name Interface to calling callbacks */
	/** Must be called when the backend sends "Ready for query" message */
	void
	ready();
	void
	error(connection_error const&);
	//@}

	void
	terminate();

	virtual void
	send(message const& m, api_handler = api_handler()) = 0;

	//@{
	/** @name Transactions interface */
	void
	begin_transaction(simple_callback const&, error_callback const&, bool autocommit);

	void
	commit_transaction(simple_callback const&, error_callback const&);

	void
	rollback_transaction(simple_callback const&, error_callback const&);

	bool
	in_transaction() const;
	//@}

	//@{
	/** @name Querying */
	void
	execute_query(std::string const& query, result_callback const& cb,
			query_error_callback const& err);

	void
	execute_prepared(std::string const& query,
			type_oid_sequence const& param_types,
			buffer_type const& params,
			result_callback const&,
			query_error_callback const&);
	//@}

	//@{
	bool
	is_prepared(std::string const& query_hash) const;
	void
	set_prepared(std::string const& query_hash,
			row_description const& result_desc);
	row_description const&
	get_prepared_description(std::string const& query_hash) const;
	//@}
	void
	notify_terminated();

	void
	lock();
	void
	unlock();

	bool
	locked() const;
protected:
	void
	create_startup_message(message& m);

	void
	read_message(std::istreambuf_iterator<char> in, size_t max_bytes);

	void
	handle_message(message_ptr m);

	void
	handle_connect(error_code const& ec);

	void
	handle_write(error_code const& ec, size_t bytes_transfered);

	void
	read_package_complete(size_t bytes);
protected:
	connection_options conn;

	asio::io_service::strand strand_;
	asio::streambuf outgoing_;
	asio::streambuf incoming_;

private:
	virtual void
	start_read() = 0;
	virtual void
	close() = 0;
private:
	typedef std::set< std::string > string_set_type;
	typedef std::map< std::string, row_description > prepared_statements;

	options_type settings_;

	int32_t serverPid_;
	int32_t	serverSecret_;

	message_ptr message_;

	state_stack state_;

	event_callback ready_;
	event_callback terminated_;
	connection_error_callback err_;

	bool locked_;

	prepared_statements prepared_statements_;
protected:
	bool
	is_terminated() const;
};

}  // namespace detail
}  // namespace pg
}  // namespace db
}  // namespace tip

#endif /* TIP_DB_PG_DETAIL_CONNECTION_BASE_HPP_ */
