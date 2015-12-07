/*
 * request_generate.cpp
 *
 *  Created on: Aug 19, 2015
 *      Author: zmij
 */

#include <tip/http/common/grammar/request_generate.hpp>

namespace tip {
namespace http {
namespace grammar {
namespace gen {

method_grammar::method_grammar()
{
	add
		(GET,		"GET")
		(HEAD,		"HEAD")
		(POST,		"POST")
		(PUT,		"PUT")
		(DELETE,	"DELETE")
		(OPTIONS,	"OPTIONS")
		(TRACE,		"TRACE")
		(CONNECT,	"CONNECT")
		(PATCH,		"PATCH")
	;
}

}  // namespace gen
}  // namespace grammar
}  // namespace http
}  // namespace tip

