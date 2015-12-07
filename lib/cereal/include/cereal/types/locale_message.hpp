/*
 * locale_message.hpp
 *
 *  Created on: Oct 8, 2015
 *      Author: zmij
 */

#ifndef CEREAL_INCLUDE_CEREAL_TYPES_LOCALE_MESSAGE_HPP_
#define CEREAL_INCLUDE_CEREAL_TYPES_LOCALE_MESSAGE_HPP_

#include <cereal/cereal.hpp>
#include <cereal/archives/json.hpp>
#include <boost/locale/message.hpp>
#include <iostream>

namespace cereal {
namespace detail {

void
read_l10n(JSONInputArchive& ar, boost::locale::message& msg, size_type sz)
{
	if (sz < 1) {
		throw Exception("Invalid size of array for L10N");
	}
	std::string id;
	ar(id);
	msg = boost::locale::message(id);
}

void
read_l10nn(JSONInputArchive& ar, boost::locale::message& msg, size_type sz)
{
	if (sz < 3) {
		throw Exception("Invalid size of array for L10NN");
	}
	std::string single, plural;
	int n;
	ar(single, plural, n);
	msg = boost::locale::message(single, plural, n);
}

void
read_l10nc(JSONInputArchive& ar, boost::locale::message& msg, size_type sz)
{
	if (sz < 2) {
		throw Exception("Invalid size of array for L10NC");
	}
	std::string context, id;
	ar(context, id);
	msg = boost::locale::message(context, id);
}

void
read_l10nnc(JSONInputArchive& ar, boost::locale::message& msg, size_type sz)
{
	if (sz < 4) {
		throw Exception("Invalid size of array for L10NNC");
	}
	std::string context, single, plural;
	int n;
	ar(context, single, plural, n);
	msg = boost::locale::message(context, single, plural, n);
}


}  // namespace detail
//----------------------------------------------------------------------------
//	Read a localized message source from JSON
//	JSON format
//----------------------------------------------------------------------------
void
prologue(JSONInputArchive& ar, boost::locale::message const& msg)
{
}

void
CEREAL_LOAD_FUNCTION_NAME(JSONInputArchive& ar, boost::locale::message& msg)
{
	if (ar.nodeValue().IsArray()) {
		ar.startNode();
		size_type sz;
		ar( make_size_tag(sz) );

		std::string type;
		ar(type);
		--sz;

		if (type == "L10N") {
			detail::read_l10n(ar, msg, sz);
		} else if (type == "L10NN") {
			detail::read_l10nn(ar, msg, sz);
		} else if (type == "L10NC") {
			detail::read_l10nc(ar, msg, sz);
		} else if (type == "L10NNC") {
			detail::read_l10nnc(ar, msg, sz);
		}

		ar.finishNode();
	} else if (ar.nodeValue().IsString()){
		std::string str;
		ar(str);
		msg = boost::locale::message(str);
	}
}

void
epilogue(JSONInputArchive& ar, boost::locale::message const& msg)
{
}

}  // namespace cereal


#endif /* CEREAL_INCLUDE_CEREAL_TYPES_LOCALE_MESSAGE_HPP_ */
