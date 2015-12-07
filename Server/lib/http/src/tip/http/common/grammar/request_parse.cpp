/*
 * request_grammar.cpp
 *
 *  Created on: Aug 18, 2015
 *      Author: zmij
 */

#include <tip/http/common/grammar/request_parse.hpp>

namespace tip {
namespace http {
namespace grammar {
namespace parse {

method_grammar::method_grammar()
{
	add
		("GET",		GET)
		("HEAD",	HEAD)
		("POST",	POST)
		("PUT",		PUT)
		("DELETE",	DELETE)
		("OPTIONS",	OPTIONS)
		("TRACE",	TRACE)
		("CONNECT",	CONNECT)
		("PATCH",	PATCH)
	;
}

}  // namespace parse
}  // namespace grammar
}  // namespace http
}  // namespace tip
