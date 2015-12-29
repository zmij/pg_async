/*
 * tank.cpp
 *
 *  Created on: Dec 25, 2015
 *      Author: zavyalov
 */

#include <tip/http/server/json_body_context.hpp>
#include <tip/db/pg/resultset.hpp>
#include <tip/log.hpp>

#include <awm/game/lobby/tank.hpp>
#include <awm/game/lobby/protocol.hpp>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <cereal/types/vector.hpp>

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
lobby_tanks_handler::checked_handle_request(tip::http::server::reply r)
{
	namespace ths = tip::http::server;
	local_log() << "tanks_handler";

	using tip::http::server::json_body_context;
	json_body_context& json = tip::http::server::use_context< json_body_context >(r);

	// stub data
	boost::uuids::nil_generator nilgen;
	std::vector<tank_ptr> tl = {
		std::make_shared<tank>(nilgen(), "VH_M1128"),
		std::make_shared<tank>(nilgen(), "VH_M1A1" )
	};

	std::vector<tank_dump> view;
	std::transform(tl.begin(), tl.end(), std::back_inserter(view),
		[]( tank_ptr t ) { return tank_dump{ t }; }
	);

	tank_list_response resp { view };
	resp.serialize(json.outgoing());

	r.done(tip::http::response_status::ok);
}

}  // namespace combat
}  // namespace game
}  // namespace tip
