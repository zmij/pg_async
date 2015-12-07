/*
 * world_reader.hpp
 *
 *  Created on: Sep 23, 2015
 *      Author: zmij
 */

#ifndef TIP_GAME_WORLD_GAME_DATA_READER_HPP_
#define TIP_GAME_WORLD_GAME_DATA_READER_HPP_

#include <memory>
#include <iosfwd>
#include <unordered_map>
#include <cereal/archives/json.hpp>

#include <boost/noncopyable.hpp>
#include <tip/l10n/message.hpp>

namespace awm {
namespace game {
namespace world {

/**
 * Base class for game designer data reader
 */
class game_data_reader : private boost::noncopyable {
public:
	virtual ~game_data_reader() {}
	/**
	 * Read input stream and store contents in internal cache in parsed structures.
	 * @param in
	 */
	virtual void
	read(std::string const& name, std::istream& in) = 0;

	/**
	 * Clear internal cache
	 */
	virtual void
	clear() = 0;
};

typedef std::shared_ptr<game_data_reader> game_data_reader_ptr;

template < typename Proto >
class proto_cache : public game_data_reader {
public:
	typedef Proto proto_type;
	typedef std::shared_ptr< proto_type > proto_ptr;
	typedef std::shared_ptr< proto_type const > const_proto_ptr;
	typedef std::unordered_map< std::string, const_proto_ptr > cache_container;
public:
	proto_cache() {}
	virtual ~proto_cache() {}

	virtual void
	read(std::string const& name, std::istream& in)
	{
		std::locale loc(in.getloc(), new tip::l10n::domain_name_facet("world"));
		in.imbue(loc);
		proto_ptr proto = std::make_shared< proto_type >();
		cereal::JSONInputArchive ar(in);
		proto->serialize(ar);
		cache_.insert(std::make_pair(name, proto));
	}

	virtual void
	clear()
	{
		cache_.clear();
	}

	bool
	empty() const
	{
		return cache_.empty();
	}

	const_proto_ptr
	get(std::string const& proto_id) const
	{
		auto f = cache_.find(proto_id);
		if (f == cache_.end())
			throw std::runtime_error("Prototype not found");
		return f->second;
	}

	bool
	exists(std::string const& proto_id) const
	{
		return cache_.count(proto_id) > 0;
	}

	cache_container const&
	cache() const
	{
		return cache_;
	}
private:
	cache_container cache_;
};

namespace traits {

template < typename Proto >
struct prototype_traits {
	typedef Proto				type;
	typedef proto_cache< type >	cache_type;
};

}  // namespace traits

}  // namespace world
}  // namespace game
}  // namespace awm


#endif /* TIP_GAME_WORLD_GAME_DATA_READER_HPP_ */
