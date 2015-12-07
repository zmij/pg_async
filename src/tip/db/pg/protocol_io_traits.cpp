/*
 * protocol_io_traits.cpp
 *
 *  Created on: Jul 19, 2015
 *      Author: zmij
 */

#include <tip/db/pg/protocol_io_traits.hpp>
#include <tip/db/pg/detail/protocol_parsers.hpp>
#include <iterator>
#include <set>

namespace tip {
namespace db {
namespace pg {
namespace io {
namespace traits {

namespace {

using namespace oids::type;

std::set< oid_type > BINARY_PARSERS {
	boolean, oids::type::bytea, int2, int4, int8, oid, tid, xid, cid
};
}  // namespace

void
register_parser_type(oids::type::oid_type oid)
{
	BINARY_PARSERS.insert(oid);
}

bool
has_binary_parser(oids::type::oid_type oid)
{
	return BINARY_PARSERS.count(oid);
}

}  // namespace traits

namespace {

const std::set< std::string > TRUE_LITERALS {
	"TRUE",
	"t",
	"true",
	"y",
	"yes",
	"on",
	"1"
};

const std::set< std::string > FALSE_LITERALS {
	"FALSE",
	"f",
	"false",
	"n",
	"no",
	"off",
	"0"
};

}  // namespace

bool
protocol_parser< bool, TEXT_DATA_FORMAT >::use_literal(std::string const& l)
{
	if (TRUE_LITERALS.count(l)) {
		base_type::value = true;
		return true;
	} else if (FALSE_LITERALS.count(l)) {
		base_type::value = false;
		return true;
	}
	return false;
}


bool
protocol_parser< bytea, TEXT_DATA_FORMAT >::operator()(std::istream& in)
{
	std::vector<byte> data;
	std::istream_iterator<char> b(in);
	std::istream_iterator<char> e;

	auto result = detail::bytea_parser().parse(b, e, std::back_inserter(data));
	if (result.first) {
		base_type::value.swap(data);
		return true;
	}
	return false;
}

bool
protocol_parser< bytea, TEXT_DATA_FORMAT >::operator()(buffer_type& buffer)
{
	std::vector<byte> data;
	auto result = detail::bytea_parser().parse(buffer.begin(), buffer.end(),
			std::back_inserter(data));
	if (result.first) {
		base_type::value.swap(data);
		return true;
	}
	return false;
}

}  // namespace io
}  // namespace pg
}  // namespace db
}  // namespace tip
