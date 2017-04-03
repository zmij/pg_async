/*
 * fiber_query_test.cpp
 *
 *  Created on: Mar 29, 2017
 *      Author: zmij
 */

#include <gtest/gtest.h>

#ifndef WITH_BOOST_FIBERS
#define WITH_BOOST_FIBERS
#endif

#include <tip/db/pg.hpp>
#include <tip/db/pg/log.hpp>

#include <pushkin/asio/fiber/shared_work.hpp>
#include <boost/fiber/all.hpp>

#include "db/config.hpp"
#include "test-environment.hpp"

namespace tip {
namespace db {
namespace pg {
namespace test {

LOCAL_LOGGING_FACILITY(PGTEST, TRACE);

TEST(FiberTest, TheOnly)
{
    if (!environment::test_database.empty()) {
        ::boost::fibers::use_scheduling_algorithm<
             ::psst::asio::fiber::shared_work >( db_service::io_service() );

        ASSERT_NO_THROW(db_service::add_connection(environment::test_database,
                            environment::connection_pool));
        connection_options opts = connection_options::parse(environment::test_database);

        const auto fiber_cnt    = environment::num_requests;
        const auto thread_cnt   = environment::num_threads;

        auto fib_fn = [&](boost::fibers::barrier& barrier) {
            try {
                auto trx = db_service::begin(opts.alias);
                local_log() << "Transaction started";
                EXPECT_TRUE(trx.get());

                EXPECT_NO_THROW(query(trx, "create temporary table pg_async_test(b bigint)")());
                local_log() << "Query one finished";
                EXPECT_NO_THROW(query(trx, "insert into pg_async_test values(1),(2),(3)")());
                local_log() << "Query two finished";
                auto res = query(trx, "select * from pg_async_test")();
                EXPECT_TRUE(res);
                EXPECT_EQ(1, res.columns_size());
                EXPECT_EQ(3, res.size());
                local_log() << "Query tree finished";
                EXPECT_NO_THROW(query(trx, "drop table pg_async_test")());
                local_log() << "Query four finished";
                EXPECT_NO_THROW(trx->commit());
                local_log() << "Transaction committed";
            } catch (::std::exception const& e) {
                local_log(logger::ERROR) << "Exception while running test " << e.what();
            }

            if (barrier.wait()) {
                db_service::stop();
            }

            local_log() << "Fiber exit";
        };

        ::std::vector< ::std::thread > threads;
        threads.reserve(thread_cnt);
        boost::fibers::barrier b(fiber_cnt * thread_cnt);

        for(auto i = 0; i < thread_cnt; ++i) {
            threads.emplace_back([&](){
                ::boost::fibers::use_scheduling_algorithm<
                     ::psst::asio::fiber::shared_work >( db_service::io_service() );
                for (auto i = 0; i < fiber_cnt; ++i) {
                    fiber{ fib_fn, ::std::ref(b) }.detach();
                }
                db_service::run();
                local_log() << "Thread exit";
            });
        }

        for (auto& t : threads) {
            t.join();
        }
    }
}

} /* namespace test */
} /* namespace pg */
} /* namespace db */
} /* namespace tip */
