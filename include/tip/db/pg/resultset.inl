/*
 * resultset.inl
 *
 *  Created on: Jul 16, 2015
 *      Author: zmij
 */

#ifndef TIP_DB_PG_RESULTSET_INL_
#define TIP_DB_PG_RESULTSET_INL_

#include <tip/db/pg/resultset.hpp>
#include <tip/util/meta_helpers.hpp>

namespace tip {
namespace db {
namespace pg {

namespace detail {


template < size_t Index, typename T >
struct nth_field {
    enum {
        index = Index
    };
    typedef T type;

    nth_field(resultset::row const& r) : row(r)
    {
    }

    T value()
    {
        return row[index].template as<T>();
    }

    bool
    to(T& val)
    {
        return row[index].to(val);
    }

    resultset::row row;
};

template < typename IndexTuple, typename ... T >
struct row_data_extractor_base;

template < size_t ... Indexes, typename ... T >
struct row_data_extractor_base< util::indexes_tuple< Indexes ... >, T ... > {
    static constexpr ::std::size_t size = sizeof ... (T);

    static void
    get_tuple( resultset::row const& row, std::tuple< T ... >& val )
    {
        std::tuple< T ... > tmp( nth_field< Indexes, T >(row).value() ... );
        tmp.swap(val);
    }

    static void
    get_values( resultset::row const& row, T& ... val )
    {
        util::expand(nth_field< Indexes, T >(row).to(val) ...);
    }
};

template < typename ... T >
struct row_data_extractor :
        row_data_extractor_base < typename util::index_builder< sizeof ... (T) >::type, T ... > {
};

template < typename IndexTuple, typename ... T >
struct field_by_name_extractor;

template < ::std::size_t ... Indexes, typename ... T >
struct field_by_name_extractor< util::indexes_tuple<Indexes ...>, T... > {
    static constexpr ::std::size_t size = sizeof ... (T);

    static void
    get_tuple( resultset::row const& row,
            ::std::initializer_list<::std::string> const& names,
            ::std::tuple< T... >& val )
    {
        if (names.size() < size)
            throw error::db_error{"Not enough names in row data extraction"};
        ::std::tuple<T ... > tmp( row[*(names.begin() + Indexes)].template as<T>()... );
        tmp.swap(val);
    }

    static void
    get_values(resultset::row const& row,
            ::std::initializer_list<::std::string> const& names,
            T&... val)
    {
        util::expand{ row[*(names.begin() + Indexes)].to(val)... };
    }
};

template < typename ... T >
struct row_data_by_name_extractor
    : field_by_name_extractor< typename util::index_builder< sizeof ... (T) >::type, T ... > {};

}  // namespace detail

template < typename ... T >
void
resultset::row::to(std::tuple< T ... >& val) const
{
    detail::row_data_extractor< T ... >::get_tuple(*this, val);
}

template < typename ... T >
void
resultset::row::to(std::tuple< T& ... > val) const
{
    std::tuple<T ... > non_ref;
    detail::row_data_extractor< T ... >::get_tuple(*this, non_ref);
    val = non_ref;
}

template < typename ... T >
void
resultset::row::to(T& ... val) const
{
    detail::row_data_extractor< T ... >::get_values(*this, val ...);
}

template < typename ... T >
void
resultset::row::to(::std::initializer_list<::std::string> const& names,
        ::std::tuple<T ...>& val) const
{
    detail::row_data_by_name_extractor<T...>::get_tuple(*this, names, val);
}

template < typename ... T >
void
resultset::row::to(::std::initializer_list<::std::string> const& names,
        ::std::tuple<T& ...> val) const
{
    std::tuple<T ... > non_ref;
    detail::row_data_by_name_extractor<T...>::get_tuple(*this, names, non_ref);
    val = non_ref;
}

template < typename ... T >
void
resultset::row::to(::std::initializer_list<::std::string> const& names,
        T& ... val) const
{
    detail::row_data_by_name_extractor<T...>::get_values(*this, names, val...);
}

}  // namespace pg
}  // namespace db
}  // namespace tip


#endif /* TIP_DB_PG_RESULTSET_INL_ */
