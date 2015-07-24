/**
 * error.hpp
 *
 *  Created on: 16 июля 2015 г.
 *     @author: zmij
 */

#ifndef TIP_DB_PG_ERROR_HPP_
#define TIP_DB_PG_ERROR_HPP_

#include <stdexcept>
#include <tip/db/pg/sqlstates.hpp>

namespace tip {
namespace db {
namespace pg {

class db_error : public std::runtime_error {
public:
	explicit db_error( std::string const& what_arg );
	explicit db_error( char const* what_arg );
};

class connection_error : public db_error {
public:
	explicit connection_error( std::string const&);
	explicit connection_error( char const* what_arg );
};

class query_error : public db_error {
public:
	explicit query_error( std::string const&);
	explicit query_error( char const* what_arg );

	query_error(std::string const& message,
		std::string severity,
		std::string code,
		std::string detail
	);
	std::string		severity;
	std::string		code;
	std::string		detail;
	sqlstate::code	sqlstate;
};

class value_is_null : public db_error {
public:
	explicit value_is_null(std::string const& field_name);
};

}  // namespace pg
}  // namespace db
}  // namespace tip



#endif /* TIP_DB_PG_ERROR_HPP_ */
