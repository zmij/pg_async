/*
 * uuid_io_test.cpp
 *
 *  Created on: Feb 18, 2017
 *      Author: sergey.fedorov
 */

#include <gtest/gtest.h>

#include <tip/db/pg.hpp>
#include <tip/db/pg/query.hpp>
#include <tip/db/pg/io/uuid.hpp>

#include <boost/uuid/string_generator.hpp>
#include <boost/uuid/random_generator.hpp>

#include "db/config.hpp"
#include "test-environment.hpp"

namespace tip {
namespace db {
namespace pg {
namespace test {

using uuid = ::boost::uuids::uuid;

namespace {

::boost::uuids::string_generator str_gen;
::boost::uuids::random_generator rand_gen;

}  /* namespace  */

class UUIDIOTest : public ::testing::TestWithParam< uuid > {
};

TEST_P(UUIDIOTest, DBRoundtrip)
{
    if (!test::environment::test_database.empty()) {
        db_service::add_connection(test::environment::test_database);
        connection_options opts = connection_options::parse(test::environment::test_database);

        uuid test_val = GetParam();
        resultset res_txt;
        resultset res_bin;

        db_service::begin(opts.alias,
        [&](transaction_ptr tran) {
            query(tran,
                "create temporary table pg_async_ts_test( id uuid)")(
            [](transaction_ptr, resultset, bool){},
            [](error::db_error const&) {});

            query(tran, "insert into pg_async_ts_test values($1)", test_val)(
            [](transaction_ptr, resultset, bool){},
            [](error::db_error const&) {}
            );
            query(tran, "select id from pg_async_ts_test")(
            [&res_txt](transaction_ptr, resultset r, bool){
                res_txt = r;
            },
            [](error::db_error const&) {
            });
            query(tran, "select id from pg_async_ts_test limit $1", 1)(
            [&res_bin](transaction_ptr, resultset r, bool){
                res_bin = r;
            },
            [](error::db_error const&) {
            });
            tran->commit([](){
                db_service::stop();
            });
        },
        [](error::db_error const&) {
            db_service::stop();
        });

        db_service::run();

        uuid out_v;
        ASSERT_FALSE(res_txt.empty());
        ASSERT_EQ(1, res_txt.size());
        ASSERT_EQ(1, res_txt.columns_size());

        res_txt.front().to(out_v);
        EXPECT_EQ(test_val, out_v) << "Successfully parsed text protocol";

        ASSERT_FALSE(res_bin.empty());
        ASSERT_EQ(1, res_bin.size());
        ASSERT_EQ(1, res_bin.columns_size());

        res_bin.front().to(out_v);
        EXPECT_EQ(test_val, out_v) << "Successfully parsed binary protocol";
    };
}

INSTANTIATE_TEST_CASE_P(IOTest, UUIDIOTest,
    ::testing::Values(
        uuid{{0}},
        str_gen("bd6258b8-45cf-4b65-b327-24d395bbe60b"),
        rand_gen()
    )
);

} // namespace test
}  /* namespace pg */
}  /* namespace db */
}  /* namespace tip */
