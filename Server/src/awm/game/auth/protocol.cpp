/*
 * protocol.cpp
 *
 *  Created on: Oct 5, 2015
 *      Author: zmij
 */

#include <awm/game/auth/protocol.hpp>
#include <iostream>

namespace awm {
namespace game {
namespace authn {

std::ostream&
operator << (std::ostream& os, vendor_uid_req const& val)
{
	std::ostream::sentry s(os);
	if (s) {
		os << val.userid << " " << val.locale << " " << val.tz;
	}
	return os;
}


}  // namespace authn
}  // namespace game
}  // namespace awm

