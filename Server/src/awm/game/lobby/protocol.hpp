/*
 * protocol.hpp
 *
 *  Created on: Dec 28, 2015
 *      Author: zavyalov
 */

#ifndef TIP_GAME_LOBBY_PROTOCOL_HPP_
#define TIP_GAME_LOBBY_PROTOCOL_HPP_

#include <string>
#include <awm/game/common.hpp>
#include <awm/game/lobby/tank.hpp>
#include <cereal/cereal.hpp>

namespace awm {
namespace game {
namespace lobby {

struct tank_dump {
	tank_ptr		tank_;

	template < typename Archive >
	void
	serialize(Archive& archive)
	{
		archive(
			cereal::make_nvp( "proto_id",   tank_->proto()     ),
			cereal::make_nvp( "name",       tank_->name()      ),
			cereal::make_nvp( "tier",       tank_->tier()      ),
			cereal::make_nvp( "dealer",     tank_->dealer()    ),
			cereal::make_nvp( "tank_type",  tank_->tank_type() )
		);
	}
};

struct tank_list_response {
	std::vector<tank_dump> list;

	template < typename Archive >
	void
	serialize(Archive& archive)
	{
		archive(
			CEREAL_NVP( list )
		);
	}
};


}  // namespace lobby
}  // namespace game
}  // namespace awm

#endif /* TIP_GAME_LOBBY_PROTOCOL_HPP_ */
