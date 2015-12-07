/*
 * error.hpp
 *
 *  Created on: Oct 6, 2015
 *      Author: zmij
 */

#ifndef TIP_HTTP_SERVER_ERROR_HPP_
#define TIP_HTTP_SERVER_ERROR_HPP_

#include <stdexcept>
#include <tip/log.hpp>
#include <tip/http/common/response_status.hpp>

namespace tip {
namespace http {
namespace server {

class error: public std::runtime_error {
public:
	error(std::string const& cat, std::string const& w,
			response_status::status_type s
				= response_status::internal_server_error,
			log::logger::event_severity sv = log::logger::ERROR) :
		runtime_error(w), category_(cat), status_(s), severity_(sv) {};
	virtual ~error() {}

	virtual std::string const&
	name() const;

	log::logger::event_severity
	severity() const
	{ return severity_; }

	std::string const&
	category() const
	{ return category_; }

	response_status::status_type
	status() const
	{ return status_; }


	void
	log_error(std::string const& message = "") const;
private:
	log::logger::event_severity		severity_;
	std::string						category_;
	response_status::status_type	status_;
};

class client_error : public error {
public:
	client_error(std::string const& cat, std::string const& w,
			response_status::status_type s
				= response_status::bad_request,
				log::logger::event_severity sv = log::logger::ERROR)
		: error(cat, w, s, sv) {};
	virtual ~client_error() {};

	virtual std::string const&
	name() const;
};

} /* namespace server */
} /* namespace http */
} /* namespace tip */

#endif /* TIP_HTTP_SERVER_ERROR_HPP_ */
