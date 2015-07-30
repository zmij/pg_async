/**
 * @file /tip-server/src/tip/db/pg/detail/auth.hpp
 * @brief
 * @date Jul 10, 2015
 * @author: zmij
 */

#ifndef TIP_DB_PG_DETAIL_AUTH_HPP_
#define TIP_DB_PG_DETAIL_AUTH_HPP_

#include "tip/db/pg/detail/protocol.hpp"
#include "tip/db/pg/detail/basic_state.hpp"

namespace tip {
namespace db {
namespace pg {
namespace detail {

class startup_state : public basic_state {
public:
	startup_state(connection_base& conn);
	virtual ~startup_state() {}
private:
	virtual bool
	do_handle_message(message_ptr);

	virtual std::string const
	get_name() const
	{ return "startup"; }

	virtual connection::state_type
	get_state() const
	{ return conn_state_; }

	connection::state_type conn_state_;
};

}  // namespace detail
}  // namespace pg
}  // namespace db
}  // namespace tip

#endif /* TIP_DB_PG_DETAIL_AUTH_HPP_ */
