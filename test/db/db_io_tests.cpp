/*
 * db_io_tests.cpp
 *
 *  Created on: Jul 27, 2015
 *      Author: zmij
 */

#include <tip/db/pg/protocol_io_traits.hpp>
#include <tip/db/pg/query.hpp>

#include <tip/db/pg/log.hpp>

#include <gtest/gtest.h>


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


class BoolParseTest : public ::testing::TestWithParam< std::pair< std::string, bool > > {
public:
	typedef std::pair< std::string, bool > test_pair;
};
class InvalidBoolParseTest : public ::testing::TestWithParam< std::string > {
};

TEST_P(BoolParseTest, Parses)
{
	typedef tip::util::input_iterator_buffer buffer_type;
	test_pair curr = GetParam();

	bool val;
	EXPECT_NE( curr.first.begin(),
		io::protocol_read< TEXT_DATA_FORMAT >(curr.first.begin(),
			curr.first.end(), val));
	EXPECT_EQ(curr.second, val);
}

TEST_P(InvalidBoolParseTest, DoesntParse)
{
	typedef tip::util::input_iterator_buffer buffer_type;
	std::string curr = GetParam();

	bool val = true;
	EXPECT_EQ( curr.begin(),
		io::protocol_read< TEXT_DATA_FORMAT >(curr.begin(), curr.end(), val));
	EXPECT_TRUE(val);

}

INSTANTIATE_TEST_CASE_P(
		IOTest,
		BoolParseTest,
		::testing::Values(
				BoolParseTest::test_pair{ "TRUE", true },
				BoolParseTest::test_pair{ "t", true },
				BoolParseTest::test_pair{ "true", true },
				BoolParseTest::test_pair{ "y", true },
				BoolParseTest::test_pair{ "yes", true },
				BoolParseTest::test_pair{ "on", true },
				BoolParseTest::test_pair{ "1", true },
				BoolParseTest::test_pair{ "FALSE", false },
				BoolParseTest::test_pair{ "f", false },
				BoolParseTest::test_pair{ "false", false },
				BoolParseTest::test_pair{ "n", false },
				BoolParseTest::test_pair{ "no", false },
				BoolParseTest::test_pair{ "off", false },
				BoolParseTest::test_pair{ "0", false }
		)
);

INSTANTIATE_TEST_CASE_P(
		IOTest,
		InvalidBoolParseTest,
		::testing::Values("foo", "bar", "trololo")
);

class ByteaTextParseTest :
		public ::testing::TestWithParam< std::pair< std::string, size_t > > {
public:
	typedef std::pair< std::string, size_t > test_pair;
};
class InvalieByteaTextParseTest :
		public ::testing::TestWithParam< std::string > {
};

TEST_P(ByteaTextParseTest, Parses)
{
	test_pair curr = GetParam();
	bytea val;

	EXPECT_NE(
			curr.first.begin(),
			io::protocol_read< TEXT_DATA_FORMAT >(curr.first.begin(),
					curr.first.end(), val));
	EXPECT_EQ(curr.second, val.size());
}

TEST_P(InvalieByteaTextParseTest, DoesntParse)
{
	std::string curr = GetParam();
	bytea val { 1, 2, 3, 4 };

	EXPECT_EQ(
			curr.begin(),
			io::protocol_read< TEXT_DATA_FORMAT >(curr.begin(), curr.end(), val));
	EXPECT_EQ(4, val.size()); // Not modified
}

INSTANTIATE_TEST_CASE_P(IOTest,
		ByteaTextParseTest,
		::testing::Values(
			ByteaTextParseTest::test_pair{ "\\xdeadbeef", 4 },
			ByteaTextParseTest::test_pair{ "\\x5c784445414442454546", 10 },
			ByteaTextParseTest::test_pair{ "\\x", 0 }
));

INSTANTIATE_TEST_CASE_P(IOTest,
		InvalieByteaTextParseTest,
		::testing::Values(
			"\\xdeadbee",
			"\\x5c78444g414442454546",
			"\\",
			""
));

class QueryParamsWriteTest : public ::testing::TestWithParam< std::tuple<
		std::vector< oids::type::oid_type >,
		std::vector< char >,
		size_t
	> > {
public:
	typedef std::vector< char > buffer_type;
	typedef buffer_type::iterator buffer_iterator;
	typedef std::tuple< type_oid_sequence, buffer_type, size_t > test_data;

	template < typename ... T >
	static test_data
	make_test_data(T const& ... args)
	{
		type_oid_sequence param_types;
		buffer_type buffer;
		tip::db::pg::detail::write_params( param_types, buffer, args ... );
		return std::make_tuple(param_types, buffer, sizeof ... (T));
	}
};

TEST_P(QueryParamsWriteTest, Buffers)
{
	using namespace tip::db::pg;
	type_oid_sequence param_types;
	buffer_type buffer;
	size_t expected_param_count;
	std::tie(param_types, buffer, expected_param_count) = GetParam();
	EXPECT_FALSE(param_types.empty());
	EXPECT_EQ(expected_param_count, param_types.size());

	EXPECT_FALSE(buffer.empty());

	buffer_iterator p = buffer.begin();
	buffer_iterator e = buffer.end();
	smallint data_format_count(0);

	p = io::protocol_read< BINARY_DATA_FORMAT >(p, e, data_format_count);
	EXPECT_EQ(expected_param_count, data_format_count);
	smallint df(0);
	for (smallint dfc = 0; dfc < data_format_count && p != e; ++dfc) {
		buffer_iterator c = io::protocol_read< BINARY_DATA_FORMAT >(p, e, df);
		EXPECT_NE(p, c);
		p = c;
	}
	smallint param_count;
	p = io::protocol_read< BINARY_DATA_FORMAT >(p, e, param_count);
	EXPECT_EQ(expected_param_count, param_count);
	for (smallint pno = 0; pno < param_count && p != e; ++pno) {
		integer param_size(0);
		p = io::protocol_read< BINARY_DATA_FORMAT >(p, e, param_size);
		EXPECT_GT(param_count, 0);
		p += param_size;
	}
	EXPECT_EQ(e, p);
}

INSTANTIATE_TEST_CASE_P(
	IOTest,
	QueryParamsWriteTest,
	::testing::Values(
		//QueryParamsWriteTest::make_test_data(), @todo Accept zero params
		QueryParamsWriteTest::make_test_data((integer)42),
		QueryParamsWriteTest::make_test_data((integer)42, (smallint)42),
		QueryParamsWriteTest::make_test_data((integer)42, (smallint)42, (bigint)420),
		QueryParamsWriteTest::make_test_data((integer)42, (smallint)42, (bigint)420, 3.1415926f)
		//QueryParamsWriteTest::make_test_data(42, 42, 420, 3.1415926f, "bla")
));
