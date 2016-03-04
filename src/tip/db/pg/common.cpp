/**
 * common.cpp
 *
 *  Created on: 11 июля 2015 г.
 *     @author: zmij
 */

#include <tip/db/pg/common.hpp>
#include <iostream>
#include <exception>
#include <stdexcept>
#include <sstream>

namespace tip {
namespace db {
namespace pg {

struct connect_string_parser {
	enum state_type {
		alias,
		schema,
		schema_slash1,
		schema_slash2,
		user,
		password,
		url,
		database,
		done,
	} state;



	connect_string_parser() : state(alias)
	{
	}

	connection_options
	operator ()(std::string const& literal) {
		std::string current;
		connection_options opts;
		for (auto p = literal.begin(); p != literal.end(); ++p) {
			switch (state) {
				case schema_slash1:
				case schema_slash2:
					if (*p != '/')
						throw std::runtime_error("invalid connection string");
					state = (state_type)(state + 1);
					break;
				default: {
					if (*p == '=') {
						if (state == alias) {
							opts.alias.swap(current);
							state = schema;
						}
					} else if (*p == ':') {
						// current string is a scheme or a user or host
						switch (state) {
							case alias:
							case schema:
								opts.schema.swap(current);
								state = schema_slash1;
								break;
							case user:
								opts.user.swap(current);
								state = password;
								break;
							case url:
								current.push_back(*p);
								break;
							default:
								current.push_back(*p);
								break;
						}
					} else if (*p == '@') {
						// current string is a user or a password
						switch (state) {
							case user:
								opts.user.swap(current);
								state = url;
								break;
							case password:
								opts.password.swap(current);
								state = url;
								break;
							default:
								current.push_back(*p);
								break;
						}
					} else if (*p == '[') {
						switch (state) {
							case user:
							case url:
								opts.uri.swap(current);
								state = database;
								break;
							case password:
								opts.uri.swap(opts.user);
								opts.uri.push_back(':');
								opts.uri += current;
								current.clear();
								state = database;
								break;
							default:
								current.push_back(*p);
								break;
						}
					} else if (*p == ']') {
						// current string is database
						if (state == database) {
							opts.database.swap(current);
							state = done;
						} else {
							current.push_back(*p);
						}
					} else if (!std::isspace(*p)) {
						// FIXME check what we are pushing
						current.push_back(*p);
					}
				}
			}
		}
		return opts;
	}
};

void
connection_options::generate_alias()
{
	std::ostringstream al;
	al << user << "@" << uri << "[" << database << "]";
	alias = al.str();
}

connection_options
connection_options::parse(std::string const& literal)
{
	return connect_string_parser()(literal);
}

}  // namespace pg
}  // namespace db
}  // namespace tip

tip::db::pg::dbalias
operator"" _db(const char* v, size_t)
{
	return tip::db::pg::dbalias{ std::string{v} };
}

tip::db::pg::connection_options
operator"" _pg(const char* literal, size_t)
{
	return tip::db::pg::connection_options::parse(literal);
}

