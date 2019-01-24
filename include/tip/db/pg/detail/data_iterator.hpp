#ifndef TIP_DB_PG_DETAIL_DATA_ITERATOR_HPP_
#define TIP_DB_PG_DETAIL_DATA_ITERATOR_HPP_

#include <iterator>

namespace tip {
namespace db {
namespace pg {
namespace detail {

template < typename FinalType, typename DataType >
class data_iterator : public DataType {
public:
  using iterator_type = FinalType;

  //@{
  /** @iterator Concept */
  using value_type = DataType;
  using difference_type = int;
  using reference = value_type;
  using pointer = value_type const*;
  using iterator_category = std::random_access_iterator_tag;
  //@}
public:
  //@{
  /** @name Iterator dereferencing */
  reference operator*() const { return reference{*this}; }
  pointer operator->() const { return this; }
  //@}

  //@{
  operator bool() const
  {
    return rebind().valid();
  }
  bool
  operator! () const
  {
    return !rebind().valid();
  }

  //@}

  //@{
  /** @name Iterator movement */
  iterator_type& operator++() { return do_advance(1); }
  iterator_type operator++(int) {
    iterator_type prev{rebind()};
    do_advance(1);
    return prev;
  }

  iterator_type& operator--() { return do_advance(-1); }
  iterator_type operator--(int) {
    iterator_type prev{rebind()};
    do_advance(-1);
    return prev;
  }

  iterator_type& operator += (difference_type distance) { return do_advance(distance); }
  iterator_type& operator -= (difference_type distance) { return do_advance(-distance); }
  //@}

  //@{
  /** @name Iterator comparison */
  bool operator ==(iterator_type const& rhs) const {
    return do_compare(rhs) == 0;
  }

  bool operator !=(iterator_type const& rhs) const {
    return !(*this == rhs);
  }

  bool operator <(iterator_type const& rhs) const {
    return do_compare(rhs) < 0;
  }
  bool operator <=(iterator_type const& rhs) const {
    return do_compare(rhs) <= 0;
  }
  bool operator >(iterator_type const& rhs) const {
    return do_compare(rhs) > 0;
  }
  bool operator >=(iterator_type const& rhs) const {
    return do_compare(rhs) >= 0;
  }
  //@}
protected:
  template < typename ... T >
  data_iterator(T ... args) : value_type(::std::forward<T>(args)...) {}
private:
  iterator_type&
  rebind()
  { return static_cast<iterator_type&>(*this); }

  iterator_type const&
  rebind() const
  { return static_cast<const iterator_type&>(*this); }

  iterator_type&
  do_advance(difference_type distance)
  { return rebind().advance(distance); }

  int
  do_compare(iterator_type const& lhs) const
  { return rebind().compare(lhs); }
};

}  // namespace detail
}  // namespace pg
}  // namespace db
}  // namespace tip

#endif /* TIP_DB_PG_DETAIL_DATA_ITERATOR_HPP_ */


