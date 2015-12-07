/**
 * uuid_hash.hpp
 *
 *  Created on: 31 авг. 2015 г.
 *      Author: zmij
 */

#ifndef TIP_UTIL_UUID_HASH_HPP_
#define TIP_UTIL_UUID_HASH_HPP_

#include <boost/uuid/uuid.hpp>

namespace std {

template <>
struct hash< boost::uuids::uuid > {
	inline size_t
	operator()(boost::uuids::uuid const& uuid) const
	{
		return boost::uuids::hash_value(uuid);
	}
};

}  // namespace std

#endif /* TIP_UTIL_UUID_HASH_HPP_ */
