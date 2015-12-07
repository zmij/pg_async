/*
 * streambuf.hpp
 *
 *  Created on: 12 июля 2015 г.
 *      Author: brysin
 */

#ifndef TIP_UTIL_STREAMBUF_HPP_
#define TIP_UTIL_STREAMBUF_HPP_

#include <streambuf>
#include <vector>

namespace tip {
namespace util {

template <typename charT, typename container = std::vector<charT>, typename traits = std::char_traits<charT> >
class basic_input_iterator_buffer : public std::basic_streambuf<charT, traits> {
public:
	typedef charT char_type;
	typedef container container_type;
	typedef typename container_type::const_iterator const_iterator;
	typedef std::ios_base ios_base;

	typedef std::basic_streambuf<charT, traits> base;
	typedef typename base::pos_type pos_type;
	typedef typename base::off_type off_type;
public:
	basic_input_iterator_buffer(const_iterator s, const_iterator e)
		: base(), s_(s), start_(const_cast<char_type*>(&*s)),
		  count_(e - s)
	{
		base::setg(start_, start_, start_ + count_);
	}
	basic_input_iterator_buffer(basic_input_iterator_buffer&& rhs)
		: base(), start_(rhs.start_), count_(rhs.count_)
	{
		auto n = rhs.gptr();
		base::setg(start_, n, start_ + count_);
	}
	const_iterator
	begin() const
	{
		return s_;
	}
	const_iterator
	end() const
	{
		return s_ + count_;
	}
	bool
	empty() const
	{
		return count_ == 0;
	}
protected:
	pos_type
	seekoff (off_type off, ios_base::seekdir way,
	                  ios_base::openmode which = ios_base::in | ios_base::out)
	{
		char_type* tgt(nullptr);
		switch (way) {
			case ios_base::beg:
				tgt = start_ + off;
				break;
			case ios_base::cur:
				tgt = base::gptr() + off;
				break;
			case ios_base::end:
				tgt = start_ + count_ - 1 + off;
				break;
			default:
				break;
		}
		if (!tgt)
			return -1;
		if (tgt < start_ || start_ + count_ < tgt)
			return -1;
		base::setg(start_, tgt, start_ + count_);
		return tgt - start_;
	}
	pos_type
	seekpos(pos_type pos, ios_base::openmode which = ios_base::in | ios_base::out)
	{
		char_type* tgt = start_ + pos;
		if (tgt < start_ || start_ + count_ < tgt)
			return -1;
		base::setg(start_, tgt, start_ + count_);
		return tgt - start_;
	}
private:
	const_iterator s_;
	char_type* start_;
	size_t count_;
};

typedef basic_input_iterator_buffer<char> input_iterator_buffer;

}  // namespace util
}  // namespace tip

#endif /* TIP_UTIL_STREAMBUF_HPP_ */
