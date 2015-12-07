/*
 * context_registry.cpp
 *
 *  Created on: Aug 28, 2015
 *      Author: zmij
 */

#include <tip/http/server/detail/context_registry.hpp>

namespace tip {
namespace http {
namespace server {
namespace detail {

void
context_registry::init_key(reply::context::key& key, reply::id const& id)
{
	key.type_info_ = nullptr;
	key.id_ = &id;
}

context_registry::context_registry() :
		first_context_(nullptr), rep_()
{
}

context_registry::~context_registry()
{
	clear();
}

void
context_registry::clear()
{
	while (first_context_) {
		reply::context* next = first_context_->next_;
		delete first_context_;
		first_context_ = next;
	}
}

void
context_registry::set_reply_impl(reply::pimpl pi)
{
	rep_ = pi;
}

void
context_registry::do_add_context(reply::context::key const& key, reply::context* new_ctx)
{
	reply::context* context = first_context_;
	while (context) {
		if (context->key_ == key)
			throw std::logic_error("Context already exists");
		context = context->next_;
	}
	new_ctx->key_ = key;
	new_ctx->next_ = first_context_;
	first_context_ = new_ctx;
}

bool
context_registry::do_has_context(reply::context::key const& key) const
{
	reply::context* context = first_context_;
	while (context) {
		if (context->key_ == key)
			return true;
		context = context->next_;
	}
	return false;
}

reply::context*
context_registry::do_use_context(reply::context::key const& key, factory_type factory)
{
	reply::context* context = first_context_;
	while (context) {
		if (context->key_ == key)
			return context;
		context = context->next_;
	}

	reply::pimpl pi = rep_.lock();
	if (!pi) {
		throw std::runtime_error("Reply was already destroyed");
	}
	reply::context* new_ctx = factory( reply(pi) );
	new_ctx->key_ = key;
	new_ctx->next_ = first_context_;
	first_context_ = new_ctx;
	return first_context_;
}

} /* namespace detail */
} /* namespace server */
} /* namespace http */
} /* namespace tip */
