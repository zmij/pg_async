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

LOCAL_LOGGING_FACILITY(PGTEST, TRACE);
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
            [&](transaction_ptr, resultset, bool) {
                local_log(logger::DEBUG) << "Query resultset callback fired";
                query_res_callback = true;
            },
            [&](error::db_error const&) {
                local_log(logger::DEBUG) << "Query error callback fired";
                query_err_callback++;

                db_service::stop();
            });
        },
        [&](error::db_error const& e){
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
//        int query_err_callback = 0;
//        bool query_res_callback = false;
        ASSERT_NO_THROW(db_service::begin(opts.alias,
        [&](transaction_ptr tran) {
            EXPECT_TRUE(tran.get());
            throw std::runtime_error("Bail out");
        },
        [&](error::db_error const& e) {
            local_log(logger::DEBUG) << "Transaction error callback fired: "
                    << e.what();
            tran_err_callback++;

            db_service::stop();
        }));

        ASSERT_NO_THROW(db_service::run());
        EXPECT_LE(1, tran_err_callback);
    }
}

TEST(ErrorTest, ExceptionInTranErrorHandler)
{
    if (!test::environment::test_database.empty()) {
        ASSERT_NO_THROW(db_service::add_connection(test::environment::test_database, 1));
        connection_options opts = connection_options::parse(test::environment::test_database);

        int tran_err_callback = 0;
//        int query_err_callback = 0;
//        bool query_res_callback = false;
        ASSERT_NO_THROW(db_service::begin(opts.alias,
        [&](transaction_ptr tran) {
            EXPECT_TRUE(tran.get());
            throw std::runtime_error("Bail out");
        },
        [&](error::db_error const& e) {
            local_log(logger::DEBUG) << "Transaction error callback fired: "
                    << e.what();
            tran_err_callback++;

            db_service::stop();
            throw std::runtime_error("Will be eaten up");
        }));

        ASSERT_NO_THROW(db_service::run());
        EXPECT_LE(1, tran_err_callback);
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
            [&](transaction_ptr, resultset, bool) {
                local_log(logger::DEBUG) << "Query resultset callback fired";
                query_res_callback = true;
                throw std::runtime_error("Bail out");
            },
            [&](error::db_error const&) {
                local_log(logger::DEBUG) << "Query error callback fired";
                query_err_callback++;

                db_service::stop();
            });
        },
        [&](error::db_error const& e) {
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
            [&](transaction_ptr, resultset, bool) {
                local_log(logger::DEBUG) << "Query resultset callback fired";
                query_res_callback = true;
            },
            [&](error::db_error const&) {
                local_log(logger::DEBUG) << "Query error callback fired";
                query_err_callback++;

                throw std::runtime_error("Bail out");
            });
        },
        [&](error::db_error const& e){
            local_log(logger::DEBUG) << "Transaction error callback fired: "
                    << e.what();
            tran_err_callback++;

            db_service::stop();
        }));

        ASSERT_NO_THROW(db_service::run());

        EXPECT_FALSE(query_res_callback);
        EXPECT_EQ(1, query_err_callback);
        // FIXME Leave only 1 error callback fire
        EXPECT_LE(1, tran_err_callback); // Some error callback may fail to fire due to io_service stop
    }
}

