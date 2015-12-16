/*
 * protocol.hpp
 *
 *  Created on: Oct 5, 2015
 *      Author: zmij
 */

#ifndef TIP_GAME_AUTH_PROTOCOL_HPP_
#define TIP_GAME_AUTH_PROTOCOL_HPP_

#include <string>
#include <awm/game/common.hpp>
#include <cereal/cereal.hpp>
#include <cereal/types/uuid.hpp>
#include <iosfwd>

namespace awm {
namespace game {
namespace authn {

/**
 * Structure for Vendor UID registration request
 */
struct vendor_uid_req {
	identity_type	userid;
	std::string		locale;
	std::string		tz;

	template < typename Archive >
	void
	serialize(Archive& archive)
	{
		archive(
			CEREAL_NVP(userid),
			CEREAL_NVP(locale),
			CEREAL_NVP(tz)
		);
	}

	operator bool() const
	{
		return !(userid.is_nil() || locale.empty() || tz.empty());
	}
};

std::ostream&
operator << (std::ostream& os, vendor_uid_req const& val);

void
set_authn_response_lobby_uri( std::string& );

struct authn_response {
	identity_type	uid;
	std::string		name;
	identity_type	token;

	static
	std::string		lobby_uri;

	template < typename Archive >
	void
	serialize(Archive& archive)
	{
		archive(
			CEREAL_NVP(uid),
			CEREAL_NVP(name),
			CEREAL_NVP(token),
			CEREAL_NVP(lobby_uri)
		);
	}
};

struct token_authn_req {
	identity_type	token;
	std::string		auth;

	template < typename Archive >
	void
	serialize(Archive& archive)
	{
		archive(
			CEREAL_NVP(token),
			CEREAL_NVP(auth)
		);
	}

	operator bool () const
	{
		return !(token.is_nil() || auth.empty());
	}
};

struct online_stats {
	size_t online;
	template <typename Archive>
	void
	serialize(Archive& archive)
	{
		archive(
			CEREAL_NVP(online)
		);
	}
};

}  // namespace authn
}  // namespace game
}  // namespace awm


#endif /* TIP_GAME_AUTH_PROTOCOL_HPP_ */
