/*
 * tokenizer_test.cpp
 *
 *  Created on: Sep 28, 2015
 *      Author: zmij
 */

#include <gtest/gtest.h>

#include <tip/db/pg.hpp>
#include <tip/db/pg/detail/tokenizer_base.hpp>
#include <tip/db/pg/io/vector.hpp>
#include <tip/db/pg/io/array.hpp>

#include "db/config.hpp"
#include "test-environment.hpp"

#include <boost/iostreams/stream_buffer.hpp>
#include <boost/iostreams/device/back_inserter.hpp>

typedef std::vector< std::string > token_list;
typedef std::pair< std::string, token_list > test_param_type;

class ArrayParseTest : public ::testing::TestWithParam< test_param_type > {
public:
	typedef tip::db::pg::io::detail::tokenizer_base<
			std::string::const_iterator, '{', '}' > string_tokenizer_type;
};

TEST_P(ArrayParseTest, Parses)
{
	ParamType param = GetParam();
	std::vector< std::string > tokens;
	std::string::const_iterator current = param.first.begin();
	string_tokenizer_type(current, param.first.end(), std::back_inserter(tokens));
	EXPECT_EQ(param.second.size(), tokens.size());
	EXPECT_EQ(param.second, tokens);
}

INSTANTIATE_TEST_CASE_P(ArraySupport, ArrayParseTest,
	::testing::Values(
		test_param_type{ "{}", {} },
		test_param_type{ "{,}", {"", ""} },
		test_param_type{ "{0,1,2,3}", {"0", "1", "2", "3"} },
		test_param_type{
			R"~({"foo",'bar','baz''bar'})~",
			{ "foo", "bar", "baz'bar" }
		}
	)
);

TEST(ArraySupport, VectorIOTest)
{
	using namespace tip::db::pg;
	typedef boost::iostreams::stream_buffer<
			boost::iostreams::back_insert_device< std::vector<char> >>
			vector_buff_type;
	typedef std::vector< byte > buffer_type;
	typedef std::vector< int > int_vector;
	typedef std::array< int, 3 > int_array;

	{
		buffer_type buffer;
		int_vector vals { 1, 2, 3, 4 };
		io::protocol_write< TEXT_DATA_FORMAT >(buffer, vals);
		std::string check(buffer.begin(), buffer.end());
		EXPECT_EQ("{1,2,3,4}", check);
	}

	{
		buffer_type buffer;
		int_vector vals;
		{
			vector_buff_type vbuff(buffer);
			std::ostream os(&vbuff);
			os << "{5,6,7,8}";
		}
		io::protocol_read< TEXT_DATA_FORMAT >(buffer.begin(), buffer.end(), vals);
		EXPECT_EQ((int_vector{ 5, 6, 7,  8 }), vals);
	}

	{
		buffer_type buffer;
		int_array vals { 1, 2, 3 };
		io::protocol_write< TEXT_DATA_FORMAT >(buffer, vals);
		std::string check(buffer.begin(), buffer.end());
		EXPECT_EQ("{1,2,3}", check);
	}

	{
		buffer_type buffer;
		int_array vals;
		{
			vector_buff_type vbuff(buffer);
			std::ostream os(&vbuff);
			os << "{5,6,7,8}";
		}
		io::protocol_read< TEXT_DATA_FORMAT >(buffer.begin(), buffer.end(), vals);
		EXPECT_EQ((int_array{ 5, 6, 7 }), vals);
	}
	{
		buffer_type buffer;
		int_array vals;
		{
			vector_buff_type vbuff(buffer);
			std::ostream os(&vbuff);
			os << "{5,6}";
		}
		io::protocol_read< TEXT_DATA_FORMAT >(buffer.begin(), buffer.end(), vals);
		EXPECT_EQ((int_array{ 5, 6, 0 }), vals);
	}
}

TEST(ArraySupport, DatabaseRoundtrip)
{
	using namespace tip::db::pg;
	if (!test::environment::test_database.empty()) {
		db_service::add_connection(test::environment::test_database);
		connection_options opts = connection_options::parse(test::environment::test_database);

		typedef std::vector< integer > int_vector;

		int_vector in_v { 1, 2, 3, 4 };
		resultset res;

		db_service::begin(opts.alias,
		[&](transaction_ptr tran) {
			query(tran,
				"create temporary table pg_async_array_support(a integer[])")(
			[](transaction_ptr t, resultset, bool) {
			},
			[](error::db_error const&) {}
			);
			query(tran, "insert into pg_async_array_support values ($1::integer[])", in_v)(
			[](transaction_ptr t, resultset, bool) {},
			[](error::db_error const&) {}
			);
			query(tran, "select a from pg_async_array_support")(
			[&res](transaction_ptr t, resultset r, bool) {
				res = r;
				db_service::stop();
			},
			[](error::db_error const&) {
				db_service::stop();
			}
			);
		},
		[](error::db_error const&) {
			db_service::stop();
		}
		);
		db_service::run();
		ASSERT_FALSE(res.empty());
		ASSERT_EQ(1, res.size());
		ASSERT_EQ(1, res.columns_size());
		int_vector out_v;
		res.front().to(out_v);
		EXPECT_EQ(in_v, out_v);
	}
}
