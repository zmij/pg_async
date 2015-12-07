/*
 * context_registry.inl
 *
 *  Created on: Aug 28, 2015
 *      Author: zmij
 */

#ifndef TIP_HTTP_SERVER_DETAIL_CONTEXT_REGISTRY_INL_
#define TIP_HTTP_SERVER_DETAIL_CONTEXT_REGISTRY_INL_

#include <tip/http/server/detail/context_registry.hpp>

namespace tip {
namespace http {
namespace server {
namespace detail {

template < typename Context >
void
context_registry::add_context(Context* new_context)
{
	reply::context::key key;
	init_key(key, Context::id);
	do_add_context(key, new_context);
}

template < typename Context >
bool
context_registry::has_context() const
{
	reply::context::key key;
	init_key(key, Context::id);
	return do_has_context(key);
}

template < typename Context >
Context&
context_registry::use_context()
{
	reply::context::key key;
	init_key(key, Context::id);
	factory_type factory = &context_registry::create<Context>;
	return *static_cast<Context*>(do_use_context(key, factory));
}

template < typename Context >
void
context_registry::init_key(reply::context::key& key, context_id<Context> const&)
{
	key.type_info_ = &typeid(typeid_wrapper<Context>);
	key.id_ = nullptr;
}

template < typename Context >
reply::context*
context_registry::create(reply const& rep)
{
	return new Context(rep);
}

} /* namespace detail */
} /* namespace server */
} /* namespace http */
} /* namespace tip */


#endif /* TIP_HTTP_SERVER_DETAIL_CONTEXT_REGISTRY_INL_ */
