/**
 * message.hpp
 *
 *  Created on: 11 окт. 2015 г.
 *      Author: zmij
 */

#ifndef TIP_L10N_MESSAGE_HPP_
#define TIP_L10N_MESSAGE_HPP_

#include <string>
#include <memory>
#include <iosfwd>

#include <boost/optional.hpp>
#include <boost/variant.hpp>
#include <boost/locale.hpp>

#include <cereal/cereal.hpp>
#include <cereal/archives/json.hpp>

namespace tip {
namespace l10n {

class format_args;

class locale_name_facet : public std::locale::facet {
public:
	static std::locale::id id;
public:
	locale_name_facet(std::string const& n) : facet(), name_(n) {}
	virtual ~locale_name_facet() {}

	std::string const&
	name() const
	{ return name_; }
private:
	std::string name_;
};

/**
 * A facet for parsing from json files
 */
class domain_name_facet : public std::locale::facet {
public:
	static std::locale::id id;
public:
	domain_name_facet(std::string const& domain) : domain_(domain) {}

	std::string const&
	domain() const
	{ return domain_; }
private:
	std::string domain_;
};

class message {
public:
	typedef boost::optional< std::string >	optional_string;
	typedef std::unique_ptr< format_args >	optional_format_args;
	typedef cereal::size_type				size_type;
	typedef boost::locale::message			localized_message;
	typedef boost::locale::format			formatted_message;

	enum type {
		EMPTY,
		SIMPLE,
		PLURAL,
		CONTEXT,
		CONTEXT_PLURAL
	};
public:
	/**
	 * Construct a default empty message
	 */
	message();
	message(optional_string const& domain);
	/**
	 * Construct a message with message id
	 */
	message(std::string const& id,
			optional_string const& domain = optional_string());
	/**
	 * Construct a message with message id and a context
	 */
	message(std::string const& context,
			std::string const& id,
			optional_string const& domain = optional_string());
	/**
	 * Construct a message with singular/plural
	 */
	message(std::string const& singular,
			std::string const& plural,
			int n,
			optional_string const& domain = optional_string());
	/**
	 * Construct a message with singular/plural and a context
	 */
	message(std::string const& context,
			std::string const& singular,
			std::string const& plural,
			int n,
			optional_string const& domain = optional_string());

	message(message const&);
	message(message&&) = default;

	void
	swap(message& rhs);
	void
	swap(message&& rhs);

	bool
	empty() const
	{ return type_ == EMPTY || id_.empty(); }

	bool
	has_context() const
	{ return context_.is_initialized(); }
	bool
	has_plural() const
	{ return plural_.is_initialized(); }
	bool
	has_format_args() const
	{ return format_args_.get() != nullptr; }

	void
	set_domain( std::string const& );

	void
	load(cereal::JSONInputArchive& ar);
	void
	save(cereal::JSONOutputArchive& ar) const;
	void
	write(std::ostream& os) const;

	localized_message
	translate() const;
private:
	void
	read_l10n(cereal::JSONInputArchive& ar, size_type& sz);
	void
	read_l10nn(cereal::JSONInputArchive& ar, size_type& sz);
	void
	read_l10nc(cereal::JSONInputArchive& ar, size_type& sz);
	void
	read_l10nnc(cereal::JSONInputArchive& ar, size_type& sz);

	void
	read_format_args(cereal::JSONInputArchive& ar, size_type sz);
private:
	type					type_;
	std::string				id_;
	optional_string			context_;
	optional_string 		plural_;
	optional_string			domain_;

	int						n_;
	optional_format_args	format_args_;
};

std::ostream&
operator << (std::ostream& os, message const& v);

class format_args {
public:
	typedef boost::variant<
			int64_t, double, bool,
			std::string, message >			arg_type;
	typedef std::vector< arg_type >			arguments_vector;
	typedef cereal::size_type				size_type;
	typedef boost::locale::format			formatted_message;
	typedef std::vector< std::string >		format_buffers_type;
	typedef boost::optional< std::string >	optional_string;
public:
	void
	load(cereal::JSONInputArchive&, size_type sz);

	size_t
	size() const
	{ return arguments_.size(); }
	void
	feed_arguments(formatted_message& msg, format_buffers_type&,
			std::locale const&, optional_string const& domain) const;
private:
	arguments_vector arguments_;
};

} /* namespace l10n */
} /* namespace tip */

namespace cereal {

inline void
prologue(JSONInputArchive& ar, tip::l10n::message const&) {}
inline void
epilogue(JSONInputArchive&, tip::l10n::message const& ) {}

inline void
prologue(JSONOutputArchive& ar, tip::l10n::message const&) {}
inline void
epilogue(JSONOutputArchive&, tip::l10n::message const& ) {}


}  // namespace cereal

#endif /* TIP_L10N_MESSAGE_HPP_ */
