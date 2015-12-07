/*
 * lru_cache.hpp
 *
 *  Created on: Aug 30, 2015
 *      Author: zmij
 */

#ifndef TIP_LRU_CACHE_LRU_CACHE_HPP_
#define TIP_LRU_CACHE_LRU_CACHE_HPP_

#include <unordered_map>
#include <functional>
#include <list>
#include <mutex>
#include <memory>
#include <chrono>

#include <iostream>
#include <sstream>

namespace tip {
namespace util {

namespace detail {

//@{
/** @name Implementation selection flags */
struct non_intrusive {
	enum {
		value = false
	};
};
struct intrusive {
	enum {
		value = true
	};
};
//@}

template < typename Clock >
struct clock_traits;
template < typename TimeType >
struct time_traits;


template < >
struct clock_traits< std::chrono::high_resolution_clock > {
	typedef std::chrono::high_resolution_clock	clock_type;
	typedef clock_type::time_point				time_type;
	typedef clock_type::duration				duration_type;

	static time_type
	now()
	{
		return clock_type::now();
	}
};
template < >
struct time_traits< std::chrono::high_resolution_clock::time_point > {
	typedef std::chrono::high_resolution_clock clock_type;
};


#ifdef POSIX_TIME_TYPES_HPP___
template < >
struct clock_traits< boost::posix_time::microsec_clock > {
	typedef boost::posix_time::microsec_clock	clock_type;
	typedef boost::posix_time::ptime			time_type;
	typedef boost::posix_time::time_duration	duration_type;

	static time_type
	now()
	{
		return clock_type::local_time();
	}
};
template < >
struct time_traits< boost::posix_time::ptime > {
	typedef boost::posix_time::microsec_clock clock_type;
};
#endif

template < typename Value, typename Key >
struct key_extraction_traits {
	typedef non_intrusive	type;
	typedef Key				key_type;
	typedef Value			value_type;
};

template < typename Value, typename Key >
struct key_extraction_traits< Value, std::function< Key(Value) > > {
	typedef intrusive										type;
	typedef Key												key_type;
	typedef Value											value_type;
	typedef std::function< key_type(value_type) >			get_key_function;
};

template < typename Value, typename Key >
struct key_extraction_traits< Value, std::function< Key(Value const&) > > {
	typedef intrusive										type;
	typedef Key												key_type;
	typedef Value											value_type;
	typedef std::function< key_type(value_type const&) >	get_key_function;
};

template < typename Value, typename TimeGet, typename TimeSet >
struct time_handling_traits;

template < typename Value, typename TimeType >
struct time_handling_traits< Value, TimeType, void > {
	typedef non_intrusive 								type;

