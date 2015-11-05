/**
 * extended-query-mode.cpp
 *
 *  Created on: Jul 23, 2015
 *      Author: zmij
 */

#include <tip/db/pg/connection.hpp>
#include <tip/db/pg/resultset.hpp>
#include <tip/db/pg/resultset.inl>
#include <tip/db/pg/database.hpp>
#include <tip/db/pg/query.hpp>
#include <tip/db/pg/query.inl>
#include <tip/db/pg/error.hpp>

#include <tip/db/pg/detail/protocol.hpp>

#include <tip/db/pg/detail/basic_connection.hpp>
#include <tip/db/pg/detail/connection_pool.hpp>
#include <tip/db/pg/transaction.hpp>

#include <tip/db/pg/log.hpp>

#include <iostream>
#include <iomanip>
#include <thread>
#include <chrono>
#include <fstream>
#include <sstream>

#include "db/config.hpp"

LOCAL_LOGGING_FACILITY(PGTEST, TRACE);

namespace {

std::string test_database;

}  // namespace

int
test_execute_prepared()
{
	if (!test_database.empty()) {
		using namespace tip::db::pg;
		connection_options opts = connection_options::parse(test_database);
		local_log(logger::INFO) << "Extended query test";
		boost::asio::io_service io_service;
		bool transaction_error = false;
		int tran_count = 0;

		std::vector< oids::type::oid_type > param_types;

		std::vector<char> params;
		tip::db::pg::detail::write_params(param_types, params, 10, 20);

		connection_ptr conn(connection::create(io_service,
		[&](connection_ptr c) {
			if (!tran_count) {
				c->begin_transaction(
				[&](transaction_ptr c_lock){
					tran_count++;
					(*c_lock)->execute_prepared("select * from pg_catalog.pg_type where typelem > $1 limit $2",
					param_types, params,
					[&](transaction_ptr c, resultset r, bool complete) {
						local_log() << "Received a resultset columns: " << r.columns_size()
								<< " rows: " << r.size()
								<< " completed: " << std::boolalpha << complete;
						if (complete)
							(*c)->terminate();
					}, [](db_error const&) {}, c_lock);
				},
				[&](db_error const&){
					transaction_error = true;
				}, true);
			}
		}, [] (connection_ptr c) {
		}, [](connection_ptr c, connection_error const& ec) {
			ec.what();
		},  opts, {
			{"client_encoding", "UTF8"},
			{"application_name", "pg_async"},
			//{"client_min_messages", "debug5"}
		}));
		io_service.run();
	}
	return 0;
}

int
main(int argc, char* argv[])
{
	try {
		logger::min_severity(logger::TRACE);
		logger::use_colors(true);
		if (argc > 1) {
			test_database = argv[1];
			test_execute_prepared();
		}
	} catch (std::exception const& e) {
		std::cerr << "Exception: " << e.what() << "\n";
	} catch (...) {
		std::cerr << "Unexpected exception\n";
	}
	return 0;
}