TEST(ErrorTest, BreakQueryQueue)
{
    using namespace tip::db::pg;
    if (!test::environment::test_database.empty()) {

        ASIO_NAMESPACE::deadline_timer timer(*db_service::io_service(),
                boost::posix_time::seconds(test::environment::deadline));
        timer.async_wait([&](asio_config::error_code const& ec){
            if (!ec) {
                #ifdef WITH_TIP_LOG
                local_log(logger::WARNING) << "Run query test timer expired";
                #endif
                db_service::stop();
            }
        });

        resultset res;

        std::vector<bool> query_resultsets(3, false);
        int transaction_error = 0;

        ASSERT_NO_THROW(db_service::add_connection(test::environment::test_database,
                test::environment::connection_pool));
        connection_options opts = connection_options::parse(test::environment::test_database);
        {
            db_service::begin(opts.alias,
            [&]( transaction_ptr tran ){
                query (tran, "create temporary table pg_async_test(b bigint)")(
                [&](transaction_ptr c, resultset, bool){
                    local_log(logger::DEBUG) << "Query one finished";
                    EXPECT_TRUE(c.get());
                    query_resultsets[0] = true;
                    throw std::runtime_error("Break the queue");
                }, [](error::db_error const&){
                });
                query(tran, "select * from pg_async_test")
                ([&](transaction_ptr c, resultset, bool) {
                    local_log(logger::DEBUG) << "Query two finished";
                    EXPECT_TRUE(c.get());
                    query_resultsets[1] = true;
                }, [](error::db_error const&){
                });
                query(tran, "drop table pg_async_test")
                ([&](transaction_ptr c, resultset, bool) {
                    local_log(logger::DEBUG) << "Query three finished";
                    EXPECT_TRUE(c.get());
                    query_resultsets[2] = true;

                }, [](error::db_error const&){
                });
                tran->commit_async();
            }, [&](error::db_error const& e){
                local_log(logger::DEBUG) << "Transaction error callback fired: "
                        << e.what();
                transaction_error++;
                timer.cancel();
                db_service::stop();
            });
        }
        db_service::run();
        EXPECT_TRUE(query_resultsets[0]);
        EXPECT_FALSE(query_resultsets[1]);
        EXPECT_FALSE(query_resultsets[2]);
        EXPECT_EQ(2, transaction_error);
    }
}

TEST(ErrorTest, FailQueryQueue)
{
    using namespace tip::db::pg;
    if (!test::environment::test_database.empty()) {

        ASIO_NAMESPACE::deadline_timer timer(*db_service::io_service(),
                boost::posix_time::seconds(test::environment::deadline));
        timer.async_wait([&](asio_config::error_code const& ec){
            if (!ec) {
                #ifdef WITH_TIP_LOG
                local_log(logger::WARNING) << "Run query test timer expired";
                #endif
                db_service::stop();
            }
        });

        resultset res;

        std::vector<bool> query_resultsets(3, false);
        int transaction_error = 0;
        int query_error = 0;

        ASSERT_NO_THROW(db_service::add_connection(test::environment::test_database,
                test::environment::connection_pool));
        connection_options opts = connection_options::parse(test::environment::test_database);
        {
            db_service::begin(opts.alias,
            [&]( transaction_ptr tran ){
                // Syntax error
                query (tran, "create temporary table pg_async_test(bigint)")(
                [&](transaction_ptr c, resultset, bool){
                    local_log(logger::DEBUG) << "Query one finished";
                    EXPECT_TRUE(c.get());
                    query_resultsets[0] = true;
                }, [&](error::db_error const&){
                    query_error++;
                });
                query(tran, "select * from pg_async_test")
                ([&](transaction_ptr c, resultset, bool) {
                    local_log(logger::DEBUG) << "Query two finished";
                    EXPECT_TRUE(c.get());
                    query_resultsets[1] = true;
                }, [&](error::db_error const&){
                    query_error++;
                });
                query(tran, "drop table pg_async_test")
                ([&](transaction_ptr c, resultset, bool) {
                    local_log(logger::DEBUG) << "Query three finished";
                    EXPECT_TRUE(c.get());
                    query_resultsets[2] = true;

                }, [&](error::db_error const&){
                    query_error++;
                });
                tran->commit_async();
            }, [&](error::db_error const& e){
                local_log(logger::DEBUG) << "Transaction error callback fired: "
                        << e.what();
                transaction_error++;
                timer.cancel();
                db_service::stop();
            });
        }
        db_service::run();
        EXPECT_FALSE(query_resultsets[0]);
        EXPECT_FALSE(query_resultsets[1]);
        EXPECT_FALSE(query_resultsets[2]);
        EXPECT_EQ(1, query_error);
        EXPECT_EQ(1, transaction_error);
    }
}

