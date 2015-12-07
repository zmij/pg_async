/*
 * uuid.hpp
 *
 *  Created on: Oct 5, 2015
 *      Author: zmij
 */

#ifndef CEREAL_INCLUDE_CEREAL_TYPES_UUID_HPP_
#define CEREAL_INCLUDE_CEREAL_TYPES_UUID_HPP_

#include <cereal/cereal.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace cereal {

template < typename Archive >
typename std::enable_if< std::is_same< Archive, ::cereal::BinaryOutputArchive >::value, void >::type
CEREAL_SAVE_FUNCTION_NAME(Archive& ar, boost::uuids::uuid const& uuid)
{
	ar( make_size_tag(uuid.size()) );
	ar( binary_data(uuid.data, uuid.size() * sizeof(uint8_t)) );
}

template < typename Archive >
typename std::enable_if< std::is_same< Archive, ::cereal::BinaryInputArchive >::value, void >::type
CEREAL_LOAD_FUNCTION_NAME(Archive& ar, boost::uuids::uuid& uuid)
{
	size_type sz;
	ar(make_size_tag(sz));
	assert(sz == 16 && "Size of uuid must be 16");
	ar( binary_data(uuid.data, uuid.size() * sizeof(uint8_t)) );
}

template < typename Archive >
typename std::enable_if< !std::is_same< Archive, ::cereal::BinaryOutputArchive >::value, void >::type
CEREAL_SAVE_FUNCTION_NAME(Archive& ar, boost::uuids::uuid const& uuid)
{
	std::ostringstream os;
	os << uuid;
	ar(os.str());
}

template < typename Archive >
typename std::enable_if< !std::is_same< Archive, ::cereal::BinaryInputArchive >::value, void >::type
CEREAL_LOAD_FUNCTION_NAME(Archive& ar, boost::uuids::uuid& uuid)
{
	std::string str;
	ar(str);
	std::istringstream is(str);
	is >> uuid;
}

namespace traits {

template < class InputArchive >
struct has_minimal_input_serialization<boost::uuids::uuid, InputArchive > : std::true_type {};
template < class OutputArchive >
struct has_minimal_output_serialization<boost::uuids::uuid, OutputArchive > : std::true_type {};

}  // namespace traits

}  // namespace cereal

#endif /* CEREAL_INCLUDE_CEREAL_TYPES_UUID_HPP_ */
