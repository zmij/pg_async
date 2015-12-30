/*
 * tokenizer_base.hpp
 *
 *  Created on: Sep 28, 2015
 *      Author: zmij
 */

#ifndef LIB_PG_ASYNC_SRC_TIP_DB_PG_DETAIL_TOKENIZER_BASE_HPP_
#define LIB_PG_ASYNC_SRC_TIP_DB_PG_DETAIL_TOKENIZER_BASE_HPP_

#include <string>
#include <utility>
#include <memory>
#include <iterator>
#include <type_traits>

namespace tip {
namespace db {
namespace pg {
namespace io {
namespace detail {

enum token_type {
	NoToken,
	LiteralToken,
	SingleQuotedToken,
	DoubleQuotedToken
};

const char SINGLE_QUOTE = '\'';
const char DOUBLE_QUOTE = '"';
const char PART_DELIMITER = ',';
const char ESCAPE_SYMBOL = '\\';

typedef std::pair< token_type, std::string > token;

template < typename InputIterator >
class parser_state_base {
public:
	typedef InputIterator								iterator_type;
	typedef std::iterator_traits< iterator_type >		iterator_traits;
	typedef typename iterator_traits::value_type		value_type;
public:
	parser_state_base(iterator_type& begin, iterator_type end) :
		current_(begin), end_(end)
	{
		static_assert(std::is_same< value_type, char >::value,
			"InputIterator must be over a char container");
	}
	virtual ~parser_state_base()
	{
	}

	void
	operator()(token& tok)
	{
		get_token(tok);
	}
protected:
	bool
	empty() const
	{
		return current_ == end_;
	}
	iterator_type const&
	current()
	{
		return current_;
	}
	value_type
	value()
	{
		return *current_;
	}
	void
	advance()
	{
		++current_;
	}
private:
	virtual void
	get_token(token& tok) = 0;
private:
	iterator_type&	current_;
	iterator_type	end_;
};

template < typename InputIterator, char CloseSymbol >
class unquoted_literal : public parser_state_base< InputIterator > {
public:
	typedef parser_state_base< InputIterator >	base_type;
	typedef typename base_type::iterator_type	iterator_type;
public:
	unquoted_literal(iterator_type& begin, iterator_type end) :
		base_type(begin, end) {}
	virtual ~unquoted_literal() {}
private:
	virtual void
	get_token(token& tok)
	{
		tok.first = LiteralToken;
		tok.second.clear();
		for (; !base_type::empty() && base_type::value() != PART_DELIMITER
					&& base_type::value() != CloseSymbol;
				base_type::advance()) {
			tok.second.push_back(base_type::value());
		}
	}
};

template < typename InputIterator, char QuoteSymbol, token_type TokenType >
class quoted_literal : public parser_state_base< InputIterator > {
public:
	typedef parser_state_base< InputIterator >	base_type;
	typedef typename base_type::iterator_type	iterator_type;
	typedef typename base_type::value_type		value_type;
public:
	quoted_literal(iterator_type& begin, iterator_type end) :
		base_type(begin, end) {}
	virtual ~quoted_literal() {}
private:
	virtual void
	get_token(token& tok)
	{
		tok.first = TokenType;
		tok.second.clear();
		while (!base_type::empty()) {
			value_type curr = base_type::value();
			switch (curr) {
				case ESCAPE_SYMBOL: {
					base_type::advance();
					if (!base_type::empty()) {
						tok.second.push_back(base_type::value());
						base_type::advance();
					}
					break;
				}
				case QuoteSymbol: {
					base_type::advance();
					if (!base_type::empty() && base_type::value() == QuoteSymbol) {
						// Quote symbol is duplicated (escaped)
						tok.second.push_back(base_type::value());
						base_type::advance();
					} else {
						return;
					}
					break;
				}
				default : {
					tok.second.push_back(base_type::value());
					base_type::advance();
					break;
				}
			}
		}
	}
};

template < typename InputIterator >
using single_quoted_literal = quoted_literal< InputIterator, SINGLE_QUOTE, SingleQuotedToken >;
template < typename InputIterator >
using double_quoted_literal = quoted_literal< InputIterator, DOUBLE_QUOTE, DoubleQuotedToken >;

template < typename InputIterator, char OpenSymbol, char CloseSymbol >
class tokenizer_base {
public:
	typedef InputIterator									input_iterator;
	typedef parser_state_base< input_iterator >				parser_base_type;
	typedef std::unique_ptr< parser_base_type >				parser_ptr;
	typedef unquoted_literal< input_iterator, CloseSymbol >	unquoted_type;
	typedef single_quoted_literal< input_iterator >			single_quoted_type;
	typedef double_quoted_literal< input_iterator >			double_quoted_type;
public:
	template < typename OutputIterator >
	tokenizer_base(input_iterator& current, input_iterator end, OutputIterator out)
	{
		for (; current != end && *current != OpenSymbol; ++current);
		if (current != end)
			++current;
		bool part_delim = false;
		while (current != end) {
			parser_ptr worker;
			switch (*current) {
				case CloseSymbol:
					if (part_delim)
						worker.reset( new unquoted_type(current, end) );
					current = end;
					break;
				case SINGLE_QUOTE:
					++current;
					worker.reset( new single_quoted_type(current, end) );
					break;
				case DOUBLE_QUOTE:
					++current;
					worker.reset( new double_quoted_type(current, end) );
					break;
				default:
					worker.reset( new unquoted_type(current, end) );
					break;
			}
			if (worker) {
				token tok;
				(*worker)(tok);
				if (tok.first != NoToken) {
					*out++ = std::move(tok.second);
				}
			}

			if (current != end) {
				part_delim = *current == PART_DELIMITER;
				if (part_delim)
					++current;
			}
		}
	}
};

} /* namespace detail */
}  // namespace io
} /* namespace pg */
} /* namespace db */
} /* namespace tip */

#endif /* LIB_PG_ASYNC_SRC_TIP_DB_PG_DETAIL_TOKENIZER_BASE_HPP_ */
