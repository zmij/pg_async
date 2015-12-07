/*
 * error.cpp
 *
 *  Created on: Oct 6, 2015
 *      Author: zmij
 */

#include <tip/http/server/error.hpp>

namespace tip {
namespace http {
namespace server {

void
error::log_error(std::string const& message) const
{
	tip::log::local local(category_, severity_);
	local << message << what();
}

std::string const&
error::name() const
{
	static std::string NAME_ = "error";
	return NAME_;
}

std::string const&
client_error::name() const
{
	static std::string NAME_ = "client_error";
	return NAME_;
}

} /* namespace server */
} /* namespace http */
} /* namespace tip */
