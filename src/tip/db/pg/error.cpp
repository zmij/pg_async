/*
 * db_error.cpp
 *
 *  Created on: 16 июля 2015 г.
 *     @author: zmij
 */

#include <tip/db/pg/error.hpp>

namespace tip {
namespace db {
namespace pg {

db_error::db_error(std::string const& what_arg)
	: std::runtime_error(what_arg), sqlstate(sqlstate::unknown_code)
{
}

db_error::db_error(char const* what_arg)
	: std::runtime_error(what_arg), sqlstate(sqlstate::unknown_code)
{
}

db_error::db_error(std::string const& message,
		std::string severity,
		std::string code,
		std::string detail)
	: std::runtime_error(message), severity(severity), code(code), detail(detail),
	  sqlstate(sqlstate::code_to_state(code))
{
}

connection_error::connection_error(std::string const& what_arg)
	: db_error(what_arg)
{
}

connection_error::connection_error(char const* what_arg)
	: db_error(what_arg)
{
}

query_error::query_error(std::string const& what_arg)
	: db_error(what_arg)
{
}

query_error::query_error(char const* what_arg)
	: db_error(what_arg)
{
}

query_error::query_error(std::string const& message,
		std::string severity,
		std::string code,
		std::string detail)
	: db_error(message, severity, code, detail)
{
}

client_error::client_error(std::string const& what_arg)
	: db_error(what_arg)
{
}

client_error::client_error(char const* what_arg)
	: db_error(what_arg)
{
}

client_error::client_error(std::exception const& ex)
	: db_error(std::string("Client thrown exception: ") + ex.what())
{
}

value_is_null::value_is_null(std::string const& field_name)
	: db_error("Value in field " + field_name + " is null")
{
}

}  // namespace pg
}  // namespace db
}  // namespace tip
