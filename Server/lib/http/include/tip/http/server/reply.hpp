/**
 * request_reply.h
 *
 *  Created on: 28 авг. 2015 г.
 *      Author: zmij
 */

#ifndef TIP_HTTP_SERVER_REPLY_HPP_
#define TIP_HTTP_SERVER_REPLY_HPP_

#include <tip/http/common/request_fwd.hpp>
#include <tip/http/common/response_fwd.hpp>
#include <tip/http/common/header.hpp>
#include <tip/http/common/cookie.hpp>
#include <tip/http/common/response_status.hpp>
#include <tip/iri.hpp>

#include <boost/noncopyable.hpp>

namespace boost {
namespace asio {

class io_service;

}  // namespace asio
}  // namespace boost

namespace tip {
namespace http {
namespace server {

namespace detail {
class context_registry;
}  // namespace detail

class reply {
public:
	typedef std::multimap< std::string, std::string >	query_type;
	typedef tip::iri::basic_iri< query_type >			iri_type;
	typedef std::vector< char >							body_type;

	typedef body_type::const_iterator					body_input_iterator;

	typedef std::function< void(response_const_ptr) >	send_response_func;
	typedef std::function< void(response_status::status_type) > send_error_func;

	typedef std::shared_ptr< boost::asio::io_service > io_service_ptr;
public:
	reply( io_service_ptr io_service, request_const_ptr req,
			send_response_func, send_error_func );

	//@{
	io_service_ptr
	io_service() const;
	//@}
	//@{
	/** @name Access to request data */
	request_const_ptr
	request() const;

	request_method
	method() const;

	tip::iri::path const&
	path() const;

	headers const&
	request_headers() const;

	// TODO query
	// TODO fragment
	// TODO cookies

	body_type const&
	request_body() const;
	//@}

	//@{
	/** @name Response interface */
	void
	add_header(header const& h);
	void
	add_header(header&& h);

	void
	add_cookie(std::string const& name, std::string const& value);
	void
	add_cookie(cookie const&);
	void
	add_cookie(cookie &&);
	void
	remove_cookie(std::string const& name);
	/**
	 * Done with the request. No further data will be sent.
	 * @param
	 */
	void
	done( response_status::status_type = response_status::ok );

	void
	redirect( iri_type const&,
			response_status::status_type = response_status::temporary_redirect );
	void
	client_error(response_status::status_type = response_status::bad_request);
	void
	server_error(response_status::status_type = response_status::internal_server_error);

	body_type&
	response_body();
	body_type const&
	response_body() const;

	std::ostream&
	response_stream();
	std::locale
	get_locale() const;
	std::locale
	set_locale(std::locale const&);
	//@}
public:
	class id;
	class context;
private:
	template < typename Context >
	friend void
	add_context(reply&, Context*);

	template < typename Context >
	friend bool
	has_context(reply&);

	template < typename Context >
	friend Context&
	use_context(reply&);

	detail::context_registry&
	context_registry();
private:
	struct impl;
	typedef std::shared_ptr<impl> pimpl;
	typedef std::weak_ptr<impl> weak_pimpl;
	pimpl pimpl_;
private:
	friend class context;
	friend class detail::context_registry;
	reply(pimpl);
};

class reply::id : private boost::noncopyable {
public:
	id() {}
};

class reply::context : private boost::noncopyable {
public:
	typedef reply::body_type body_type;
public:
	context( reply const& r );
	virtual ~context();

	request_const_ptr
	get_request() const;

	reply
	get_reply() const;
private:
	friend class tip::http::server::detail::context_registry;
	struct key {
		key() : type_info_(nullptr), id_(nullptr) {}

		std::type_info const* type_info_;
		reply::id const* id_;

		bool
		operator == (key const& rhs)
		{
			if (id_ && rhs.id_ && id_ == rhs.id_)
				return true;
			if (type_info_ && rhs.type_info_ && type_info_ == rhs.type_info_)
				return true;
			return false;
		}
	} key_;

	context* next_;
	reply::weak_pimpl wpimpl_;
};

} /* namespace server */
} /* namespace http */
} /* namespace tip */

#endif /* TIP_HTTP_SERVER_REPLY_HPP_ */
