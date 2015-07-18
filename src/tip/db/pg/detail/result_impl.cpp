/*
 * result_impl.cpp
 *
 *  Created on: 12 июля 2015 г.
 *     @author: zmij
 */

#include <tip/db/pg/detail/result_impl.hpp>
#include <iostream>
#include <sstream>
#include <string>
#include <exception>

namespace tip {
namespace db {
namespace pg {
namespace detail {

result_impl::result_impl()
{
}

size_t
result_impl::size() const
{
	return rows_.size();
}

bool
result_impl::empty() const
{
	return rows_.empty();
}

void
result_impl::check_row_index(uint32_t row) const
{
	if (row >= rows_.size()) {
		std::ostringstream out;
		out << "Row index " << row << " is out of bounds [0.."
				<< rows_.size() << ")";
		throw std::out_of_range(out.str().c_str());
	}
}

field_buffer
result_impl::at(uint32_t row, uint16_t col) const
{
	check_row_index(row);
	row_data const& rd = rows_[row];
	return rd.field_data(col);
}

bool
result_impl::is_null(uint32_t row, uint16_t col) const
{
	check_row_index(row);
	row_data const& rd = rows_[row];
	return rd.is_null(col);
}

row_data::data_buffer_bounds
result_impl::buffer_bounds(uint32_t row, uint16_t col) const
{
	check_row_index(row);
	row_data const& rd = rows_[row];
	return rd.field_buffer_bounds(col);
}

} /* namespace detail */
} /* namespace pg */
} /* namespace db */
} /* namespace tip */
