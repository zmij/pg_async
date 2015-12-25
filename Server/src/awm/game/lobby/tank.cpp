/*
 * tank.cpp
 *
 *  Created on: Dec 25, 2015
 *      Author: zavyalov
 */

#include <awm/game/lobby/tank.hpp>
#include <tip/db/pg/resultset.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <tip/log.hpp>

#include <string>

namespace awm {
namespace game {
namespace lobby {

LOCAL_LOGGING_FACILITY(TANK, TRACE);

namespace {

const tip::db::pg::dbalias db = "main"_db;
const std::string FIELDS      = "uid, proto_id";

const std::string CREATE_SQL  = R"~(
insert into lobby.tanks(uid, proto_id)
values($1, $2)
returning )~" + FIELDS;

}  // namespace

tank::tank(tip::db::pg::resultset::row r)
{
	read(r);
}

void
tank::read(tip::db::pg::resultset::row r)
{
	r.to(uid_, proto_);
}

lobby_tanks_handler::lobby_tanks_handler()
{
	// TODO constructor stub
}

lobby_tanks_handler::~lobby_tanks_handler()
{
	// TODO destructor stub
}

void
lobby_tanks_handler::do_handle_request(tip::http::server::reply r)
{
	r.server_error(tip::http::response_status::not_implemented);
}

}  // namespace combat
}  // namespace game
}  // namespace tip
