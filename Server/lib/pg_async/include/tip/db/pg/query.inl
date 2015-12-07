/*
 * query.inl
 *
 *  Created on: Jul 20, 2015
 *      Author: zmij
 */

#ifndef LIB_PG_ASYNC_INCLUDE_TIP_DB_PG_QUERY_INL_
#define LIB_PG_ASYNC_INCLUDE_TIP_DB_PG_QUERY_INL_

#include <tip/db/pg/query.hpp>
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
	typedef io::traits::best_formatter<type> best_formatter;
	static constexpr protocol_data_format data_format = best_formatter::value;
	typedef typename best_formatter::type formatter_type;

	static bool
	write_format( std::vector<byte>& buffer )
	{
		return io::protocol_write< BINARY_DATA_FORMAT >(buffer, (smallint)data_format);
	}

	static bool
	write_value (std::vector<byte>& buffer, type const& value)
	{
		// TODO Reserve space
		size_t buffer_pos = buffer.size();
		// space for length
		buffer.resize(buffer.size() + sizeof(integer));
		size_t prev_size = buffer.size();
		io::protocol_write< data_format >(buffer, value);
		integer len = buffer.size() - prev_size;
		// write length
		std::vector<byte>::iterator sz_iter = buffer.begin() +
				buffer_pos;
		io::protocol_write< BINARY_DATA_FORMAT >( sz_iter, len );
		integer written(0);
		io::protocol_read< BINARY_DATA_FORMAT > (sz_iter, sz_iter + sizeof(integer), written);
		assert(written == len && "Length is written ok");
		return true;
	}

	static integer
	size(type const& value)
	{
		return io::protocol_writer< data_format >(value).size() + sizeof(integer);
	}
};

template < size_t Index, typename T >
struct nth_param < Index, boost::optional< T > > {
	enum {
		index = Index
	};
	typedef T type;
	typedef io::traits::best_formatter<type> best_formatter;
	static constexpr protocol_data_format data_format = best_formatter::value;
	typedef typename best_formatter::type formatter_type;

	static bool
	write_format( std::vector<byte>& buffer )
	{
		return io::protocol_write< BINARY_DATA_FORMAT >(buffer, (smallint)data_format);
	}

	static bool
	write_value (std::vector<byte>& buffer, boost::optional<type> const& value)
	{
		if (value.is_initialized()) {
			return nth_param< Index, T >::write_value( buffer, *value );
		}
		// NULL value
		return io::protocol_write< BINARY_DATA_FORMAT >( buffer, (integer)-1 );
	}
	static integer
	size(boost::optional<type> const& value)
	{
		if (value.is_initialized())
			return io::protocol_writer< data_format >(*value).size() + sizeof(integer);
		return sizeof(integer);
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
	typedef io::traits::best_formatter<type> best_formatter;
	typedef typename best_formatter::type formatter_type;

	typedef io::traits::cpppg_data_mapping< T > data_mapping;

	static constexpr protocol_data_format data_format = best_formatter::value;
	static constexpr bool single_format = true;
	enum {
		type_oid = data_mapping::type_oid
	};

	static void
	write_type(type_oid_sequence& param_types)
	{
		static_assert( io::traits::cpppg_data_mapping< T >::type_oid != oids::type::unknown,
				"Parameter type doesn't have a PostgreSQL typeoid mapping" );
		param_types.push_back((oids::type::oid_type)type_oid);
	}
	static void
	write_format( std::vector<byte>& buffer )
	{
		io::protocol_write< BINARY_DATA_FORMAT >(buffer, (smallint)data_format);
	}

	static void
	write_param_value( std::vector<byte>& buffer, type const& value)
	{
		nth_param<index, T>::write_value(buffer, value);
	}

	static size_t
	size( type const& value )
	{
		return nth_param< index, T >::size(value);
	}
};

template < size_t N, typename T, typename ... Y >
struct format_selector< N, T, Y ... > {
	enum {
		index = N
	};
	typedef T type;
	typedef io::traits::best_formatter<type> best_formatter;
	typedef typename best_formatter::type formatter_type;

	typedef format_selector< N - 1, Y ...> next_param_type;

	typedef io::traits::cpppg_data_mapping< T > data_mapping;

	static constexpr protocol_data_format data_format = best_formatter::value;
	static constexpr bool single_format =
			next_param_type::single_format &&
				next_param_type::data_format == data_format;
	enum {
		type_oid = data_mapping::type_oid
	};

	static void
	write_type(type_oid_sequence& param_types)
	{
		static_assert( io::traits::cpppg_data_mapping< T >::type_oid != oids::type::unknown,
				"Parameter type doesn't have a PostgreSQL typeoid mapping" );
		param_types.push_back((oids::type::oid_type)type_oid);
		next_param_type::write_type(param_types);
	}
	static void
	write_format( std::vector<byte>& buffer )
	{
		io::protocol_write< BINARY_DATA_FORMAT >(buffer, (smallint)data_format);
		next_param_type::write_format(buffer);
	}

