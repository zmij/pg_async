/*
 * cookie.hpp
 *
 *  Created on: Aug 26, 2015
 *      Author: zmij
 */

#ifndef TIP_HTTP_COMMON_COOKIE_HPP_
#define TIP_HTTP_COMMON_COOKIE_HPP_

#include <string>
#include <vector>

#include <boost/date_time.hpp>
#include <boost/optional.hpp>

#include <tip/iri.hpp>

namespace tip {
namespace http {

struct cookie {
	typedef boost::optional< iri::host >	host_opt;
	typedef boost::optional< iri::path >	path_opt;
	typedef boost::optional< std::int32_t >	int_opt;
	typedef boost::posix_time::ptime		datetime_type;
	typedef boost::optional< datetime_type>	datetime_opt;

	std::string		name;
	std::string		value;

	datetime_opt	expires;
	int_opt			max_age;
	host_opt		domain;
	path_opt		path;
	bool			secure;
	bool			http_only;
};

struct cookie_name_cmp {
	bool
	operator()(cookie const& lhs, cookie const& rhs)
	{
		return lhs.name < rhs.name;
	}
};

// TODO multiset?
using request_cookies = std::vector<cookie>;

}  // namespace http
}  // namespace tip


#endif /* TIP_HTTP_COMMON_COOKIE_HPP_ */
