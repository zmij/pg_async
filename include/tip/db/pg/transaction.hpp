/**
 *	@file tip/pg/db/transaction.hpp
 *
 *  @date Jul 14, 2015
 *  @author zmij
 */

#ifndef TIP_DB_PG_DETAIL_CONNECTION_LOCK_HPP_
#define TIP_DB_PG_DETAIL_CONNECTION_LOCK_HPP_

#include <memory>
#include <functional>

namespace tip {
namespace db {
namespace pg {
class connection;

class transaction : public std::enable_shared_from_this< transaction > {
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

	//@{
	/** Access to underlying connection object */
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
	//@}

	bool
	in_transaction() const;

	void
	commit(transaction_callback = transaction_callback(),
			error_callback = error_callback());
	void
	rollback(transaction_callback = transaction_callback(),
			error_callback = error_callback());
private:
	connection_ptr connection_;
	release_func release_;
};

} /* namespace pg */
} /* namespace db */
} /* namespace tip */

#endif /* TIP_DB_PG_DETAIL_CONNECTION_LOCK_HPP_ */
