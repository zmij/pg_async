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
namespace error {

db_error::db_error(std::string const& what_arg)
	: std::runtime_error(what_arg), sqlstate(sqlstate::unknown_code)
{
}

db_error::db_error(char const* what_arg)
	: std::runtime_error(what_arg), sqlstate(sqlstate::unknown_code)
{
}

db_error::db_error(std::string const& message,
		std::string s,
		std::string c,
		std::string d)
	: std::runtime_error(message), severity(s), code(c), detail(d),
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
		std::string s, std::string c, std::string d)
	: db_error(message, s, c, d)
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

}  // namespace error
}  // namespace pg
}  // namespace db
}  // namespace tip
