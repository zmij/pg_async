/*
 * pg_types.cpp
 *
 *  Created on: Jul 24, 2015
 *      Author: zmij
 */

#include <tip/db/pg/pg_types.hpp>
#include <map>
#include <string>
#include <iostream>

namespace tip {
namespace db {
namespace pg {

namespace oids {

namespace type {
namespace {
	const std::map< oid_type, std::string > OID_TYPE_TO_STRING {
		{ boolean, "boolean" },
		{ bytea, "bytea" },
		{ char_, "char" },
		{ name, "name" },
		{ int8, "int8" },
		{ int2, "int2" },
		{ int2_vector, "int2_vector" },
		{ int4, "int4" },
		{ regproc, "regproc" },
		{ text, "text" },
		{ oid, "oid" },
		{ tid, "tid" },
		{ xid, "xid" },
		{ cid, "cid" },
		{ oid_vector, "oid_vector" },
		{ json, "json" },
		{ xml, "xml" },
		{ pg_node_tree, "pg_node_tree" },
		{ pg_ddl_command, "pg_ddl_command" },
		{ point, "point" },
		{ lseg, "lseg" },
		{ path, "path" },
		{ box, "box" },
		{ polygon, "polygon" },
		{ line, "line" },
		{ float4, "float4" },
		{ float8, "float8" },
		{ abstime, "abstime" },
		{ reltime, "reltime" },
		{ tinterval, "tinterval" },
		{ unknown, "unknown" },
		{ circle, "circle" },
		{ cash, "cash" },
		{ macaddr, "macaddr" },
		{ inet, "inet" },
		{ cidr, "cidr" },
		{ int2_array, "int2_array" },
		{ int4_array, "int4_array" },
		{ text_array, "text_array" },
		{ oid_array, "oid_array" },
		{ float4_array, "float4_array" },
		{ acl_item, "acl_item" },
		{ cstring_array, "cstring_array" },
		{ bpchar, "bpchar" },
		{ varchar, "varchar" },
		{ date, "date" },
		{ time, "time" },
		{ timestamp, "timestamp" },
		{ timestamptz, "timestamptz" },
		{ interval, "interval" },
		{ timetz, "timetz" },
		{ bit, "bit" },
		{ varbit, "varbit" },
		{ numeric, "numeric" },
		{ refcursor, "refcursor" },
		{ regprocedure, "regprocedure" },
		{ regoper, "regoper" },
		{ regoperator, "regoperator" },
		{ regclass, "regclass" },
		{ regtype, "regtype" },
		{ regrole, "regrole" },
		{ regtypearray, "regtypearray" },
		{ uuid, "uuid" },
		{ lsn, "lsn" },
		{ tsvector, "tsvector" },
		{ gtsvector, "gtsvector" },
		{ tsquery, "tsquery" },
		{ regconfig, "regconfig" },
		{ regdictionary, "regdictionary" },
		{ jsonb, "jsonb" },
		{ int4_range, "int4_range" },
		{ record, "record" },
		{ record_array, "record_array" },
		{ cstring, "cstring" },
		{ any, "any" },
		{ any_array, "any_array" },
		{ void_, "void" },
		{ trigger, "trigger" },
		{ evttrigger, "evttrigger" },
		{ language_handler, "language_handler" },
		{ internal, "internal" },
		{ opaque, "opaque" },
		{ any_element, "any_element" },
		{ any_non_array, "any_non_array" },
		{ any_enum, "any_enum" },
		{ fdw_handler, "fdw_handler" },
		{ any_range, "any_range" },
	}; // OID_TYPE_TO_STRING
	const std::map< std::string, oid_type > STRING_TO_OID_TYPE {
		{ "boolean", boolean },
		{ "bytea", bytea },
		{ "char", char_ },
		{ "name", name },
		{ "int8", int8 },
		{ "int2", int2 },
		{ "int2_vector", int2_vector },
		{ "int4", int4 },
		{ "regproc", regproc },
		{ "text", text },
		{ "oid", oid },
		{ "tid", tid },
		{ "xid", xid },
		{ "cid", cid },
		{ "oid_vector", oid_vector },
		{ "json", json },
		{ "xml", xml },
		{ "pg_node_tree", pg_node_tree },
		{ "pg_ddl_command", pg_ddl_command },
		{ "point", point },
		{ "lseg", lseg },
		{ "path", path },
		{ "box", box },
		{ "polygon", polygon },
		{ "line", line },
		{ "float4", float4 },
		{ "float8", float8 },
		{ "abstime", abstime },
		{ "reltime", reltime },
		{ "tinterval", tinterval },
		{ "unknown", unknown },
		{ "circle", circle },
		{ "cash", cash },
		{ "macaddr", macaddr },
		{ "inet", inet },
		{ "cidr", cidr },
		{ "int2_array", int2_array },
		{ "int4_array", int4_array },
		{ "text_array", text_array },
		{ "oid_array", oid_array },
		{ "float4_array", float4_array },
		{ "acl_item", acl_item },
		{ "cstring_array", cstring_array },
		{ "bpchar", bpchar },
		{ "varchar", varchar },
		{ "date", date },
		{ "time", time },
		{ "timestamp", timestamp },
		{ "timestamptz", timestamptz },
		{ "interval", interval },
		{ "timetz", timetz },
		{ "bit", bit },
		{ "varbit", varbit },
		{ "numeric", numeric },
		{ "refcursor", refcursor },
		{ "regprocedure", regprocedure },
		{ "regoper", regoper },
		{ "regoperator", regoperator },
		{ "regclass", regclass },
		{ "regtype", regtype },
		{ "regrole", regrole },
		{ "regtypearray", regtypearray },
		{ "uuid", uuid },
		{ "lsn", lsn },
		{ "tsvector", tsvector },
		{ "gtsvector", gtsvector },
		{ "tsquery", tsquery },
		{ "regconfig", regconfig },
		{ "regdictionary", regdictionary },
		{ "jsonb", jsonb },
		{ "int4_range", int4_range },
		{ "record", record },
		{ "record_array", record_array },
		{ "cstring", cstring },
		{ "any", any },
		{ "any_array", any_array },
		{ "void", void_ },
		{ "trigger", trigger },
		{ "evttrigger", evttrigger },
		{ "language_handler", language_handler },
		{ "internal", internal },
		{ "opaque", opaque },
		{ "any_element", any_element },
		{ "any_non_array", any_non_array },
		{ "any_enum", any_enum },
		{ "fdw_handler", fdw_handler },
		{ "any_range", any_range },
	}; // STRING_TO_OID_TYPE
} // namespace

// Generated output operator
std::ostream&
operator << (std::ostream& out, oid_type val)
{
	std::ostream::sentry s (out);
	if (s) {
		auto f = OID_TYPE_TO_STRING.find(val);
		if (f != OID_TYPE_TO_STRING.end()) {
			out << f->second;
		} else {
			out << "oid_type " << (int)val;
		}
	}
	return out;
}
// Generated input operator
std::istream&
operator >> (std::istream& in, oid_type& val)
{
	std::istream::sentry s (in);
	if (s) {
		std::string name;
		if (in >> name) {
			auto f = STRING_TO_OID_TYPE.find(name);
			if (f != STRING_TO_OID_TYPE.end()) {
				val = f->second;
			} else {
				in.setstate(std::ios_base::failbit);
			}
		}
	}
	return in;
}

}  // namespace type

namespace type_class {

namespace {
	const std::map< code, std::string > CODE_TO_STRING {
		{ base, "base" },
		{ composite, "composite" },
		{ domain, "domain" },
		{ enumerated, "enumerated" },
		{ pseudo, "pseudo" },
		{ range, "range" },
	}; // CODE_TO_STRING
	const std::map< std::string, code > STRING_TO_CODE {
		{ "base", base },
		{ "composite", composite },
		{ "domain", domain },
		{ "enumerated", enumerated },
		{ "pseudo", pseudo },
		{ "range", range },
	}; // STRING_TO_CODE
} // namespace

// Generated output operator
std::ostream&
operator << (std::ostream& out, code val)
{
	std::ostream::sentry s (out);
	if (s) {
		auto f = CODE_TO_STRING.find(val);
		if (f != CODE_TO_STRING.end()) {
			out << f->second;
		} else {
			out << "type " << (int)val;
		}
	}
	return out;
}
// Generated input operator
std::istream&
operator >> (std::istream& in, code& val)
{
	std::istream::sentry s (in);
	if (s) {
		std::string name;
		if (in >> name) {
			auto f = STRING_TO_CODE.find(name);
			if (f != STRING_TO_CODE.end()) {
				val = f->second;
			} else {
				in.setstate(std::ios_base::failbit);
			}
		}
	}
	return in;
}

}  // namespace type_class

namespace type_category {

namespace {
	const std::map< code, std::string > CODE_TO_STRING {
		{ invalid, "invalid" },
		{ array, "array" },
		{ boolean, "boolean" },
		{ composite, "composite" },
		{ datetime, "datetime" },
		{ enumeration, "enumeration" },
		{ geometric, "geometric" },
		{ network, "network" },
		{ numeric, "numeric" },
		{ pseudotype, "pseudotype" },
		{ range_category, "range_category" },
		{ string, "string" },
		{ timespan, "timespan" },
		{ user, "user" },
		{ bitstring, "bitstring" },
		{ unknown, "unknown" },
	}; // CODE_TO_STRING
	const std::map< std::string, code > STRING_TO_CODE {
		{ "invalid", invalid },
		{ "array", array },
		{ "boolean", boolean },
		{ "composite", composite },
		{ "datetime", datetime },
		{ "enumeration", enumeration },
		{ "geometric", geometric },
		{ "network", network },
		{ "numeric", numeric },
		{ "pseudotype", pseudotype },
		{ "range_category", range_category },
		{ "string", string },
		{ "timespan", timespan },
		{ "user", user },
		{ "bitstring", bitstring },
		{ "unknown", unknown },
	}; // STRING_TO_CODE
} // namespace

// Generated output operator
std::ostream&
operator << (std::ostream& out, code val)
{
	std::ostream::sentry s (out);
	if (s) {
		auto f = CODE_TO_STRING.find(val);
		if (f != CODE_TO_STRING.end()) {
			out << f->second;
		} else {
			out << "category " << (int)val;
		}
	}
	return out;
}
// Generated input operator
std::istream&
operator >> (std::istream& in, code& val)
{
	std::istream::sentry s (in);
	if (s) {
		std::string name;
		if (in >> name) {
			auto f = STRING_TO_CODE.find(name);
			if (f != STRING_TO_CODE.end()) {
				val = f->second;
			} else {
				in.setstate(std::ios_base::failbit);
			}
		}
	}
	return in;
}

}  // namespace type_category

}  // namespace oids

}  // namespace pg
}  // namespace db
}  // namespace tip


