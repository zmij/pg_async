/*
 * world.h
 *
 *  Created on: 3 сент. 2015 г.
 *      Author: zelenov
 */

#ifndef TIP_GAME_WORLD_WORLD_HPP_
#define TIP_GAME_WORLD_WORLD_HPP_

#include <string>
#include <map>
#include <boost/noncopyable.hpp>
#include <awm/game/world/game_data_reader.hpp>
#include <tip/util/facet.hpp>

namespace awm {
namespace game {
namespace world {

class world : private boost::noncopyable {
public:
	static world&
	instance();

	void
	add_data_reader(std::string const& path, game_data_reader_ptr reader);

	template < typename T >
	void
	add_data_cache(std::string const& path, std::shared_ptr< T > cache);

	void
	import(std::string const& root);
private:
	world();

	template < typename Proto >
	friend typename traits::prototype_traits< Proto >::cache_type&
	use_proto_cache();

	template < typename Proto >
	friend std::shared_ptr< Proto const >
	get_proto(std::string const& name);

	template < typename Proto >
	friend bool
	proto_exists(std::string const& name);

	typedef tip::util::facet_registry< game_data_reader > cache_registry_type;

	cache_registry_type&
	get_cache_registry();
private:
	struct impl;
	typedef std::shared_ptr<impl> pimpl;
	pimpl pimpl_;
};

template < typename Proto >
typename traits::prototype_traits< Proto >::cache_type&
use_proto_cache();

template < typename Proto >
std::shared_ptr< Proto const >
get_proto(std::string const& proto_id);

template < typename Proto >
bool
proto_exists(std::string const& proto_id);

} /* namespace world */
} /* namespace game */
} /* namespace awm */

#include <awm/game/world/world.inl>

#endif /* TIP_GAME_WORLD_WORLD_HPP_ */
