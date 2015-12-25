/*
 * tank.cpp
 *
 *  Created on: Dec 25, 2015
 *      Author: zavyalov
 */

#include <awm/game/lobby/tank.hpp>
#include <tip/db/pg/resultset.hpp>
#include <boost/uuid/uuid.hpp>
#include <tip/log.hpp>

namespace awm {
namespace game {
namespace lobby {

LOCAL_LOGGING_FACILITY(TANK, TRACE);

tank::tank(tip::db::pg::resultset::row r)
{
}

}  // namespace combat
}  // namespace game
}  // namespace tip
