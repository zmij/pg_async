/**
 * locale_name.hpp
 *
 *  Created on: 10 окт. 2015 г.
 *      Author: zmij
 */

#ifndef TIP_HTTP_SERVER_LOCALE_NAME_HPP_
#define TIP_HTTP_SERVER_LOCALE_NAME_HPP_

#include <string>
#include <iosfwd>

namespace tip {
namespace http {
namespace server {

struct locale_name {
	std::string language;
	std::string culture;
	std::string encoding;

	locale_name();
	locale_name(std::string const&);
	locale_name(std::string const& lang,
			std::string const& culture,
			std::string const& encoding = std::string());

	operator std::string() const;

	bool
	empty() const
	{ return language.empty(); }

	enum compare_options {
		all				= 0,
		ignore_culture	= 1,
		ignore_encoding = 2,
		language_only	= ignore_culture | ignore_encoding
	};

	int
	compare(locale_name const& rhs, compare_options = all) const;

	bool
	operator == (locale_name const& rhs) const
	{ return compare(rhs) == 0; }
	bool
	operator != (locale_name const& rhs) const
	{ return !(*this == rhs); }

	bool
	operator < (locale_name const& rhs) const
	{ return compare(rhs) < 0; }
	bool
	operator <= (locale_name const& rhs) const
	{ return compare(rhs) <= 0; }
	bool
	operator > (locale_name const& rhs) const
	{ return compare(rhs) > 0; }
	bool
	operator >= (locale_name const& rhs) const
	{ return compare(rhs) >= 0; }

	struct no_culture {
		bool
		operator()(locale_name const& lhs, locale_name const& rhs) const
		{
			return lhs.compare(rhs, ignore_culture);
		}
	};
	struct no_encoding {
		bool
		operator()(locale_name const& lhs, locale_name const& rhs) const
		{
			return lhs.compare(rhs, ignore_encoding);
		}
	};
	struct lang_only {
		bool
		operator()(locale_name const& lhs, locale_name const& rhs) const
		{
			return lhs.compare(rhs, language_only);
		}
	};
};

std::ostream&
operator << (std::ostream& os, locale_name const& val);
std::istream&
operator >> (std::istream& is, locale_name& val);

} /* namespace server */
} /* namespace http */
} /* namespace tip */

#endif /* TIP_HTTP_SERVER_LOCALE_NAME_HPP_ */
