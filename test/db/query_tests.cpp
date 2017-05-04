/*
 * query_tests.cpp
 *
 *  Created on: Jul 27, 2015
 *      Author: zmij
 */

#include <tip/db/pg.hpp>

#include <tip/db/pg/log.hpp>

#include <gtest/gtest.h>

#include <iostream>
#include <fstream>
#include <thread>

#include "db/config.hpp"
#include "test-environment.hpp"

LOCAL_LOGGING_FACILITY(PGTEST, TRACE);

using namespace tip::db::pg;

TEST(QueryTest, QueryInlay)
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

        ASSERT_NO_THROW(db_service::add_connection(test::environment::test_database));
        connection_options opts = connection_options::parse(test::environment::test_database);
        {
            query (opts.alias, "create temporary table pg_async_test(b bigint)").run_async(
            [&](transaction_ptr c, resultset, bool){
                local_log() << "Query one finished";
                EXPECT_TRUE(c.get());
                query(c, "insert into pg_async_test values(1),(2),(3)").run_async(
                [&](transaction_ptr c1, resultset, bool){
                    local_log() << "Query two finished";
                    EXPECT_TRUE(c1.get());
                    query(c1, "select * from pg_async_test").run_async(
                    [&](transaction_ptr c2, resultset r, bool) {
                        local_log() << "Query three finished. Result columns "
                                << r.columns_size() << " rows " << r.size();
                        EXPECT_TRUE(c2.get());
                        res = r;
                        query(c2, "drop table pg_async_test").run_async(
                        [&](transaction_ptr c3, resultset, bool) {
                            local_log() << "Query four finished";
                            EXPECT_TRUE(c3.get());
                            timer.cancel();
                            //c->commit();
                            db_service::stop();
                        }, [](error::db_error const&){
                            FAIL();
                        });
                    }, [](error::db_error const&){
                        FAIL();
                    });
                }, [](error::db_error const&){
                    FAIL();
                });
            }, [](error::db_error const&){
                SUCCEED();
            });
        }
        db_service::run();

        local_log() << "Queries done";
        EXPECT_EQ(1, res.columns_size());
        EXPECT_EQ(3, res.size());
    }
}

struct call_when_done {
    ::std::function< void() > handler;

    explicit call_when_done(::std::function< void() > h) : handler{h} {}
    ~call_when_done()
    {
        local_log(logger::INFO) << "Call when done";
        if (handler) {
            try {
                handler();
            } catch (...) {}
        }
    }
};

