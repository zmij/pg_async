/*
 * result.cpp
 *
 *  Created on: 12 июля 2015 г.
 *     @author: zmij
 */

#include <tip/db/pg/resultset.hpp>
#include <tip/db/pg/detail/protocol.hpp>
#include <tip/db/pg/detail/result_impl.hpp>

#include <tip/db/pg/log.hpp>

#include <algorithm>
#include <assert.h>

namespace tip {
namespace db {
namespace pg {

LOCAL_LOGGING_FACILITY_CFG(PGRESULT, config::QUERY_LOG);

const resultset::size_type resultset::npos = std::numeric_limits<resultset::size_type>::max();
const resultset::row::size_type resultset::row::npos = std::numeric_limits<resultset::row::size_type>::max();

//----------------------------------------------------------------------------
// result::row implementation
//----------------------------------------------------------------------------
resultset::row::size_type
resultset::row::size() const
{
	// TODO get a row from pimpl and return it's size
	return result_->columns_size();
}

bool
resultset::row::empty() const
{
	// FIXME implement this
	return false;
}

resultset::row::const_iterator
resultset::row::begin() const
{
	return const_iterator(result_, row_index_, 0);
}

resultset::row::const_iterator
resultset::row::end() const
{
	return const_iterator(result_, row_index_, size());
}

resultset::row::const_reverse_iterator
resultset::row::rbegin() const
{
	return const_reverse_iterator(const_iterator(result_, row_index_, size() - 1));
}

resultset::row::const_reverse_iterator
resultset::row::rend() const
{
	return const_reverse_iterator(const_iterator(result_, row_index_, npos));
}

resultset::row::reference
resultset::row::operator [](size_type col_index) const
{
	// TODO check the index
	return reference(result_, row_index_, col_index);
}

resultset::row::reference
resultset::row::operator [](std::string const& name) const
{
	size_type col_index = result_->index_of_name(name);
	return (*this)[col_index];
}

//----------------------------------------------------------------------------
// result::const_row_iterator implementation
//----------------------------------------------------------------------------
int
resultset::const_row_iterator::compare(const_row_iterator const& rhs) const
{
	if (!(*this) && !rhs) // invalid iterators are equal
		return 0;
	assert(result_ == rhs.result_
			&& "Cannot compare iterators in different result sets");
	if (row_index_ != rhs.row_index_)
		return (row_index_ < rhs.row_index_) ? -1 : 1;
	return 0;
}

resultset::const_row_iterator&
resultset::const_row_iterator::advance(difference_type distance)
{
	if (*this) {
		// movement is defined only for valid iterators
		difference_type target = distance + row_index_;
		if (target < 0 || target > result_->size()) {
			// invalidate the iterator
			row_index_ = npos;
		} else {
			row_index_ = target;
		}
	} else if (result_) {
		if (distance == 1) {
			// When a non-valid iterator that belongs to a result set
			// is incremented it is moved to the beginning of sequence.
			// This is to support rend iterator moving
			// to the beginning of sequence.
			row_index_ = 0;
		} else if (distance == -1) {
			// When a non-valid iterator that belongs to a result set
			// is decremented it is moved to the end of sequence.
			// This is to support end iterator moving
			// to the end of sequence.
			row_index_ = result_->size() - 1;
		}
	}
	return *this;
}

//----------------------------------------------------------------------------
// result::field implementation
//----------------------------------------------------------------------------
std::string const&
resultset::field::name() const
{
	assert(result_ && "Cannot get field name not bound to result set");
	return result_->field_name(field_index_);
}

field_description const&
resultset::field::description() const
{
	assert(result_ && "Cannot get field description not bound to result set");
	return result_->field(field_index_);
}

bool
resultset::field::is_null() const
{
	return result_->is_null(row_index_, field_index_);
}

field_buffer
resultset::field::input_buffer() const
{
	return result_->at(row_index_, field_index_);
}

//----------------------------------------------------------------------------
// result::const_field_iterator implementation
//----------------------------------------------------------------------------
int
resultset::const_field_iterator::compare(const_field_iterator const& rhs) const
{
	if (!(*this) && !rhs) // invalid iterators are equal
		return 0;
	assert(result_ == rhs.result_
			&& "Cannot compare iterators in different result sets");
	assert(row_index_ == rhs.row_index_
			&& "Cannot compare iterators in different data rows");
	if (field_index_ != rhs.field_index_)
		return field_index_ < rhs.field_index_ ? -1 : 1;
	return 0;
}

resultset::const_field_iterator&
resultset::const_field_iterator::advance(difference_type distance)
{
	if (*this) {
		// movement is defined only for valid iterators
		difference_type target = distance + field_index_;
		if (target < 0 || target > result_->size()) {
			// invalidate the iterator
			field_index_ = row::npos;
		} else {
			field_index_ = target;
		}
	} else if (result_) {
		if (distance == 1) {
			// When a non-valid iterator that belongs to a result set
			// is incremented it is moved to the beginning of sequence.
			// This is to support rend iterator moving
			// to the beginning of sequence.
			field_index_ = 0;
		} else if (distance == -1) {
			// When a non-valid iterator that belongs to a result set
			// is decremented it is moved to the end of sequence.
			// This is to support end iterator moving
			// to the end of sequence.
			field_index_ = result_->columns_size() - 1;
		}
	}
	return *this;
}

//----------------------------------------------------------------------------
// result implementation
//----------------------------------------------------------------------------
resultset::resultset()
	: pimpl_(new detail::result_impl)
{
}
resultset::resultset(result_impl_ptr impl)
	: pimpl_(impl)
{
}

resultset::size_type
resultset::size() const
{
	return pimpl_->size();
}

bool
resultset::empty() const
{
	return pimpl_->empty();
}

resultset::const_iterator
resultset::begin() const
{
	return const_iterator(this, 0);
}

resultset::const_iterator
resultset::end() const
{
	return const_iterator(this, size());
}

resultset::const_reverse_iterator
resultset::rbegin() const
{
	return const_reverse_iterator(const_iterator(this, size() - 1));
}

resultset::const_reverse_iterator
resultset::rend() const
{
	return const_reverse_iterator(const_iterator(this, npos));
}

resultset::reference
resultset::front() const
{
	assert(size() > 0 && "Cannot get row in an empty resultset");
	return row(this, 0);
}

resultset::reference
resultset::back() const
{
	assert(size() > 0 && "Cannot get row in an empty resultset");
	return row(this, size() - 1);
}

resultset::reference
resultset::operator [](size_type index) const
{
	assert(index < size() && "Index is out of bounds");
	return row(this, index);
}

resultset::size_type
resultset::columns_size() const
{
	return pimpl_->row_description().size();
}

row_description_type const&
resultset::row_description() const
{
	return pimpl_->row_description();
}

resultset::size_type
resultset::index_of_name(std::string const& name) const
{
	row_description_type const& descriptions
		= pimpl_->row_description();
	auto f = std::find_if(descriptions.begin(), descriptions.end(),
			[name](field_description const& fd) {
				return fd.name == name;
			});
	if (f != descriptions.end()) {
		return f - descriptions.begin();
	}
	return npos;
}

field_description const&
resultset::field(size_type col_index) const
{
	return pimpl_->row_description().at(col_index);
}

field_description const&
resultset::field(std::string const& name) const
{
	for (field_description const& fd : pimpl_->row_description()) {
		if (fd.name == name)
			return fd;
	}
	throw std::runtime_error("No field with name");
}

std::string const&
resultset::field_name(size_type index) const
{
	return field(index).name;
}

field_buffer
resultset::at(size_type r, row::size_type c) const
{
	return pimpl_->at(r, c);
}

bool
resultset::is_null(size_type r, row::size_type c) const
{
	return pimpl_->is_null(r, c);
}

}  // namespace pg
}  // namespace db
}  // namespace tip
