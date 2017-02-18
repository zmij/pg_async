/*
 * uuid.hpp
 *
 *  Created on: Aug 10, 2015
 *      Author: zmij
 */

#ifndef LIB_PG_ASYNC_INCLUDE_TIP_DB_PG_IO_UUID_HPP_
#define LIB_PG_ASYNC_INCLUDE_TIP_DB_PG_IO_UUID_HPP_

#include <tip/db/pg/protocol_io_traits.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <iterator>

namespace tip {
namespace db {
namespace pg {
namespace io {

template <>
struct protocol_parser< ::boost::uuids::uuid, BINARY_DATA_FORMAT >:
        detail::parser_base< ::boost::uuids::uuid > {

    using uuid          = ::boost::uuids::uuid;
    using base_type     = detail::parser_base<uuid>;
    using value_type    = base_type::value_type;

    protocol_parser(value_type& v) : base_type(v) {}

    size_t
    size() const
    {
        return uuid::static_size();
    }

    template < typename InputIterator >
    InputIterator
    operator()( InputIterator begin, InputIterator end)
    {
        typedef std::iterator_traits< InputIterator > iter_traits;
        typedef typename iter_traits::value_type iter_value_type;
        static_assert(std::is_same< iter_value_type, byte >::type::value,
                "Input iterator must be over a char container");
        assert( (end - begin) >= (decltype (end - begin))size() && "Buffer size is insufficient" );
        end = begin + size();
        ::std::copy(begin, end, base_type::value.begin());
        return begin;
    }
};

template < >
struct protocol_formatter< ::boost::uuids::uuid, BINARY_DATA_FORMAT > :
        detail::formatter_base< ::boost::uuids::uuid > {

    using uuid          = ::boost::uuids::uuid;
    using base_type     = detail::formatter_base<uuid>;
    using value_type    = base_type::value_type;

    protocol_formatter(value_type const& val) : base_type(val) {}

    size_t
    size() const
    {
        return uuid::static_size();
    }

    bool
    operator() (::std::vector<byte>& buffer)
    {
        if (buffer.capacity() - buffer.size() < size()) {
            buffer.reserve(buffer.size() + size());
        }

        ::std::copy(base_type::value.begin(),
                base_type::value.end(), ::std::back_inserter(buffer));
        return true;
    }
};

namespace traits {

template <>
struct has_parser < ::boost::uuids::uuid, BINARY_DATA_FORMAT > : std::true_type {};
template <>
struct has_formatter < ::boost::uuids::uuid, BINARY_DATA_FORMAT > : std::true_type {};

//@{
template < >
struct pgcpp_data_mapping < oids::type::uuid > :
        detail::data_mapping_base< oids::type::uuid, boost::uuids::uuid > {};
template < >
struct cpppg_data_mapping < boost::uuids::uuid > :
        detail::data_mapping_base< oids::type::uuid, boost::uuids::uuid > {};
//@}

template <>
struct is_nullable< ::boost::uuids::uuid > : ::std::true_type {};

template <>
struct nullable_traits< ::boost::uuids::uuid > {
    using uuid = ::boost::uuids::uuid;
    inline static bool
    is_null(uuid const& val)
    {
        return val.is_nil();
    }
    inline static void
    set_null(uuid& val)
    {
        val = uuid{{0}};
    }
};

}  // namespace traits
}  // namespace io
}  // namespace pg
}  // namespace db
}  // namespace tip

#endif /* LIB_PG_ASYNC_INCLUDE_TIP_DB_PG_IO_UUID_HPP_ */