TEST(QueryTest, QueryQueue)
{
    using namespace tip::db::pg;
    using atomic_counter = ::std::atomic<::std::size_t>;
    auto const num_requests = test::environment::num_requests;

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

        bool got_error = false;

        ASSERT_NO_THROW(db_service::add_connection(test::environment::test_database,
                test::environment::connection_pool));
        connection_options opts = connection_options::parse(test::environment::test_database);
        auto stop = ::std::make_shared< call_when_done >(
            [&timer]()
            {
                timer.cancel();
                db_service::stop();
            });

        for (int n = 0; n < num_requests; ++n) {
            auto insert_count = ::std::make_shared<atomic_counter>(0);
            auto insert2_count = ::std::make_shared<atomic_counter>(0);
            auto select_count = ::std::make_shared<atomic_counter>(0);

            db_service::begin(opts.alias,
            [&, insert_count, insert2_count, select_count, stop, n]( transaction_ptr tran ){
                query (tran, "create temporary table pg_async_test(b bigint)")(
                [&, n](transaction_ptr c, resultset, bool){
                    local_log(logger::DEBUG) << "Test " << n
                            << "Query one finished";
                    EXPECT_TRUE(c.get());
                }, [](error::db_error const&){
                    SUCCEED();
                });
                for (int i = 0; i < num_requests; ++i) {
                    local_log(logger::DEBUG) << "Test " << n
                            << " Query two enqueue (i=" << i << ")";
                    query(tran, "insert into pg_async_test values($1)", i)
                    ([&, i, insert_count, n](transaction_ptr c, resultset, bool){
                        local_log(logger::DEBUG) << "Test " << n
                                << " Query two finished (i="
                                << i << " insert_count=" << *insert_count << ")";
                        EXPECT_TRUE(c.get());
                        EXPECT_EQ(i, *insert_count) << "Callback called in correct order";
                        ++(*insert_count);
                    }, [](error::db_error const&){
                        FAIL();
                    });
                }
                for (int i = 0; i < num_requests; ++i) {
                    local_log(logger::DEBUG) << "Test " << n
                            << " Query three enqueue (i=" << i << ")";
                    query(tran, "select * from pg_async_test")
                    ([&, i, select_count, n](transaction_ptr c, resultset r, bool) {
                        local_log(logger::DEBUG) << "Test " << n
                                << " Query three finished (i="
                                << i << " select_count=" << *select_count << ")";
                        EXPECT_TRUE(c.get());
                        EXPECT_EQ(num_requests, r.size());
                        EXPECT_EQ(1, r.columns_size());
                        EXPECT_EQ(i, *select_count) << "Callback called in correct order";
                        ++(*select_count);
                        res = r;
                    }, [](error::db_error const&){
                        FAIL();
                    });
                }
                for (int i = 0; i < num_requests; ++i) {
                    local_log(logger::DEBUG) << "Test " << n
                        << " Query four enqueue (i=" << i << ")";
                    query(tran, "insert into pg_async_test values($1)", i)
                    ([&, i, insert2_count, n](transaction_ptr c, resultset, bool){
                        local_log(logger::DEBUG) << "Test " << n
                                << " Query four finished (i="
                                << i << " insert2_count=" << *insert2_count << ")";
                        EXPECT_TRUE(c.get());
                        EXPECT_EQ(i, *insert2_count) << "Callback called in correct order";
                        ++(*insert2_count);
                    }, [](error::db_error const&){
                        FAIL();
                    });
                }
                query(tran, "drop table pg_async_test")
                ([&, n](transaction_ptr c, resultset, bool) {
                    local_log(logger::DEBUG) << "Test " << n
                            << " Query five finished";
                    EXPECT_TRUE(c.get());
                }, [](error::db_error const&){
                    FAIL();
                });
                tran->commit_async();
                query(tran, "select 1")
                ([&](transaction_ptr c, resultset r, bool)
                 {
                    local_log(logger::DEBUG) << "Unexpected finish of query six. Resultset size " << r.size();
                 },
                 [&](error::db_error const&) { got_error = true; });
            }, [](error::db_error const&){});

        }
        // Additional service threads
        ::std::vector<::std::thread> threads;
        for (auto i = 0; i < test::environment::num_threads; ++i) {
            threads.emplace_back(
                []()
                {
                    db_service::run();
                });
        }

        db_service::run();

        for (auto& t : threads) {
            t.join();
        }

//        EXPECT_EQ(num_requests * num_requests, insert_count);
        EXPECT_EQ(num_requests, res.size());
        EXPECT_EQ(1, res.columns_size());
        EXPECT_TRUE(got_error);
    }
}

TEST(QueryTest, BasicResultParsing)
{
    using namespace tip::db::pg;
    if (!test::environment::test_database.empty() && !test::SCRIPT_SOURCE_DIR.empty()) {

        //resultset res;

        ASSERT_NO_THROW(db_service::add_connection(test::environment::test_database));
        std::string script_name = test::SCRIPT_SOURCE_DIR + "results-parse-test.sql";
        std::ifstream script(script_name);
        local_log() << "Script file name " << script_name;
        if (script) {
            ASIO_NAMESPACE::deadline_timer timer(*db_service::io_service(),
                    boost::posix_time::seconds(test::environment::deadline));
            timer.async_wait([&](asio_config::error_code const& ec){
                timer.cancel();
                if (!ec) {
                    local_log(logger::WARNING) << "Parse result set test timer expired";
                    db_service::stop();
                }
            });

            std::string script_str, line;
            while (getline(script, line)) {
                script_str += line;
            }
            connection_options opts = connection_options::parse(test::environment::test_database);
            query(opts.alias, script_str).run_async(
            [&](transaction_ptr c, resultset res, bool) {
                {
                    local_log() << "Received a resultset. Columns: " << res.columns_size()
                            << " rows: " << res.size() << " empty: " << res.empty();
                }
                EXPECT_TRUE(c.get());
                if (!res.empty()) {
                    EXPECT_TRUE(res);
                    EXPECT_FALSE(res.empty());
                    EXPECT_TRUE(res.size());
                    EXPECT_TRUE(res.columns_size());

                    for (int i = 0; i < res.columns_size(); ++i) {
                        field_description const& fd = res.field(i);
                        local_log() << "Field " << fd.name << " type "
                                << fd.type_oid << " type mod " << fd.type_mod;
                    }

                    for (resultset::const_iterator row = res.begin(); row != res.end(); ++row) {
                        auto local = local_log();
                        local << "Row " << (row - res.begin()) << ": ";

                        long id;
                        std::string ctime, ctimetz;
                        row.to(std::tie(id, ctime, ctimetz));

                        local << "id: " << id << " ctime: " << ctime << " ";

                        for (resultset::const_field_iterator f = row.begin(); f != row.end(); ++f) {
                            local << f.as< std::string >() << " ";
                        }
                    }
                }
            }, [&](error::db_error const&) {});

            db_service::run();
        }
    }
}
