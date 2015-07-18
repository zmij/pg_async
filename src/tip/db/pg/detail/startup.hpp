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
	enum auth_states {
		OK				= 0, /**< Specifies that the authentication was successful. */
		KerberosV5		= 2, /**< Specifies that Kerberos V5 authentication is required. */
		Cleartext		= 3, /**< Specifies that a clear-text password is required. */
		/**
		 * Specifies that an MD5-encrypted password is required.
		 * Message contains additional 4 bytes of salt
		 */
		MD5Password		= 5,
		SCMCredential	= 6, /**< Specifies that an SCM credentials message is required. */
		GSS				= 7, /**< Specifies that GSSAPI authentication is required. */
		/**
		 * Specifies that this message contains GSSAPI or SSPI data.
		 * Message contains additional bytes with GSSAPI or SSPI authentication data.
		 */
		GSSContinue		= 8,
		SSPI			= 9, /**< Specifies that SSPI authentication is required. */

	};
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
