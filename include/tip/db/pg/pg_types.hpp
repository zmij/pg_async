/*
 * pg_types.hpp
 *
 *  Created on: Jul 24, 2015
 *      Author: zmij
 */

#ifndef LIB_PG_ASYNC_INCLUDE_TIP_DB_PG_PG_TYPES_HPP_
#define LIB_PG_ASYNC_INCLUDE_TIP_DB_PG_PG_TYPES_HPP_

#include <iosfwd>

namespace tip {
namespace db {
namespace pg {

namespace oids {

namespace type {
enum oid_type {
	boolean				= 16,
	bytea				= 17,
	char_				= 18,
	name				= 19,
	int8				= 20,
	int2				= 21,
	int2_vector			= 22,
	int4				= 23,
	regproc				= 24,
	text				= 25,
	oid					= 26,
	tid					= 27,
	xid					= 28,
	cid					= 29,
	oid_vector			= 30,
	json				= 114,
	xml					= 142,
	pg_node_tree		= 194,
	pg_ddl_command		= 32,
	point				= 600,
	lseg				= 601,
	path				= 602,
	box					= 603,
	polygon				= 604,
	line				= 628,
	float4				= 700,
	float8				= 701,
	abstime				= 702,
	reltime				= 703,
	tinterval			= 704,
	unknown				= 705,
	circle				= 718,
	cash				= 790,
	macaddr				= 829,
	inet				= 869,
	cidr				= 650,
	int2_array			= 1005,
	int4_array			= 1007,
	text_array			= 1009,
	oid_array			= 1028,
	float4_array		= 1021,
	acl_item			= 1033,
	cstring_array		= 1263,
	bpchar				= 1042,
	varchar				= 1043,
	date				= 1082,
	time				= 1083,
	timestamp			= 1114,
	timestamptz			= 1184,
	interval			= 1186,
	timetz				= 1266,
	bit					= 1560,
	varbit				= 1562,
	numeric				= 1700,
	refcursor			= 1790,
	regprocedure		= 2202,
	regoper				= 2203,
	regoperator			= 2204,
	regclass			= 2205,
	regtype				= 2206,
	regrole				= 4096,
	regtypearray		= 2211,
	uuid				= 2950,
	lsn					= 3220,
	tsvector			= 3614,
	gtsvector			= 3642,
	tsquery				= 3615,
	regconfig			= 3734,
	regdictionary		= 3769,
	jsonb				= 3802,
	int4_range			= 3904,
	record				= 2249,
	record_array		= 2287,
	cstring				= 2275,
	any					= 2276,
	any_array			= 2277,
	void_				= 2278,
	trigger				= 2279,
	evttrigger			= 3838,
	language_handler	= 2280,
	internal			= 2281,
	opaque				= 2282,
	any_element			= 2283,
	any_non_array		= 2776,
	any_enum			= 3500,
	fdw_handler			= 3115,
	any_range			= 3831
};

std::ostream&
operator << (std::ostream& out, oid_type val);
std::istream&
operator >> (std::ostream& in, oid_type& val);


}  // namespace type
namespace type_class {
enum code {
	base				= 'b',
	composite			= 'c',
	domain				= 'd',
	enumerated			= 'e',
	pseudo				= 'p',
	range				= 'r'
};
}  // namespace type_class
namespace type_category {
enum code {
	invalid				= 0,
	array				= 'A',
	boolean				= 'B',
	composite			= 'C',
	datetime			= 'D',
	enumeration			= 'E',
	geometric			= 'G',
	network				= 'I',
	numeric				= 'N',
	pseudotype			= 'P',
	range_category		= 'R',
	string				= 'S',
	timespan			= 'T',
	user				= 'U',
	bitstring			= 'V',
	unknown				= 'X'
};
}  // type_category

}  // namespace oids

}  // namespace pg
}  // namespace db
}  // namespace tip


#endif /* LIB_PG_ASYNC_INCLUDE_TIP_DB_PG_PG_TYPES_HPP_ */
