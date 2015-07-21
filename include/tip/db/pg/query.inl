/*
 * query.inl
 *
 *  Created on: Jul 20, 2015
 *      Author: zmij
 */

#ifndef LIB_PG_ASYNC_INCLUDE_TIP_DB_PG_QUERY_INL_
#define LIB_PG_ASYNC_INCLUDE_TIP_DB_PG_QUERY_INL_

#include <tip/db/pg/query.inl>
#include <tip/util/meta_helpers.hpp>
#include <tip/db/pg/protocol_io_traits.hpp>

namespace tip {
namespace db {
namespace pg {

namespace detail {

template < size_t Index, typename T >
struct nth_param {
	enum {
		index = Index
	};
	typedef T type;
	typedef traits::best_formatter<type> best_formatter;
	static constexpr protocol_data_format data_format = best_formatter::value;
	typedef typename best_formatter::type formatter_type;

	static bool
	write_format( std::vector<byte>& buffer )
	{
		return protocol_write< BINARY_DATA_FORMAT >(buffer, (smallint)data_format);
	}

	static bool
	write_value (std::vector<byte>& buffer, type const& value)
	{
		// space for length
		buffer.resize(buffer.size() + sizeof(integer));
		std::vector<byte>::iterator sz_iter = buffer.begin() +
				(buffer.size() - sizeof(integer));
		size_t prev_size = buffer.size();
		protocol_write< data_format >(buffer, value);
		integer len = buffer.size() - prev_size;
		// write length
		return protocol_write< BINARY_DATA_FORMAT >( sz_iter, len );
	}
};

template < size_t Index, typename T >
struct nth_param < Index, boost::optional< T > > {
	enum {
		index = Index
	};
	typedef T type;
	typedef traits::best_formatter<type> best_formatter;
	static constexpr protocol_data_format data_format = best_formatter::value;
	typedef typename best_formatter::type formatter_type;

	static bool
	write_format( std::vector<byte>& buffer )
	{
		return protocol_write< BINARY_DATA_FORMAT >(buffer, (smallint)data_format);
	}

	static bool
	write_value (std::vector<byte>& buffer, boost::optional<type> const& value)
	{
		if (value) {
			return nth_param< Index, T >::write_value( buffer, *value );
		}
		// NULL value
		return protocol_write< BINARY_DATA_FORMAT >( buffer, (integer)-1 );
	}
};


template < size_t Index, typename ... T >
struct format_selector;

template < typename T >
struct format_selector< 0, T > {
	enum {
		index = 0
	};
	typedef T type;
	typedef traits::best_formatter<type> best_formatter;
	static constexpr protocol_data_format data_format = best_formatter::value;
	typedef typename best_formatter::type formatter_type;

	static constexpr bool single_format = true;
};

template < size_t N, typename T, typename ... Y >
struct format_selector< N, T, Y ... > {
	enum {
		index = N
	};
	typedef T type;
	typedef traits::best_formatter<type> best_formatter;
	static constexpr protocol_data_format data_format = best_formatter::value;
	typedef typename best_formatter::type formatter_type;
	typedef format_selector< index - 1, Y ...> prev_param_type;

	static constexpr bool single_format =
			prev_param_type::single_format &&
				prev_param_type::data_format == data_format;
};

template < typename ... T >
struct is_single_format {
	enum {
		size = sizeof ... (T),
		last_index = size - 1
	};
	typedef format_selector< last_index, T... > last_selector;
	static constexpr bool value = last_selector::single_format;
};



template < bool, typename IndexTuple, typename ... T >
struct param_format_builder;

template < size_t ... Indexes, typename ... T >
struct param_format_builder< true, util::indexes_tuple< Indexes ... >, T ... > {
	enum {
		size = sizeof ... (T),
		last_index = size - 1
	};

	typedef format_selector< last_index, T... > last_selector;
	static constexpr protocol_data_format  data_format = last_selector::data_format;

	bool
	static write_params( std::vector<byte>& buffer, T const& ... args )
	{
		protocol_write<BINARY_DATA_FORMAT>(buffer, (smallint)data_format);
		util::expand(nth_param< Indexes, T >::write_value(buffer, args) ...);
		return true;
	}

};

template < size_t ... Indexes, typename ... T >
struct param_format_builder< false, util::indexes_tuple< Indexes ... >, T ... > {
	enum {
		size = sizeof ... (T),
		last_index = size - 1
	};

	typedef format_selector< last_index, T... > last_selector;
	static constexpr protocol_data_format  data_format = last_selector::data_format;

	bool
	static write_params( std::vector<byte>& buffer, T const& ... args )
	{
		protocol_write<BINARY_DATA_FORMAT>(buffer, (smallint)size);
		util::expand(nth_param< Indexes, T >::write_format(buffer) ...);
		util::expand(nth_param< Indexes, T >::write_value(buffer, args) ...);
		return true;
	}

};


template < typename ... T >
struct param_formatter : param_format_builder <
		is_single_format< T ... >::value,
		typename util::index_builder< sizeof ... (T) >::type,
		T ... > {

};

struct ___no_binary_format {};
static_assert( !is_single_format< smallint, integer, bigint, ___no_binary_format >::value,
		"No single format");

static_assert( is_single_format< smallint, integer, bigint >::value,
		"Single format for integral types" );
static_assert( param_formatter< smallint, integer, bigint >::data_format
			== BINARY_DATA_FORMAT,
		"Binary format for integral types");

template < typename ... T >
void
write_params(std::vector<byte>& buffer, T const& ... params)
{
	param_formatter< T ... >::write_params(buffer, params ...);
}

}  // namespace detail

template < typename ... T >
query::query(dbalias const& alias, std::string const& expression,
		bool start_tran, bool autocommit, T const& ... params)
{
	create_impl(alias, expression, start_tran, autocommit);
	bind_params(params ...);
}

template < typename ... T >
query::query(connection_lock_ptr c, std::string const& expression,
		T const& ... params)
{
	create_impl(c, expression);
	bind_params(params ...);
}

template < typename ... T >
void
query::bind(T const& ... params)
{
	// 1. Write format codes
	// 	- detect if all of param types have binary formatters
	//	- write a binary format code (1) if all have binary formatters
	//	- write format codes for each param
	//	- evaluate buffer length
	// 2. Params
	//  - write the number of params
	//  - write each param preceded by it's length
	detail::write_params(buffer(), params ...);
}

}  // namespace pg
}  // namespace db
}  // namespace tip

#endif /* LIB_PG_ASYNC_INCLUDE_TIP_DB_PG_QUERY_INL_ */