	static void
	write_param_value( std::vector<byte>& buffer, type const& value,
			Y const& ... next )
	{
		nth_param<index, T>::write_value(buffer, value);
		next_param_type::write_param_value(buffer, next ...);
	}
	static size_t
	size( type const& value, Y const& ... next )
	{
		return nth_param< index, T >::size(value) + next_param_type::size(next ...);
	}
};

/**
 * Metafunction to detect parameters protocol format
 */
template < typename ... T >
struct is_text_format {
	enum {
		size = sizeof ... (T),
		last_index = size - 1
	};
	typedef format_selector< last_index, T... > last_selector;
	static constexpr bool value = last_selector::single_format &&
			last_selector::data_format == TEXT_DATA_FORMAT;
};



template < bool, typename IndexTuple, typename ... T >
struct param_format_builder;

/**
 * Specialization for params that are only in text format
 */
template < size_t ... Indexes, typename ... T >
struct param_format_builder< true, util::indexes_tuple< Indexes ... >, T ... > {
	enum {
		size = sizeof ... (T),
		last_index = size - 1
	};

	typedef format_selector< last_index, T... > first_selector;
	static constexpr protocol_data_format  data_format = first_selector::data_format;


	bool
	static write_params( type_oid_sequence& param_types, std::vector<byte>& buffer,
			T const& ... args )
	{
		param_types.reserve(size);
		first_selector::write_type(param_types);

		size_t sz = sizeof(smallint) * 2 //text data format + count of params
				+ first_selector::size(args ...); // size of params
		buffer.reserve(sz);

		io::protocol_write<BINARY_DATA_FORMAT>(buffer, (smallint)data_format);
		io::protocol_write<BINARY_DATA_FORMAT>(buffer, (smallint)size);
		first_selector::write_param_value(buffer, args ...);
		return true;
	}

};

/**
 * Specialization for params that use binary format or mixed formats
 */
template < size_t ... Indexes, typename ... T >
struct param_format_builder< false, util::indexes_tuple< Indexes ... >, T ... > {
	enum {
		size = sizeof ... (T),
		last_index = size - 1
	};

	typedef format_selector< last_index, T... > first_selector;
	static constexpr protocol_data_format  data_format = first_selector::data_format;


	bool
	static write_params( type_oid_sequence& param_types, std::vector<byte>& buffer,
			T const& ... args )
	{
		param_types.reserve(size);
		first_selector::write_type(param_types);

		size_t sz = sizeof(smallint) * 2 // data format count + count of params
				+ sizeof(smallint) * size // data formats
				+ first_selector::size(args ...); // params
		buffer.reserve(sz);

		io::protocol_write<BINARY_DATA_FORMAT>(buffer, (smallint)size);
		first_selector::write_format(buffer);
		io::protocol_write<BINARY_DATA_FORMAT>(buffer, (smallint)size);
		first_selector::write_param_value(buffer, args ...);
		return true;
	}

};


template < typename ... T >
struct param_formatter : param_format_builder <
		is_text_format< T ... >::value,
		typename util::index_builder< sizeof ... (T) >::type,
		T ... > {

};

struct ___no_binary_format {};
static_assert( !is_text_format< smallint, integer, bigint, ___no_binary_format >::value,
		"No single format");

static_assert( !is_text_format< smallint, integer, bigint >::value,
		"Single format for integral types" );
static_assert( param_formatter< smallint, integer, bigint >::data_format
			== BINARY_DATA_FORMAT,
		"Binary format for integral types");

template < typename ... T >
void
write_params(std::vector< oids::type::oid_type >& param_types,
		std::vector<byte>& buffer, T const& ... params)
{
	param_formatter< T ... >::write_params(param_types, buffer, params ...);
}

}  // namespace detail

template < typename ... T >
query::query(dbalias const& alias, std::string const& expression,
		T const& ... params)
	: pimpl_(create_impl(alias, expression, params ...))
{
}

template < typename ... T >
query::query(transaction_ptr t, std::string const& expression,
		T const& ... params)
	: pimpl_(create_impl(t, expression, params ... ))
{
}

template < typename ... T >
query::pimpl
query::create_impl(dbalias const& alias, std::string const& expression,
		T const& ... params)
{
	type_oid_sequence ptypes;
	params_buffer buf;
	detail::write_params(ptypes, buf, params ...);
	return create_impl(alias, expression, std::move(ptypes), std::move(buf));
}

template < typename ... T >
query::pimpl
query::create_impl(transaction_ptr tran, std::string const& expression,
		T const& ... params)
{
	type_oid_sequence ptypes;
	params_buffer buf;
	detail::write_params(ptypes, buf, params ...);
	return create_impl(tran, expression, std::move(ptypes), std::move(buf));
}


template < typename ... T >
query&
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
	type_oid_sequence& ptypes = param_types();
	ptypes.clear();
	params_buffer& buf = buffer();
	buf.clear();
	detail::write_params(ptypes, buf, params ...);
	return *this;
}

}  // namespace pg
}  // namespace db
}  // namespace tip

#endif /* LIB_PG_ASYNC_INCLUDE_TIP_DB_PG_QUERY_INL_ */
