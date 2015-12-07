/*
 * header.hpp
 *
 *  Created on: Aug 18, 2015
 *      Author: zmij
 */

#ifndef TIP_HTTP_COMMON_HEADER_HPP_
#define TIP_HTTP_COMMON_HEADER_HPP_

#include <boost/variant.hpp>
#include <map>
#include <set>
#include <vector>
#include <tip/http/common/header_names.hpp>

namespace tip {
namespace http {

using header_name = boost::variant< known_header_name, std::string >;
using header = std::pair<header_name, std::string>;
using headers = std::multimap<header_name, std::string>;

std::ostream&
operator << (std::ostream&, header const&);
std::ostream&
operator << (std::ostream&, headers const&);

size_t
content_length(headers const& hdrs);

bool
chunked_transfer_encoding(headers const& hdrs);

using accept_language = std::pair< std::string, float >;
struct accept_language_qvalue_sort {
	bool
	operator()(accept_language const& lhs, accept_language const& rhs) const
	{
		return lhs.second > rhs.second;
	}
};
using accept_languages = std::multiset< accept_language, accept_language_qvalue_sort >;

}  // namespace http
}  // namespace tip



#endif /* TIP_HTTP_COMMON_HEADER_HPP_ */
