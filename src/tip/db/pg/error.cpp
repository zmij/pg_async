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
	: std::runtime_error(what_arg)
{
}

db_error::db_error(char const* what_arg)
	: std::runtime_error(what_arg)
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
	: db_error(what_arg), sqlstate(unknown_code)
{
}

query_error::query_error(char const* what_arg)
	: db_error(what_arg), sqlstate(unknown_code)
{
}

query_error::query_error(std::string const& message,
		std::string severity,
		std::string code,
		std::string detail)
	: db_error(message), severity(severity), code(code), detail(detail),
	  sqlstate(code_to_state(code))
{
}

value_is_null::value_is_null(std::string const& field_name)
	: db_error("Value in field " + field_name + " is null")
{
}

}  // namespace pg
}  // namespace db
}  // namespace tip
