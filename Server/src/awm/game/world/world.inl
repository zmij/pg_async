/*
 * world.inl
 *
 *  Created on: Sep 25, 2015
 *      Author: zmij
 */

#ifndef TIP_GAME_WORLD_WORLD_INL_
#define TIP_GAME_WORLD_WORLD_INL_

#include <awm/game/world/world.hpp>

namespace awm {
namespace game {
namespace world {

template < typename T >
void
world::add_data_cache(std::string const& path, std::shared_ptr< T > cache)
{
	add_data_reader(path, cache);
	// Add the cache to cache registry
	cache_registry_type& reg = get_cache_registry();
	reg.add_facet(cache.get());
}

template < typename Proto >
typename traits::prototype_traits< Proto >::cache_type&
use_proto_cache()
{
	typedef Proto									proto_type;
	typedef traits::prototype_traits< proto_type >	traits_type;
	typedef typename traits_type::cache_type		cache_type;

	world::cache_registry_type& reg = world::instance().get_cache_registry();
	return reg.use_facet< cache_type >();
}

template < typename Proto >
std::shared_ptr< Proto const >
get_proto(std::string const& proto_id)
{
	typedef Proto									proto_type;
	typedef traits::prototype_traits< proto_type >	traits_type;
	typedef typename traits_type::cache_type		cache_type;

	cache_type& c = use_proto_cache< proto_type >();

	return c.get(proto_id);
}

template < typename Proto >
bool
proto_exists(std::string const& proto_id)
{
	typedef Proto									proto_type;
	typedef traits::prototype_traits< proto_type >	traits_type;
	typedef typename traits_type::cache_type		cache_type;

	cache_type& c = use_proto_cache< proto_type >();
	return c.exists(proto_id);
}


}  // namespace world
}  // namespace game
}  // namespace awm

#endif /* TIP_GAME_WORLD_WORLD_INL_ */
