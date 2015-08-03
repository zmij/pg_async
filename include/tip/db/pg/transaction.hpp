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
#include <tip/db/pg/common.hpp>

namespace tip {
namespace db {
namespace pg {
class basic_connection;

/**
 *  @brief RAII transaction object.
 *
 *  It is created by the library internally when a transaction is started. It
 *  will rollback the transaction if it wasn't explicitly committed.
 *
 * 	@warning A tip::db::pg::transaction object shoudn't be stored and accessed
 * 	concurrently.
 */
class transaction : public std::enable_shared_from_this< transaction > {
public:
	typedef std::shared_ptr<basic_connection> connection_ptr;
	typedef basic_connection* pointer;
	typedef basic_connection const* const_pointer;
public:
	transaction(connection_ptr);
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

	dbalias const&
	alias() const;

	bool
	in_transaction() const;

	void
	commit();
	void
	rollback();

	void
	execute(std::string const& query, query_result_callback,
			query_error_callback);
	void
	execute(std::string const& query, type_oid_sequence const& param_types,
			std::vector< byte > params_buffer,
			query_result_callback, query_error_callback);
private:
	void
	handle_results(resultset, bool, query_result_callback);
	void
	handle_query_error(error::query_error const&, query_error_callback);
	connection_ptr connection_;
	bool finished_;
};

} /* namespace pg */
} /* namespace db */
} /* namespace tip */

#endif /* TIP_DB_PG_DETAIL_CONNECTION_LOCK_HPP_ */
