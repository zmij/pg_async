/*
 * resultset.inl
 *
 *  Created on: Jul 16, 2015
 *      Author: zmij
 */

#ifndef TIP_DB_PG_RESULTSET_INL_
#define TIP_DB_PG_RESULTSET_INL_

#include <tip/db/pg/resultset.hpp>

namespace tip {
namespace db {
namespace pg {

namespace detail {

/**
 * Metafunction for calculating Nth type in variadic template parameters
 */
template < size_t num, typename ... T >
struct nth_type;

template < size_t num, typename T, typename ... Y >
struct nth_type< num, T, Y ... > : nth_type< num - 1, Y ...> {
};

template < typename T, typename ... Y >
struct nth_type < 0, T, Y ... > {
	typedef T type;
};

template < size_t ... Indexes >
struct indexes_tuple {
	enum {
		size = sizeof ... (Indexes)
	};
};

template < size_t num, typename tp = indexes_tuple <> >
struct index_builder;

template < size_t num, size_t ... Indexes >
struct index_builder< num, indexes_tuple< Indexes ... > >
	: index_builder< num - 1, indexes_tuple< Indexes ..., sizeof ... (Indexes) > > {
};

template <size_t ... Indexes >
struct index_builder< 0, indexes_tuple< Indexes ... > > {
	typedef indexes_tuple < Indexes ... > type;
	enum {
		size = sizeof ... (Indexes)
	};
};

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

	resultset::row row;
};

template < typename IndexTuple, typename ... T >
struct tuple_builder_base;

template < size_t ... Indexes, typename ... T >
struct tuple_builder_base< indexes_tuple< Indexes ... >, T ... > {
	enum {
		size = sizeof ... (T)
	};

	static void
	get_tuple( resultset::row const& row, std::tuple< T ... >& val )
	{
		std::tuple< T ... > tmp( nth_field< Indexes, T >(row).value() ... );
		tmp.swap(val);
	}
};

template < typename ... T >
struct tuple_builder :
		tuple_builder_base < typename index_builder< sizeof ... (T) >::type, T ... > {
};

}  // namespace detail

template < typename ... T >
void
resultset::row::to(std::tuple< T ... >& val) const
{
	detail::tuple_builder< T ... >::get_tuple(*this, val);
}

template < typename ... T >
void
resultset::row::to(std::tuple< T& ... > val) const
{
	std::tuple<T ... > non_ref;
	detail::tuple_builder< T ... >::get_tuple(*this, non_ref);
	val = non_ref;
}

}  // namespace pg
}  // namespace db
}  // namespace tip


#endif /* TIP_DB_PG_RESULTSET_INL_ */
