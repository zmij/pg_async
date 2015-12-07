/*
 * ssl_context_service.hpp
 *
 *  Created on: Aug 21, 2015
 *      Author: zmij
 */

#ifndef TIP_SSL_SSL_CONTEXT_SERVICE_HPP_
#define TIP_SSL_SSL_CONTEXT_SERVICE_HPP_

#include <boost/asio/io_service.hpp>
#include <boost/asio/ssl.hpp>

#include <memory>

// Forwards
namespace boost {
namespace filesystem {

struct path;

}  // namespace filesystem
}  // namespace boost

namespace tip {
namespace ssl {

class ssl_context_service: public boost::asio::io_service::service {
public:
	typedef boost::asio::io_service io_service;
	typedef io_service::service base_type;
	typedef boost::asio::ssl::context context_type;
	typedef boost::filesystem::path path_type;

	static io_service::id id;
public:
	ssl_context_service(io_service&);
	virtual ~ssl_context_service();

	context_type&
	context();
	context_type const&
	context() const;

	void
	load_verify_files(path_type const& directory);
private:
	virtual void
	shutdown_service();
private:
	struct impl;
	typedef std::shared_ptr<impl> pimpl;
	pimpl pimpl_;
};

} /* namespace ssl */
} /* namespace tip */

#endif /* TIP_SSL_SSL_CONTEXT_SERVICE_HPP_ */
