/*
 * connection_lock.h
 *
 *  Created on: 14 июля 2015 г.
 *     @author: zmij
 */

#ifndef TIP_DB_PG_DETAIL_CONNECTION_LOCK_HPP_
#define TIP_DB_PG_DETAIL_CONNECTION_LOCK_HPP_

#include <memory>
#include <functional>

namespace tip {
namespace db {
namespace pg {
class connection;

class transaction {
public:
	typedef std::shared_ptr<connection> connection_ptr;
	typedef std::function< void () > release_func;
	typedef connection* pointer;
	typedef connection const* const_pointer;
public:
	transaction(connection_ptr, release_func);
	~transaction();

	transaction(transaction const&) = delete;
	transaction&
	operator = (transaction const&) = delete;
	transaction(transaction&&) = delete;
	transaction&
	operator = (transaction&&) = delete;

	pointer
	operator-> ()
	{
		return connection_.get();
	}
	const_pointer
	operator-> () const
	{
		return connection_.get();
	}

	pointer
	operator* ()
	{
		return connection_.get();
	}
	const_pointer
	operator* () const
	{
		return connection_.get();
	}
private:
	connection_ptr connection_;
	release_func release_;
};

} /* namespace pg */
} /* namespace db */
} /* namespace tip */

#endif /* TIP_DB_PG_DETAIL_CONNECTION_LOCK_HPP_ */
