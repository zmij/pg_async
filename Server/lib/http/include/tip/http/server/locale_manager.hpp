/**
 * locale_manager.hpp
 *
 *  Created on: 10 окт. 2015 г.
 *      Author: zmij
 */

#ifndef TIP_HTTP_SERVER_LOCALE_MANAGER_HPP_
#define TIP_HTTP_SERVER_LOCALE_MANAGER_HPP_

#include <tip/http/common/header.hpp>
#include <tip/http/server/reply.hpp>
#include <boost/asio/io_service.hpp>
#include <string>
#include <set>
#include <functional>

namespace tip {
namespace http {
namespace server {


class locale_manager : public boost::asio::io_service::service {
public:
	typedef boost::asio::io_service io_service;
	typedef io_service::service base_type;
	typedef std::set< std::string > language_set;
	typedef std::pair< std::string, bool > request_language;
	typedef std::function< request_language (reply& r) > request_language_callback;

	static io_service::id id;
public:
	locale_manager(io_service&);
	virtual ~locale_manager();

	void
	add_language(std::string const& lcname);
	void
	add_languages(std::string const& lcnames);
	void
	set_default_language(std::string const& lcname);
	language_set const&
	supported_languages() const;

	void
	add_messages_path(std::string const& path);
	void
	add_messages_domain(std::string const& domain);

	std::string
	default_locale_name() const;
	std::locale
	default_locale() const;

	bool
	is_locale_supported(std::string const& name) const;
	std::locale
	get_locale(std::string const& name) const;
	std::locale
	get_locale(accept_languages const&) const;

	void
	set_reply_callback(request_language_callback);
	void
	deduce_locale(reply r) const;
private:
	virtual void shutdown_service() {}
private:
	struct impl;
	typedef std::unique_ptr< impl > pimpl;
	pimpl pimpl_;
};

} /* namespace server */
} /* namespace http */
} /* namespace tip */

#endif /* TIP_HTTP_SERVER_LOCALE_MANAGER_HPP_ */