	typedef time_traits< TimeType > 					time_traits_type;
	typedef typename time_traits_type::clock_type		clock_type;
	typedef clock_traits< clock_type > 					clock_traits_type;
	typedef typename clock_traits_type::time_type		time_type;
	typedef typename clock_traits_type::duration_type	duration_type;
};

template < typename Value, typename TimeType >
struct time_handling_traits<
		Value,
		std::function< TimeType(Value const&) >,
		std::function< void (Value&, TimeType) >> {
	typedef intrusive 									type;

	typedef time_traits< TimeType > 					time_traits_type;
	typedef typename time_traits_type::clock_type		clock_type;
	typedef clock_traits< clock_type > 					clock_traits_type;
	typedef typename clock_traits_type::time_type		time_type;
	typedef typename clock_traits_type::duration_type	duration_type;
	typedef std::function< TimeType(Value const&) >		get_time_function;
	typedef std::function< void (Value&, TimeType) >	set_time_function;
};

template < typename Value, typename TimeType >
struct time_handling_traits<
		Value,
		std::function< TimeType(Value) >,
		std::function< void (Value, TimeType) >> {
	typedef intrusive 									type;

	typedef time_traits< TimeType > 					time_traits_type;
	typedef typename time_traits_type::clock_type		clock_type;
	typedef clock_traits< clock_type > 					clock_traits_type;
	typedef typename clock_traits_type::time_type		time_type;
	typedef typename clock_traits_type::duration_type	duration_type;
	typedef std::function< TimeType(Value) >			get_time_function;
	typedef std::function< void (Value, TimeType) >		set_time_function;
};

template < typename KeyExtraction, typename TimeHandling >
struct cache_types {
	typedef KeyExtraction								key_extraction_type;
	typedef TimeHandling								time_handling_type;
	typedef typename key_extraction_type::key_type		key_type;
	typedef typename key_extraction_type::value_type	value_type;
	typedef typename time_handling_type::time_type		time_type;
	typedef typename time_handling_type::duration_type	duration_type;
	typedef typename
			time_handling_type::clock_traits_type		clock_traits_type;
};

template < typename CacheTypes, typename ValueHolder >
class cache_container {
public:
	typedef CacheTypes 									types;
	typedef ValueHolder 								value_holder;
	typedef typename types::value_type					value_type;
	typedef typename types::key_type 					key_type;
	typedef typename types::time_type					time_type;
	typedef typename types::duration_type				duration_type;
	typedef typename types::clock_traits_type			clock_traits_type;
	typedef std::shared_ptr< value_holder > 			element_type;
protected:
	typedef std::list< element_type > 					lru_list_type;
	typedef typename lru_list_type::iterator 			list_iterator;
	typedef std::unordered_map< key_type, list_iterator> lru_map_type;
	typedef std::function< key_type(element_type) >		get_key_function;
	typedef std::function< time_type(element_type) >	get_time_function;
	typedef std::function<void(element_type, time_type)>set_time_function;
	typedef std::recursive_mutex						mutex_type;
	typedef std::lock_guard<mutex_type>					lock_type;
public:
	cache_container()
	{
		throw std::logic_error("Cache container should be constructed with "
				"data extraction functions");
	}
	cache_container(get_key_function key_fn, get_time_function get_time_fn,
			set_time_function set_time_fn) :
				get_key_(key_fn), get_time_(get_time_fn), set_time_(set_time_fn)
	{
	}
protected:
	void
	put( key_type const& key, element_type elem)
	{
		lock_type lock(mutex_);
		erase(key);
		set_time_(elem, clock_traits_type::now());
		cache_list_.push_front(elem);
		cache_map_.insert(std::make_pair(key, cache_list_.begin()));
	}
public:
	void
	erase(key_type const& key)
	{
		lock_type lock(mutex_);
		auto f = cache_map_.find(key);
		if (f != cache_map_.end()) {
			cache_list_.erase(f->second);
			cache_map_.erase(f);
		}
	}
	value_type
	get(key_type const& key)
	{
		lock_type lock(mutex_);
		auto f = cache_map_.find(key);
		if (f == cache_map_.end()) {
			std::ostringstream os("No key ");
			os << key << " in cache of " << typeid(value_type).name();
			throw std::range_error(os.str());
		}
		cache_list_.splice(cache_list_.begin(), cache_list_, f->second);
		set_time_(*f->second, clock_traits_type::now());
		return (*f->second)->value_;
	}
	void
	shrink(size_t max_size)
	{
		lock_type lock(mutex_);
		while (cache_list_.size() > max_size) {
			auto last = cache_list_.end();
			--last;
			cache_map_.erase(get_key_(*last));
			cache_list_.pop_back();
		}
	}
	void
	expire(duration_type age)
	{
		lock_type lock(mutex_);
		time_type now = clock_traits_type::now();
		time_type eldest = now - age;
		for (auto p = cache_list_.rbegin();
				p != cache_list_.rend() && get_time_(*p) < eldest;) {
			cache_map_.erase(get_key_(*p));
			cache_list_.pop_back();
		}
	}
	void
	clear()
	{
		lock_type lock(mutex_);
		cache_list_.clear();
		cache_map_.clear();
	}
	bool
	exists(key_type const& key) const
	{
		return cache_map_.find(key) != cache_map_.end();
	}
	bool
	empty() const
	{
		return cache_list_.empty();
	}
	size_t
	size() const
	{
		return cache_list_.size();
	}
private:
	mutex_type			mutex_;
	lru_list_type		cache_list_;
	lru_map_type		cache_map_;

