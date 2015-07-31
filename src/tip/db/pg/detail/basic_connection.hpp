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

#include <tip/db/pg/detail/protocol.hpp>

//#include <boost/msm/back/state_machine.hpp>
//#include <boost/msm/front/state_machine_def.hpp>
//#include <boost/msm/front/functor_row.hpp>
//#include <boost/msm/front/euml/operator.hpp>

namespace tip {
namespace db {
namespace pg {

class resultset;
class transaction;
typedef std::shared_ptr< transaction > transaction_ptr;

class basic_connection;
typedef std::shared_ptr< basic_connection > basic_connection_ptr;

typedef std::function < void (basic_connection_ptr) > connection_event_callback;
typedef std::function < void (basic_connection_ptr, connection_error) > connection_error_callback;

struct connection_callbacks {
	connection_event_callback	idle;
	connection_event_callback	terminated;
	connection_error_callback	error;
};

namespace events {
struct begin {
	// TODO Transaction isolation etc
	transaction_callback	started;
	error_callback			error;
};

struct commit {};
struct rollback {};

struct execute {
	std::string				expression;
	query_result_callback	result;
	query_error_callback	error;
};
struct execute_prepared {
	std::string expression;
	std::vector< oids::type::oid_type > param_types;
	std::vector< byte > params;
	query_result_callback	result;
	query_error_callback	error;
};

}

class basic_connection : public boost::noncopyable {
public:
	typedef boost::asio::io_service io_service;
public:
	static basic_connection_ptr
	create(io_service& svc, connection_options const&,
			client_options_type const&, connection_callbacks const&);
public:
	virtual ~basic_connection() {}

	void
	connect(connection_options const&);

	void
	begin(events::begin const&);
	void
	commit();
	void
	rollback();

	bool
	in_transaction() const;

	void
	terminate();
protected:
	basic_connection();

private:
	virtual void
	do_connect(connection_options const& ) = 0;

	virtual bool
	is_in_transaction() const = 0;

	virtual void
	do_begin(events::begin const&) = 0;
	virtual void
	do_commit() = 0;
	virtual void
	do_rollback() = 0;

	virtual void
	do_terminate() = 0;
};

}  // namespace pg
}  // namespace db
}  // namespace tip

#endif /* TIP_DB_PG_DETAIL_CONNECTION_BASE_HPP_ */
