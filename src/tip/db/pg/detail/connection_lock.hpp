/*
 * connection_lock.h
 *
 *  Created on: 14 июля 2015 г.
 *      Author: brysin
 */

#ifndef TIP_DB_PG_DETAIL_CONNECTION_LOCK_HPP_
#define TIP_DB_PG_DETAIL_CONNECTION_LOCK_HPP_

#include <memory>
#include <functional>

namespace tip {
namespace db {
namespace pg {
struct connection;
namespace detail {

class connection_lock {
public:
	typedef std::shared_ptr<connection> connection_ptr;
	typedef std::function< void () > release_func;
public:
	connection_lock(connection_ptr, release_func);
	~connection_lock();

	connection_lock(connection_lock const&) = delete;
	connection_lock&
	operator = (connection_lock const&) = delete;
	connection_lock(connection_lock&&) = delete;
	connection_lock&
	operator = (connection_lock&&) = delete;

	connection_ptr
	operator-> ()
	{
		return connection_;
	}
	connection_ptr
	operator-> () const
	{
		return connection_;
	}

	connection_ptr
	operator* ()
	{
		return connection_;
	}
	connection_ptr
	operator* () const
	{
		return connection_;
	}
private:
	connection_ptr connection_;
	release_func release_;
};

} /* namespace detail */
} /* namespace pg */
} /* namespace db */
} /* namespace tip */

#endif /* TIP_DB_PG_DETAIL_CONNECTION_LOCK_HPP_ */
