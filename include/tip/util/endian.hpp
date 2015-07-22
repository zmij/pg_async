/**
 * endian.hpp
 *
 *  Created on: 22 июля 2015 г.
 *      Author: zmij
 */

#ifndef LIB_PG_ASYNC_INCLUDE_TIP_UTIL_ENDIAN_HPP_
#define LIB_PG_ASYNC_INCLUDE_TIP_UTIL_ENDIAN_HPP_

#include <boost/predef/other/endian.h>
#include <boost/predef/detail/endian_compat.h>
#include <boost/cstdint.hpp>
#include <boost/endian/detail/intrinsic.hpp>
#include <type_traits>

namespace tip {
namespace util {
namespace endian {

enum order {
	big, little,
#ifdef BOOST_BIG_ENDIAN
	native = big
#else
	nagive = little
#endif
};

namespace detail {
template < size_t Size, bool sign >
struct bytes;

template <>
struct bytes<sizeof(int8_t), true> {
	static inline int8_t
	reverse(int8_t x)
	{
		return x;
	}
};
template <>
struct bytes<sizeof(int8_t), false> {
	static inline uint8_t
	reverse(uint8_t x)
	{
		return x;
	}
};

template <>
struct bytes< sizeof(int16_t), true > {
	static inline int16_t
	reverse(int16_t x)
	{
# ifdef BOOST_ENDIAN_NO_INTRINSICS
		return (static_cast<uint16_t>(x) << 8) | (static_cast<uint16_t>(x) >> 8);
# else
		return BOOST_ENDIAN_INTRINSIC_BYTE_SWAP_2(static_cast<uint16_t>(x));
# endif
	}
};
template <>
struct bytes< sizeof(int16_t), false > {
	static inline uint16_t
	reverse(uint16_t x)
	{
# ifdef BOOST_ENDIAN_NO_INTRINSICS
		return (x << 8) | (x >> 8);
# else
		return BOOST_ENDIAN_INTRINSIC_BYTE_SWAP_2(x);
# endif
	}
};

template <>
struct bytes< sizeof(int32_t), true > {
	static inline int32_t
	reverse(int32_t x)
	{
# ifdef BOOST_ENDIAN_NO_INTRINSICS
		uint32_t step16;
		step16 = static_cast<uint32_t>(x) << 16 | static_cast<uint32_t>(x) >> 16;
		return
			((static_cast<uint32_t>(step16) << 8) & 0xff00ff00)
		  | ((static_cast<uint32_t>(step16) >> 8) & 0x00ff00ff);
# else
    	return BOOST_ENDIAN_INTRINSIC_BYTE_SWAP_4(static_cast<uint32_t>(x));
# endif
	}
};
template <>
struct bytes< sizeof(int32_t), false > {
	static inline uint32_t
	reverse(uint32_t x)
	{
# ifdef BOOST_ENDIAN_NO_INTRINSICS
		uint32_t step16 = x << 16 | x >> 16;
		return
			((step16 << 8) & 0xff00ff00)
		  | ((step16 >> 8) & 0x00ff00ff);
# else
    	return BOOST_ENDIAN_INTRINSIC_BYTE_SWAP_4(static_cast<uint32_t>(x));
# endif
	}
};

template <>
struct bytes< sizeof(int64_t), true > {
	static inline int64_t
	reverse(int64_t x)
	{
# ifdef BOOST_ENDIAN_NO_INTRINSICS
		uint64_t step32, step16;
		step32 = static_cast<uint64_t>(x) << 32 | static_cast<uint64_t>(x) >> 32;
		step16 = (step32 & 0x0000FFFF0000FFFFULL) << 16
			   | (step32 & 0xFFFF0000FFFF0000ULL) >> 16;
		return static_cast<int64_t>((step16 & 0x00FF00FF00FF00FFULL) << 8
			   | (step16 & 0xFF00FF00FF00FF00ULL) >> 8);
# else
    	return BOOST_ENDIAN_INTRINSIC_BYTE_SWAP_8(static_cast<uint64_t>(x));
# endif
	}
};
template <>
struct bytes< sizeof(int64_t), false > {
	static inline uint64_t
	reverse(uint64_t x)
	{
# ifdef BOOST_ENDIAN_NO_INTRINSICS
		uint64_t step32, step16;
		step32 = x << 32 | x >> 32;
		step16 = (step32 & 0x0000FFFF0000FFFFULL) << 16
			   | (step32 & 0xFFFF0000FFFF0000ULL) >> 16;
		return (step16 & 0x00FF00FF00FF00FFULL) << 8
			   | (step16 & 0xFF00FF00FF00FF00ULL) >> 8;
# else
    	return BOOST_ENDIAN_INTRINSIC_BYTE_SWAP_8(static_cast<uint64_t>(x));
# endif
	}
};
}  // namespace detail

template < typename T >
inline T
endian_reverse(T x)
{
	return detail::bytes<sizeof(T), std::is_signed<T>::value >::reverse(x);
}

template < typename T >
inline T
big_to_native(T x)
{
#ifdef BOOST_BIG_ENDIAN
	return x;
#else
	return endian_reverse(x);
#endif
}

template < typename T >
inline T
native_to_big(T x)
{
#ifdef BOOST_BIG_ENDIAN
	return x;
#else
	return endian_reverse(x);
#endif
}

template < typename T >
inline T
little_to_native(T x)
{
#ifdef BOOST_LITTLE_ENDIAN
	return x;
#else
	return endian_reverse(x);
#endif
}

template < typename T >
inline T
native_to_little(T x)
{
#ifdef BOOST_LITTLE_ENDIAN
	return x;
#else
	return endian_reverse(x);
#endif
}

}  // namespace endian
}  // namespace util
}  // namespace tip


#endif /* LIB_PG_ASYNC_INCLUDE_TIP_UTIL_ENDIAN_HPP_ */
