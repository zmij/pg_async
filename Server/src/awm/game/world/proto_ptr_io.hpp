/*
 * proto_ptr_io.hpp
 *
 *  Created on: Sep 29, 2015
 *      Author: zmij
 */

#ifndef TIP_GAME_WORLD_PROTO_PTR_IO_HPP_
#define TIP_GAME_WORLD_PROTO_PTR_IO_HPP_

#include <tip/db/pg/protocol_io_traits.hpp>
#include <awm/game/world/proto_ptr.hpp>
#include <iosfwd>

namespace awm {
namespace game {
namespace world {

template < typename T >
std::ostream&
operator << (std::ostream& out, proto_ptr< T > const& val)
{
	std::ostream::sentry s(out);
	if (s) {
		out << val.proto_id();
	}
	return out;
}

template < typename T >
std::istream&
operator >> (std::istream& is, proto_ptr< T >& val)
{
	std::istream::sentry s(is);
	if (s) {
		is >> val.proto_id();
	}
	return is;
}


}  // namespace world
}  // namespace game

namespace db {
namespace pg {
namespace io {

namespace traits {

template < typename T >
struct cpppg_data_mapping< game::world::proto_ptr< T > > :
	detail::data_mapping_base < oids::type::text, game::world::proto_ptr< T > > {};

}  // namespace traits

}  // namespace io
}  // namespace pg
}  // namespace db
}  // namespace awm

#endif /* TIP_GAME_WORLD_PROTO_PTR_IO_HPP_ */
