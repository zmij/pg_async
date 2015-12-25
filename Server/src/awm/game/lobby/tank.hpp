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
	void
	read(tip::db::pg::resultset::row);
private:
	boost::uuids::uuid  uid_;
	std::string			proto_;
};

class lobby_tanks_handler : public tip::http::server::request_handler {
public:
	lobby_tanks_handler();
	virtual ~lobby_tanks_handler();
private:
	virtual void
	do_handle_request(tip::http::server::reply);
};

}  // namespace lobby
}  // namespace game
}  // namespace awm


#endif /* AWM_GAME_LOBBY_TANK_HPP_ */
