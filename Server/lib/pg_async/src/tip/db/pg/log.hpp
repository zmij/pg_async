/*
 * log.hpp
 *
 *  Created on: Jul 19, 2015
 *      Author: zmij
 */

#ifndef LIB_PG_ASYNC_SRC_TIP_DB_PG_LOG_HPP_
#define LIB_PG_ASYNC_SRC_TIP_DB_PG_LOG_HPP_

#ifdef WITH_TIP_LOG
#include <tip/log.hpp>
#else
#include <tip/db/pg/null_logger.hpp>
#endif

#include <tip/db/pg/log_config.hpp>

#endif /* LIB_PG_ASYNC_SRC_TIP_DB_PG_LOG_HPP_ */
