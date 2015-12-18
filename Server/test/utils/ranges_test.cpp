/*
 * ranges_test.cpp
 *
 *  Created on: Dec 16, 2015
 *      Author: zmij
 */

#include <gtest/gtest.h>
#include <awm/util/range.hpp>
#include <sstream>

namespace awm {
namespace util {
namespace test {

TEST(Range, ShortRange)
{
	typedef range<uint16_t> uint16_range;

	uint16_range r1 { 4444, 4744, range_ends::inclusive };
	std::ostringstream os;
	os << r1;
	EXPECT_EQ("[4444,4744]", os.str());
	std::istringstream is("[4444,4744]");
	uint16_range r2;
	bool read_from_stream = (bool)(is >> r2);
	EXPECT_TRUE(read_from_stream);
	EXPECT_EQ(r1, r2);
}

}  // namespace test
}  // namespace util
}  // namespace awm
