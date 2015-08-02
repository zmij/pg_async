/*
 * errors_tests.cpp
 *
 *  Created on: Aug 2, 2015
 *      Author: zmij
 */

#include <tip/db/pg.hpp>

#include <tip/db/pg/log.hpp>

#include <gtest/gtest.h>

#include <iostream>
#include <fstream>

#include "db/config.hpp"
#include "test-environment.hpp"

namespace {
/** Local logging facility */
using namespace tip::log;

const std::string LOG_CATEGORY = "PGTEST";
logger::event_severity DEFAULT_SEVERITY = logger::TRACE;
local
local_log(logger::event_severity s = DEFAULT_SEVERITY)
{
	return local(LOG_CATEGORY, s);
}

}  // namespace
// For more convenient changing severity, eg local_log(logger::WARNING)
using tip::log::logger;
using namespace tip::db::pg;

TEST(ErrorTest, InvalidQueryError)
{
	if (!test::environment::test_database.empty()) {
		ASSERT_NO_THROW(db_service::add_connection(test::environment::test_database));
		connection_options opts = connection_options::parse(test::environment::test_database);

		int tran_err_callback = 0;
		int query_err_callback = 0;
		bool query_res_callback = false;
		ASSERT_NO_THROW(db_service::begin(opts.alias,
		[&](transaction_ptr tran){
			EXPECT_TRUE(tran.get());
			query(tran, "select * from __shouldnt_be_there_")(
			[&](transaction_ptr t, resultset r, bool) {
				local_log(logger::DEBUG) << "Query resultset callback fired";
				query_res_callback = true;
			},
			[&](db_error const& e) {
				local_log(logger::DEBUG) << "Query error callback fired";
				query_err_callback++;

				db_service::stop();
			});
		},
		[&](db_error const& e){
			local_log(logger::DEBUG) << "Transaction error callback fired: "
					<< e.what();
			tran_err_callback++;

			db_service::stop();
		}));

		ASSERT_NO_THROW(db_service::run());

		EXPECT_FALSE(query_res_callback);
		EXPECT_EQ(1, query_err_callback);
		EXPECT_EQ(1, tran_err_callback);
	}
}

TEST(ErrorTest, ExceptionInTranHanlder)
{
	if (!test::environment::test_database.empty()) {
		ASSERT_NO_THROW(db_service::add_connection(test::environment::test_database, 1));
		connection_options opts = connection_options::parse(test::environment::test_database);

		int tran_err_callback = 0;
		int query_err_callback = 0;
		bool query_res_callback = false;
		ASSERT_NO_THROW(db_service::begin(opts.alias,
		[&](transaction_ptr tran) {
			EXPECT_TRUE(tran.get());
			throw std::runtime_error("Bail out");
		},
		[&](db_error const& e) {
			local_log(logger::DEBUG) << "Transaction error callback fired: "
					<< e.what();
			tran_err_callback++;

			db_service::stop();
		}));

		ASSERT_NO_THROW(db_service::run());
		EXPECT_EQ(2, tran_err_callback);
	}
}

TEST(ErrorTest, ExceptionInTranErrorHandler)
{
	if (!test::environment::test_database.empty()) {
		ASSERT_NO_THROW(db_service::add_connection(test::environment::test_database, 1));
		connection_options opts = connection_options::parse(test::environment::test_database);

		int tran_err_callback = 0;
		int query_err_callback = 0;
		bool query_res_callback = false;
		ASSERT_NO_THROW(db_service::begin(opts.alias,
		[&](transaction_ptr tran) {
			EXPECT_TRUE(tran.get());
			throw std::runtime_error("Bail out");
		},
		[&](db_error const& e) {
			local_log(logger::DEBUG) << "Transaction error callback fired: "
					<< e.what();
			tran_err_callback++;

			db_service::stop();
			throw std::runtime_error("Will be eaten up");
		}));

		ASSERT_NO_THROW(db_service::run());
		EXPECT_EQ(2, tran_err_callback);
	}
}

TEST(ErrorTest, ExceptionInQueryResultsHanlder)
{
	if (!test::environment::test_database.empty()) {
		ASSERT_NO_THROW(db_service::add_connection(test::environment::test_database, 1));
		connection_options opts = connection_options::parse(test::environment::test_database);

		int tran_err_callback = 0;
		int query_err_callback = 0;
		bool query_res_callback = false;

		ASSERT_NO_THROW(db_service::begin(opts.alias,
		[&](transaction_ptr tran) {
			query(tran, "select * from pg_catalog.pg_type")(
			[&](transaction_ptr t, resultset r, bool) {
				local_log(logger::DEBUG) << "Query resultset callback fired";
				query_res_callback = true;
				throw std::runtime_error("Bail out");
			},
			[&](db_error const& e) {
				local_log(logger::DEBUG) << "Query error callback fired";
				query_err_callback++;

				db_service::stop();
			});
		},
		[&](db_error const& e) {
			local_log(logger::DEBUG) << "Transaction error callback fired: "
					<< e.what();
			tran_err_callback++;

			db_service::stop();
		}));

		ASSERT_NO_THROW(db_service::run());

		EXPECT_TRUE(query_res_callback);
		EXPECT_EQ(0, query_err_callback);
		EXPECT_EQ(2, tran_err_callback);
	}
}

TEST(ErrorTest, ExceptionInQueryErrorHanlder)
{
	if (!test::environment::test_database.empty()) {
		ASSERT_NO_THROW(db_service::add_connection(test::environment::test_database));
		connection_options opts = connection_options::parse(test::environment::test_database);

		int tran_err_callback = 0;
		int query_err_callback = 0;
		bool query_res_callback = false;
		ASSERT_NO_THROW(db_service::begin(opts.alias,
		[&](transaction_ptr tran){
			EXPECT_TRUE(tran.get());
			query(tran, "select * from __shouldnt_be_there_")(
			[&](transaction_ptr t, resultset r, bool) {
				local_log(logger::DEBUG) << "Query resultset callback fired";
				query_res_callback = true;
			},
			[&](db_error const& e) {
				local_log(logger::DEBUG) << "Query error callback fired";
				query_err_callback++;

				db_service::stop();
				throw std::runtime_error("Bail out");
			});
		},
		[&](db_error const& e){
			local_log(logger::DEBUG) << "Transaction error callback fired: "
					<< e.what();
			tran_err_callback++;

			db_service::stop();
		}));

		ASSERT_NO_THROW(db_service::run());

		EXPECT_FALSE(query_res_callback);
		EXPECT_EQ(1, query_err_callback);
		EXPECT_EQ(3, tran_err_callback);
	}
}
