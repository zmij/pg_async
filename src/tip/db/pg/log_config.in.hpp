/*
 * log_config.in.hpp
 *
 *  Created on: Nov 5, 2015
 *      Author: zmij
 */

#ifndef LIB_PG_ASYNC_SRC_TIP_DB_PG_LOG_CONFIG_IN_HPP_
#define LIB_PG_ASYNC_SRC_TIP_DB_PG_LOG_CONFIG_IN_HPP_

#include <tip/db/pg/log.hpp>

namespace tip {
namespace db {
namespace pg {
namespace config {

namespace log = ::psst::log;

const log::logger::event_severity SERVICE_LOG = log::logger::@PGASYNC_LOG_SERVICE@;
const log::logger::event_severity CONNECTION_LOG = log::logger::@PGASYNC_LOG_CONNECTION@;
const log::logger::event_severity QUERY_LOG = log::logger::@PGASYNC_LOG_QUERY@;
const log::logger::event_severity INTERNALS_LOG = log::logger::@PGASYNC_LOG_INTERNALS@;


}  // namespace config
}  // namespace pg
}  // namespace db
}  // namespace tip

#endif /* LIB_PG_ASYNC_SRC_TIP_DB_PG_LOG_CONFIG_IN_HPP_ */
