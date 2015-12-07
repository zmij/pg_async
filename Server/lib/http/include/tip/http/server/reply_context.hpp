/*
 * reply_context.hpp
 *
 *  Created on: Aug 28, 2015
 *      Author: zmij
 */

#ifndef TIP_HTTP_SERVER_DETAIL_REPLY_CONTEXT_HPP_
#define TIP_HTTP_SERVER_DETAIL_REPLY_CONTEXT_HPP_

#include <tip/http/server/detail/context_registry.hpp>

namespace tip {
namespace http {
namespace server {

template < typename Context >
void
add_context(reply& r, Context* ctx)
{
	static_assert(std::is_base_of< reply::context, Context >::value,
			"Context must be derived from reply::context");
	(void)static_cast<reply::id*>(&Context::id);
	r.context_registry().template add_context(ctx);
}

template < typename Context >
bool
has_context(reply& r)
{
	static_assert(std::is_base_of< reply::context, Context >::value,
			"Context must be derived from reply::context");
	(void)static_cast<reply::id*>(&Context::id);
	return r.context_registry().template has_context<Context>();
}

template < typename Context >
Context&
use_context(reply& r)
{
	static_assert(std::is_base_of< reply::context, Context >::value,
			"Context must be derived from reply::context");
	(void)static_cast<reply::id*>(&Context::id);
	return r.context_registry().template use_context<Context>();
}

} /* namespace server */
} /* namespace http */
} /* namespace tip */


#endif /* TIP_HTTP_SERVER_DETAIL_REPLY_CONTEXT_HPP_ */
