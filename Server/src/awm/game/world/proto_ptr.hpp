/**
 * proto_ptr.hpp
 *
 *  Created on: 28 сент. 2015 г.
 *      Author: zmij
 */

#ifndef TIP_GAME_WORLD_PROTO_PTR_HPP_
#define TIP_GAME_WORLD_PROTO_PTR_HPP_

#include <awm/game/world/world.hpp>

namespace awm {
namespace game {
namespace world {

template < typename Proto >
class proto_ptr {
public:
	typedef Proto value_type;
	typedef proto_ptr< value_type > this_type;
	typedef std::shared_ptr< value_type const > proto_shared_ptr;
	typedef value_type const* pointer;
	typedef value_type const& reference;
public:
	proto_ptr() {}
	explicit
	proto_ptr(std::string const& proto_id) : proto_id_(proto_id) {}

	void
	swap(this_type& rhs)
	{
		std::swap(proto_id_, rhs.proto_id_);
	}

	bool
	empty() const
	{
		return proto_id_.empty();
	}

	bool
	valid() const
	{
		return !empty() && proto_exists< value_type >(proto_id_);
	}

	operator bool() const
	{
		return valid();
	}

	pointer
	operator ->() const
	{
		return get_proto< value_type >(proto_id_).get();
	}
	reference
	operator *() const
	{
		return *get_proto< value_type >(proto_id_);
	}

	proto_shared_ptr
	lock() const
	{
		return get_proto< value_type >(proto_id_);
	}

	std::string&
	proto_id()
	{
		return proto_id_;
	}
	std::string const&
	proto_id() const
	{
		return proto_id_;
	}
private:
	std::string proto_id_;
};

}  // namespace world
}  // namespace game
}  // namespace awm

#endif /* TIP_GAME_WORLD_PROTO_PTR_HPP_ */
