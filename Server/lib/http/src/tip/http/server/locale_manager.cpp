/**
 * locale_manager.cpp
 *
 *  Created on: 10 окт. 2015 г.
 *      Author: zmij
 */

#include <tip/http/server/locale_manager.hpp>
#include <tip/http/server/locale_name.hpp>
#include <tip/http/server/grammar/locale_name.hpp>
#include <tip/http/server/language_context.hpp>
#include <boost/locale.hpp>

namespace tip {
namespace http {
namespace server {

boost::asio::io_service::id locale_manager::id;

struct locale_manager::impl {
	boost::locale::generator	gen_;
	std::string					default_encoding_;

	locale_name					default_locale_;
	language_set				supported_languages_;
	request_language_callback	callback_;

	impl() : default_encoding_("UTF-8"), default_locale_{ "en_US.UTF-8" }
	{
		gen_.locale_cache_enabled(true);
	}

	void
	add_language(std::string const& lcname)
	{
		add_language( create_locale_name({lcname}) );
	}

	void
	add_language(locale_name const& lname)
	{
		if (!supported_languages_.count(lname.language)) {
			if (supported_languages_.empty()) {
				default_locale_ = lname;
			}
			supported_languages_.insert(lname.language);
		}
	}

	void
	add_languages(std::string const& lcnames)
	{
		typedef std::string::const_iterator input_iterator;
		typedef std::vector<locale_name> language_list;
		typedef grammar::parse::locale_names_grammar< input_iterator, language_list > names_grammar;

		language_list new_locales;
		input_iterator f = lcnames.begin();
		input_iterator l = lcnames.end();
		if (!boost::spirit::qi::parse(f, l, names_grammar(), new_locales) || f != l)
			throw std::runtime_error("Invalid locales string");
		for (auto& l: new_locales) {
			add_language( create_locale_name(l));
		}
	}

	locale_name
	create_locale_name(locale_name const& lname )
	{
		locale_name copy{lname};
		if (copy.encoding.empty()) {
			copy.encoding = default_encoding_;
		}
		return copy;
	}

	bool
	is_supported(std::string const& name)
	{
		return is_supported(locale_name{name});
	}
	bool
	is_supported(locale_name const& lname)
	{
		return supported_languages_.count(lname.language);
	}

	std::locale
	get_locale(std::string const& name)
	{
		locale_name lname{name};
		auto f = supported_languages_.find(lname.language);
		if (f == supported_languages_.end()) {
			lname = default_locale_;
		}
		if (lname.encoding.empty()) {
			lname.encoding = default_encoding_;
		}
		return gen_((std::string)lname);
	}
	std::locale
	get_locale(accept_languages const& langs)
	{
		for (auto&& lang : langs) {
			locale_name lname{ lang.first };
			if (is_supported(lname)) {
				if (lname.encoding.empty()) {
					lname.encoding = default_encoding_;
				}
				return gen_((std::string)lname);
			}
		}
		return gen_((std::string)default_locale_);
	}

	std::locale
	get_locale(reply r)
	{
		if (callback_) {
			request_language rl = callback_(r);
			if (rl.second) {
				return get_locale(rl.first);
			}
		}
		language_context& lctx = use_context<language_context>(r);
		return get_locale(lctx.languages());
	}

	locale_name
	negotiate_locale(std::string const& lcname)
	{
		locale_name lname{lcname};
		auto f = supported_languages_.find(lname.language);
		if (f == supported_languages_.end()) {
			lname = default_locale_;
		}
		if (lname.encoding.empty()) {
			lname.encoding = default_encoding_;
		}
		return lname;
	}
	locale_name
	negotiate_locale(accept_languages const& langs)
	{
		for (auto&& lang : langs) {
			locale_name lname{ lang.first };
			if (is_supported(lname)) {
				if (lname.encoding.empty()) {
					lname.encoding = default_encoding_;
				}
				return lname;
			}
		}
		return default_locale_;
	}

	void
	deduce_locale(reply& r)
	{
		locale_name lname;
		if (callback_) {
			request_language rl = callback_(r);
			if (rl.second) {
				lname = negotiate_locale(rl.first);
			}
		}
		if (lname.empty()) {
			language_context& lctx = use_context<language_context>(r);
			lname = negotiate_locale(lctx.languages());
		}
		r.add_header({ http::ContentLanguage, lname.language });
		r.response_stream().imbue( gen_((std::string)lname));
	}
};

locale_manager::locale_manager(io_service& svc) : base_type(svc), pimpl_(new impl)
{
}

locale_manager::~locale_manager()
{
}

void
locale_manager::add_language(std::string const& lcname)
{
	pimpl_->add_language(lcname);
}

void
locale_manager::add_languages(std::string const& lcnames)
{
	pimpl_->add_languages(lcnames);
}

void
locale_manager::set_default_language(std::string const& lcname)
{
	pimpl_->default_locale_ = pimpl_->create_locale_name(lcname);
}

locale_manager::language_set const&
locale_manager::supported_languages() const
{
	return pimpl_->supported_languages_;
}

void
locale_manager::add_messages_path(std::string const& path)
{
	pimpl_->gen_.add_messages_path(path);
}

void
locale_manager::add_messages_domain(std::string const& domain)
{
	pimpl_->gen_.add_messages_domain(domain);
}

std::string
locale_manager::default_locale_name() const
{
	return pimpl_->default_locale_;
}

std::locale
locale_manager::default_locale() const
{
	return pimpl_->gen_((std::string)pimpl_->default_locale_);
}

bool
locale_manager::is_locale_supported(std::string const& name) const
{
	return pimpl_->is_supported(name);
}

std::locale
locale_manager::get_locale(std::string const& name) const
{
	return pimpl_->get_locale(name);
}

std::locale
locale_manager::get_locale(accept_languages const& langs) const
{
	return pimpl_->get_locale(langs);
}

void
locale_manager::set_reply_callback(request_language_callback cb)
{
	pimpl_->callback_ = cb;
}

void
locale_manager::deduce_locale(reply r) const
{
	pimpl_->deduce_locale(r);
}

} /* namespace server */
} /* namespace http */
} /* namespace tip */
