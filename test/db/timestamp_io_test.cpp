/*
 * timestamp_io_test.cpp
 *
 *  Created on: Mar 24, 2016
 *      Author: zmij
 */

#include <gtest/gtest.h>

#include <tip/db/pg.hpp>
#include <tip/db/pg/query.hpp>
#include <tip/db/pg/io/boost_date_time.hpp>

#include <boost/date_time/c_local_time_adjustor.hpp>

#include "db/config.hpp"
#include "test-environment.hpp"

using namespace tip::db::pg;
using boost::posix_time::ptime;
using boost::gregorian::date;
using boost::posix_time::time_duration;

class DateTimeIOTest : public ::testing::TestWithParam< ::std::pair< ::std::string, boost::posix_time::ptime > > {
public:
    static ParamType
    make_test_data(::std::string const& f, boost::posix_time::ptime const& s)
    {
        return std::make_pair(f, s);
    }
};

TEST_P(DateTimeIOTest, Parse)
{
    ParamType test_val = GetParam();

    boost::posix_time::ptime val;
    io::protocol_read< TEXT_DATA_FORMAT >(test_val.first.begin(), test_val.first.end(), val);
    EXPECT_EQ( test_val.second, val );
}

TEST_P(DateTimeIOTest, DBRoundtrip)
{
    using local_adjust = boost::date_time::c_local_adjustor<ptime>;
    if (!test::environment::test_database.empty()) {
        db_service::add_connection(test::environment::test_database);
        connection_options opts = connection_options::parse(test::environment::test_database);

        ParamType test_val = GetParam();
        resultset res_txt;
        resultset res_bin;

        db_service::begin(opts.alias,
        [&](transaction_ptr tran) {
            query(tran,
                "create temporary table pg_async_ts_test( t timestamp with time zone )")(
            [](transaction_ptr, resultset, bool){},
            [](error::db_error const&) {});

            query(tran, "insert into pg_async_ts_test values($1)", test_val.second)(
            [](transaction_ptr, resultset, bool){},
            [](error::db_error const&) {}
            );
            query(tran, "select t from pg_async_ts_test where $1=$1", 1)(
            [&res_txt](transaction_ptr, resultset r, bool){
                res_txt = r;
            },
            [](error::db_error const&) {
            });
            query(tran, "select t from pg_async_ts_test where $1=$1", 1)(
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

        ASSERT_FALSE(res_txt.empty());
        ASSERT_EQ(1, res_txt.size());
        ASSERT_EQ(1, res_txt.columns_size());

        boost::posix_time::ptime out_v;
        res_txt.front().to(out_v);
        EXPECT_EQ(test_val.second, out_v) << "Successfully parsed text protocol";

        ASSERT_FALSE(res_bin.empty());
        ASSERT_EQ(1, res_bin.size());
        ASSERT_EQ(1, res_bin.columns_size());

        res_bin.front().to(out_v);
        if (!out_v.is_not_a_date_time())
            out_v = local_adjust::utc_to_local(out_v);
        EXPECT_EQ(test_val.second, out_v) << "Successfully parsed binary protocol";
    }
}

INSTANTIATE_TEST_CASE_P(IOTest, DateTimeIOTest,
    ::testing::Values(
        DateTimeIOTest::make_test_data("", {}),
        DateTimeIOTest::make_test_data(
            "2016-03-24 18:00:00.0+03",
            { date{ 2016, boost::gregorian::Mar, 24 }, time_duration{ 18, 0, 0 } }),
        DateTimeIOTest::make_test_data(
            "2000-01-01 00:00:00.0+03",
            { date{ 2000, boost::gregorian::Jan, 1 }, time_duration{ 0, 0, 0 } })
    )
);

