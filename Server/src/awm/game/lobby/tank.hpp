/*
 * tank.hpp
 *
 *  Created on: Dec 25, 2015
 *      Author: zavyalov
 */

#ifndef AWM_GAME_LOBBY_TANK_HPP_
#define AWM_GAME_LOBBY_TANK_HPP_

#include <tip/db/pg/resultset.hpp>

#include <boost/uuid/uuid.hpp>
#include <string>

namespace awm {
namespace game {
namespace lobby {

class tank {
public:
	tank() {}
	tank(tip::db::pg::resultset::row r);

private:
	boost::uuids::uuid  uid_;
	std::string			proto_id_;
};

}  // namespace lobby
}  // namespace game
}  // namespace awm


#endif /* AWM_GAME_LOBBY_TANK_HPP_ */