	get_key_function	get_key_;
	get_time_function	get_time_;
	set_time_function	set_time_;
};

template < typename KeyTag, typename TimeTag, typename KeyExtraction, typename TimeHandling >
struct cache_value_holder;

template < typename KeyExtraction, typename TimeHandling >
struct cache_value_holder < non_intrusive, non_intrusive, KeyExtraction, TimeHandling > {
	typedef cache_types < KeyExtraction, TimeHandling > types;
	typename types::key_type 	key_;
	typename types::value_type	value_;
	typename types::time_type	access_time_;
};

template < typename KeyExtraction, typename TimeHandling >
struct cache_value_holder < intrusive, non_intrusive, KeyExtraction, TimeHandling > {
	typedef cache_types < KeyExtraction, TimeHandling > types;
	typename types::value_type	value_;
	typename types::time_type	access_time_;
};

template < typename KeyExtraction, typename TimeHandling >
struct cache_value_holder < non_intrusive, intrusive, KeyExtraction, TimeHandling > {
	typedef cache_types < KeyExtraction, TimeHandling > types;
	typename types::key_type 	key_;
	typename types::value_type	value_;
};

template < typename KeyExtraction, typename TimeHandling >
struct cache_value_holder < intrusive, intrusive, KeyExtraction, TimeHandling > {
	typedef cache_types < KeyExtraction, TimeHandling > types;
	typename types::value_type	value_;
};

template < typename KeyTag, typename TimeTag, typename KeyExtraction, typename TimeHandling >
class basic_cache;

template < typename KeyExtraction, typename TimeHandling >
class basic_cache <non_intrusive, non_intrusive, KeyExtraction, TimeHandling > :
		public cache_container<
				cache_types< KeyExtraction, TimeHandling >,
				cache_value_holder< non_intrusive, non_intrusive,
						KeyExtraction, TimeHandling >
			> {
public:
	typedef cache_types < KeyExtraction, TimeHandling >		types;
	typedef cache_value_holder< non_intrusive, non_intrusive,
							KeyExtraction, TimeHandling >	value_holder_type;
	typedef cache_container< types, value_holder_type >		base_type;
	typedef typename base_type::element_type				element_type;
public:
	basic_cache() :
		base_type(
			[](element_type elem)
			{ return elem->key_; },
			[](element_type elem)
			{ return elem->access_time_; },
			[](element_type elem, typename types::time_type tm)
			{ elem->access_time_ = tm; }
		)
	{}

	void
	put(typename types::key_type const& key, typename types::value_type const& value)
	{
		base_type::put(key,
				element_type( new value_holder_type{ key, value }) );
	}
};

template < typename KeyExtraction, typename TimeHandling >
class basic_cache<intrusive, non_intrusive, KeyExtraction, TimeHandling > :
		public cache_container<
				cache_types< KeyExtraction, TimeHandling >,
				cache_value_holder< intrusive, non_intrusive,
						KeyExtraction, TimeHandling >
			> {
public:
	typedef cache_types < KeyExtraction, TimeHandling >		types;
	typedef cache_value_holder< intrusive, non_intrusive,
							KeyExtraction, TimeHandling >	value_holder_type;
	typedef cache_container< types, value_holder_type >		base_type;
	typedef typename base_type::element_type				element_type;
	typedef typename types::key_extraction_type				key_extraction_type;
	typedef typename key_extraction_type::get_key_function	get_key_function;
public:
	basic_cache() : base_type()
	{
	}
	basic_cache(get_key_function get_key) :
		base_type(
			[get_key](element_type elem)
			{ return get_key( elem->value_ ); },
			[](element_type elem)
			{ return elem->access_time_; },
			[](element_type elem, typename types::time_type tm)
			{ elem->access_time_ = tm; }
		), get_key_(get_key)
	{}

	void
	put(typename types::value_type const& value)
	{
		typename types::key_type const& key = get_key_(value);
		base_type::put(key,
				element_type( new value_holder_type{ value }) );
	}
private:
	get_key_function get_key_;
};

template < typename KeyExtraction, typename TimeHandling >
class basic_cache<intrusive, intrusive, KeyExtraction, TimeHandling > :
		public cache_container<
				cache_types< KeyExtraction, TimeHandling >,
				cache_value_holder< intrusive, intrusive,
						KeyExtraction, TimeHandling >
			> {
public:
	typedef cache_types < KeyExtraction, TimeHandling > 	types;
	typedef cache_value_holder< intrusive, intrusive,
							KeyExtraction, TimeHandling > 	value_holder_type;
	typedef cache_container< types, value_holder_type > 	base_type;
	typedef typename base_type::element_type				element_type;
	typedef typename types::key_extraction_type				key_extraction_type;
	typedef typename types::time_handling_type				time_handling_type;
	typedef typename key_extraction_type::get_key_function	get_key_function;
	typedef typename time_handling_type::get_time_function	get_time_function;
	typedef typename time_handling_type::set_time_function	set_time_function;
public:
	basic_cache() : base_type()
	{
	}
	basic_cache(
			get_key_function get_key,
			get_time_function get_time,
			set_time_function set_time) :
		base_type(
			[get_key](element_type elem)
			{ return get_key( elem->value_ ); },
			[get_time](element_type elem)
			{ return get_time(elem->value_); },
			[set_time](element_type elem, typename types::time_type tm)
			{ set_time(elem->value_, tm); }
		), get_key_(get_key)
	{}
	void
	put(typename types::value_type const& value)
	{
		typename types::key_type const& key = get_key_(value);
		base_type::put(key,
				element_type( new value_holder_type{ value }) );
	}
private:
	get_key_function get_key_;
};

template < typename KeyExtraction, typename TimeHandling >
class basic_cache<non_intrusive, intrusive, KeyExtraction, TimeHandling> :
		public cache_container<
				cache_types< KeyExtraction, TimeHandling >,
				cache_value_holder< non_intrusive, intrusive,
						KeyExtraction, TimeHandling >
			> {
public:
	typedef cache_types < KeyExtraction, TimeHandling >		types;
	typedef cache_value_holder< non_intrusive, intrusive,
							KeyExtraction, TimeHandling >	value_holder_type;
	typedef cache_container< types, value_holder_type >		base_type;
	typedef typename base_type::element_type				element_type;
	typedef typename types::time_handling_type				time_handling_type;
	typedef typename time_handling_type::get_time_function	get_time_function;
	typedef typename time_handling_type::set_time_function	set_time_function;
public:
	basic_cache() : base_type()
	{
	}
	basic_cache(
			get_time_function get_time,
			set_time_function set_time) :
		base_type(
			[](element_type elem)
			{ return elem->key_; },
			[get_time](element_type elem)
			{ return get_time(elem->value_); },
			[set_time](element_type elem, typename types::time_type tm)
			{ set_time(elem->value_, tm); }
		)
	{}
	void
	put(typename types::key_type const& key, typename types::value_type const& value)
	{
		base_type::put(key,
				element_type( new value_holder_type{ key, value }) );
	}
};

template < typename Value, typename Key,
		typename T0 = std::chrono::high_resolution_clock::time_point,
		typename T1 = void >
struct cache_traits {
	typedef Value										value_type;
	typedef key_extraction_traits< Value, Key >			key_extraction_type;
	typedef time_handling_traits< Value, T0, T1 >		time_handling_type;
	typedef typename key_extraction_type::key_type		key_type;
	typedef typename time_handling_type::clock_type		clock_type;
	typedef typename time_handling_type::time_type		time_type;
	typedef typename time_handling_type::duration_type	duration_type;

