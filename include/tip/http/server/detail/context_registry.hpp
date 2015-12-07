/*
 * context_registry.hpp
 *
 *  Created on: Aug 28, 2015
 *      Author: zmij
 */

#ifndef TIP_HTTP_SERVER_DETAIL_CONTEXT_REGISTRY_HPP_
#define TIP_HTTP_SERVER_DETAIL_CONTEXT_REGISTRY_HPP_

#include <tip/http/server/reply.hpp>

namespace tip {
namespace http {
namespace server {
namespace detail {

template < typename Context >
struct context_id : reply::id {};
template < typename T >
class typeid_wrapper {};

class context_registry {
public:
	context_registry();
	virtual ~context_registry();

	template < typename Context >
	void
	add_context(Context* new_context);

	template < typename Context >
	bool
	has_context() const;

	template < typename Context >
	Context&
	use_context();

	void
	clear();
private:
	static void
	init_key(reply::context::key& key, reply::id const& id);

	template < typename Context >
	static void
	init_key(reply::context::key& key, context_id<Context> const&);

	template < typename Context >
	static reply::context*
	create( reply const& );
private:
	friend class ::tip::http::server::reply;
	void
	set_reply_impl(reply::pimpl);

	struct auto_context_ptr {
		reply::context* ptr_;
		~auto_context_ptr() { delete ptr_; }
	};
	typedef reply::context* (*factory_type)(reply const&);
	void
	do_add_context(reply::context::key const& key, reply::context* ctx);
	bool
	do_has_context(reply::context::key const& key) const;
	reply::context*
	do_use_context(reply::context::key const& key, factory_type factory);
private:
	reply::context* first_context_;
	reply::weak_pimpl rep_;
};

} /* namespace detail */
} /* namespace server */
} /* namespace http */
} /* namespace tip */

#include <tip/http/server/detail/context_registry.inl>

#endif /* TIP_HTTP_SERVER_DETAIL_CONTEXT_REGISTRY_HPP_ */
