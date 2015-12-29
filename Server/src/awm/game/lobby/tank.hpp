/*
 * tank.hpp
 *
 *  Created on: Dec 25, 2015
 *      Author: zavyalov
 */

#ifndef AWM_GAME_LOBBY_TANK_HPP_
#define AWM_GAME_LOBBY_TANK_HPP_

#include <tip/db/pg/resultset.hpp>
#include <tip/http/server/request_handler.hpp>
#include <tip/http/server/prerequisite_handler.hpp>

#include <awm/game/auth/session.hpp>
#include <boost/uuid/uuid.hpp>
#include <string>

namespace awm {
namespace game {
namespace lobby {

enum tank_types {
	MBT     = 1,        // Main battle tank
	LT      = 2,        // Light tank
	AFV     = 3,        // Armored fighting vehicle
	SPG     = 4,        // Self-propelled gun
	TD      = 5         // Tank destroyer
};

class tank {
public:
	tank() {}
	tank(tip::db::pg::resultset::row r);
	tank(boost::uuids::uuid uid, std::string proto) : uid_(uid), proto_(proto) {}

	std::string proto(void)     { return proto_; }
	std::string name(void)      { return proto_; }
	int         tier(void)      { return 1; }
	std::string dealer(void)    { return "leopard"; }
	tank_types  tank_type(void) { return MBT; }


private:
	void
	read(tip::db::pg::resultset::row);
private:
	boost::uuids::uuid  uid_;
	std::string			proto_;
};

typedef std::shared_ptr< tank > tank_ptr;

class lobby_tanks_handler : public tip::http::server::prerequisite_handler< authn::authorized > {
public:
	lobby_tanks_handler();
	virtual ~lobby_tanks_handler();
private:
	virtual void
	checked_handle_request(tip::http::server::reply);
};

}  // namespace lobby
}  // namespace game
}  // namespace awm


#endif /* AWM_GAME_LOBBY_TANK_HPP_ */