	typedef typename key_extraction_type::type			key_intrusive;
	typedef typename time_handling_type::type			time_intrusive;

	typedef basic_cache<
			key_intrusive,
			time_intrusive,
			key_extraction_type,
			time_handling_type
		> cache_base_type;
};
}  // namespace detail

template < typename ValueType,
	typename KeyType,
	typename GetTime = std::chrono::high_resolution_clock::time_point,
	typename SetTime = void >
class lru_cache :
		public detail::cache_traits< ValueType, KeyType, GetTime, SetTime >::cache_base_type {
public:
	typedef lru_cache< ValueType, KeyType, GetTime, SetTime >		this_type;
	typedef detail::cache_traits<
			ValueType, KeyType, GetTime, SetTime >					traits_type;
	typedef typename traits_type::value_type						value_type;
	typedef typename traits_type::key_type							key_type;
	typedef typename traits_type::time_type							time_type;
	typedef typename traits_type::cache_base_type					base_type;
	typedef typename traits_type::key_intrusive						key_intrusive;
	typedef typename traits_type::time_intrusive					time_intrusive;
public:
	lru_cache()
		: base_type()
	{
	}
	template < typename U = this_type,
		typename SFINAE = typename
			std::enable_if< U::key_intrusive::value && !U::time_intrusive::value >::type >
	lru_cache(typename U::key_extraction_type::get_key_function key_extract)
		: base_type(key_extract)
	{
	}
	template < typename U = this_type, typename SFINAE =
			typename std::enable_if< U::key_intrusive::value && U::time_intrusive::value >::type >
	lru_cache(typename U::key_extraction_type::get_key_function key_extract,
			typename U::time_handling_type::get_time_function get_time,
			typename U::time_handling_type::set_time_function set_time)
		: base_type(key_extract, get_time, set_time)
	{
	}
	template < typename U = this_type, typename SFINAE =
			typename std::enable_if< !U::key_intrusive::value && U::time_intrusive::value >::type >
	lru_cache(typename U::time_handling_type::get_time_function get_time,
			typename U::time_handling_type::set_time_function set_time)
		: base_type(get_time, set_time)
	{
	}
};

}  // namespace util
}  // namespace tip

#endif /* TIP_LRU_CACHE_LRU_CACHE_HPP_ */
