/*
 * facet.hpp
 *
 *  Created on: Sep 25, 2015
 *      Author: zmij
 */

#ifndef TIP_UTIL_FACET_HPP_
#define TIP_UTIL_FACET_HPP_

#include <tuple>
#include <functional>
#include <stdexcept>

namespace tip {
namespace util {

/** @internal */
namespace detail {

template < typename BaseFacet >
struct facet_id {
	constexpr facet_id() = default;
	facet_id(facet_id const&) = delete;
	facet_id&
	operator = (facet_id const&) = delete;
};

template < typename BaseFacet, typename Facet >
struct facet_identifier {
	typedef facet_id< BaseFacet > id_type;
	static id_type id;
};

template < typename BaseFacet, typename Facet >
typename facet_identifier< BaseFacet, Facet >::id_type
		facet_identifier< BaseFacet, Facet >::id;

template < int ... >
struct index {};

template < int N, int ... S >
struct index_builder : index_builder< N - 1, N - 1, S... > {};

template < int ... S >
struct index_builder< 0, S... > {
	typedef index< S ... > type;
};

template < typename BaseFacet, typename ... Args >
struct facet_factory {
	typedef facet_factory< BaseFacet, Args ... >				factory_type;
	typedef BaseFacet											base_facet_type;
	typedef std::tuple< Args ... >								params_type;
	typedef typename index_builder< sizeof ... (Args) >::type	index_type;

	facet_factory() : params_() {}
	facet_factory( Args&& ... args ) : params_(std::forward<Args>(args) ...) {}
	facet_factory( Args const& ... args ) : params_(args ...) {}

	template < typename Facet >
	struct factory {
		typedef Facet	facet_type;
		factory_type const& factory_;
		base_facet_type*
		operator()() const
		{
			return create_impl(index_type());
		}
	private:
		template < int ... S >
		base_facet_type*
		create_impl(index<S...>) const
		{
			return new facet_type( std::get<S>(factory_.params_) ... );
		}
	};

	template < typename Facet >
	base_facet_type*
	create() const
	{
		typedef factory< Facet > factory_type;
		factory_type f{ *this };
		return f();
	}

private:
	template < typename Facet >
	friend struct factory;

	params_type params_;
};

template < typename BaseFacet >
struct facet_factory< BaseFacet > {
	typedef facet_factory< BaseFacet >	factory_type;
	typedef BaseFacet 					base_facet_type;
	facet_factory() {};

	template < typename Facet >
	base_facet_type*
	create()
	{
		return new Facet();
	}

	template < typename Facet >
	struct factory {
		typedef Facet	facet_type;
		factory_type const& factory_;
		base_facet_type*
		operator()() const
		{
			return new facet_type();
		}
	};
};

template < typename BaseFacet, typename ... Args >
class facet_registry_base {
public:
	typedef BaseFacet									facet_type;
	typedef facet_factory<facet_type, Args ...>			factory_type;
	typedef std::function< void (facet_type*) >			deleter_func_type;
private:
	typedef detail::facet_id< facet_type >				id_type;
	typedef std::function< facet_type* () >				factory_func;

	struct key {
		key() : type_info_(nullptr), id_(nullptr) {}

		std::type_info const* type_info_;
		id_type const* id_;

		bool
		operator == (key const& rhs)
		{
			if (id_ && rhs.id_ && id_ == rhs.id_)
				return true;
			if (type_info_ && rhs.type_info_ && type_info_ == rhs.type_info_)
				return true;
			return false;
		}
	};
	struct facet {
		key			key_;
		facet_type* facet_;
		facet*		next_;
	};
public:
	facet_registry_base(factory_type&& f) :
		factory_(std::forward<factory_type>(f)), first_facet_(nullptr),
		deleter_([]( facet_type* f ) { delete f; })
	{
	}

	facet_registry_base(deleter_func_type del, factory_type&& f) :
		factory_(std::forward<factory_type>(f)), first_facet_(nullptr),
		deleter_(del)
	{
	}

	~facet_registry_base()
	{
		clear();
	}

	facet_registry_base(facet_registry_base const&) = delete;
	facet_registry_base&
	operator = (facet_registry_base const&) = delete;

	template < typename Facet >
	void
	add_facet(Facet* new_facet)
	{
		static_assert(std::is_base_of< facet_type, Facet >::value,
				"All facets in a facet registry must derive from the same base type");
		key k = create_key<Facet>();
		if (find_facet(k))
			throw std::logic_error("Facet already exists");

		insert_facet(k, new_facet);
	}

	template < typename Facet >
	bool
	has_facet() const
	{
		static_assert(std::is_base_of< facet_type, Facet >::value,
				"All facets in a facet registry must derive from the same base type");
		key k = create_key<Facet>();
		return find_facet(k) != nullptr;
	}

	template < typename Facet >
	Facet&
	use_facet()
	{
		static_assert(std::is_base_of< facet_type, Facet >::value,
				"All facets in a facet registry must derive from the same base type");
		typedef typename factory_type::template factory< Facet > facet_constructor_type;
		key k = create_key<Facet>();
		facet_constructor_type c{ factory_ };
		facet* f = use_facet(k, c);
		return *static_cast< Facet* >(f->facet_);
	}

	void
	clear()
	{
		while (first_facet_) {
			facet* next = first_facet_->next_;
			deleter_(first_facet_->facet_);
			delete first_facet_;
			first_facet_ = next;
		}
	}
protected:
	void
	set_factory(factory_type&& f)
	{
		factory_ = std::forward< factory_type >(f);
	}
private:
	template < typename Facet >
	key
	create_key() const
	{
		typedef detail::facet_identifier<facet_type, Facet> facet_identifier;
		key k;
		k.id_ = &facet_identifier::id;
		return k;
	}

	facet*
	insert_facet(key const& k, facet_type* new_facet)
	{
		first_facet_ = new facet { k, new_facet, first_facet_ };
		return first_facet_;
	}

	facet*
	use_facet(key const& k, factory_func create)
	{
		facet* f = find_facet(k);
		if (!f) {
			f = insert_facet(k, create());
		}
		return f;
	}

	facet*
	find_facet(key const& k) const
	{
		facet* f = first_facet_;
		while (f) {
			if (f->key_ == k)
				return f;
			f = f->next_;
		}
		return nullptr;
	}
private:
	factory_type		factory_;
	facet*				first_facet_;
	deleter_func_type	deleter_;
};

}  // namespace detail
/** @endinternal */

/**
 * Facet registry class
 */
template < typename BaseFacet, typename ... Args >
class facet_registry :
		public detail::facet_registry_base< BaseFacet, Args ... > {
public:
	typedef detail::facet_registry_base< BaseFacet, Args ... >	base_type;
	typedef typename base_type::factory_type					factory_type;
	typedef typename base_type::deleter_func_type				deleter_func_type;
public:
	/**
	 * Create facet registry instance with default deleter and no arguments
	 * (default values for the types will be used).
	 */
	facet_registry() : base_type(factory_type()) {}
	/**
	 * Create facet registry instance with a deleter function and no arguments
	 * (default values for the types will be used).
	 * @param del Deleter function. Takes a pointer to facet as an argument.
	 */
	facet_registry(deleter_func_type del) : base_type(del, factory_type()) {}
	/**
	 * Create facet registry instance with default deleter and facet constructor
	 * arguments. Each facet constructor will receive the arguments.
	 * @param args Arguments passed to facets' constructors
	 */
	facet_registry(Args&& ... args) : base_type(factory_type(std::forward<Args>(args) ... )) {}
	/**
	 * Create facet registry instance with a deleter function and facet constructor
	 * arguments. Each facet constructor will receive the arguments.
	 * @param del Deleter function. Takes a pointer to facet as an argument.
	 * @param args Arguments passed to facets' constructors
	 */
	facet_registry(deleter_func_type del, Args&& ... args) : base_type(del, factory_type(std::forward(args) ... )) {}

	/**
	 * Set facet construction arguments.
	 * Handy in case when the arguments can be figured out only after the
	 * registry's construction, e.g. weak pointer to the registry's owner.
	 * @param args Arguments passed to facets' constructors
	 */
	void
	set_construction_args(Args&& ... args)
	{
		base_type::set_factory(factory_type(std::forward<Args>(args) ... ));
	}
	/**
	 * Set facet construction arguments.
	 * Handy in case when the arguments can be figured out only after the
	 * registry's construction, e.g. weak pointer to the registry's owner.
	 * @param args Arguments passed to facets' constructors
	 */
	void
	set_construction_args(Args const& ... args)
	{
		base_type::set_factory(factory_type(args ... ));
	}

	/** @todo Document the clear function here*/
};

/**
 * Facet registry class for parameterless facets
 */
template < typename BaseFacet >
class facet_registry< BaseFacet > : public detail::facet_registry_base< BaseFacet > {
public:
	typedef detail::facet_registry_base< BaseFacet >			base_type;
	typedef typename base_type::factory_type					factory_type;
	typedef typename base_type::deleter_func_type				deleter_func_type;
public:
	/**
	 * Create facet registry instance with default deleter.
	 */
	facet_registry() : base_type(factory_type()) {}
	/**
	 * Create facet registry instance with a deleter function.
	 * @param del Deleter function. Takes a pointer to facet as an argument.
	 */
	facet_registry(deleter_func_type del) : base_type(del, factory_type()) {}
	/** @todo Document the clear function here*/
};

/**
 * Add a facet instance to a facet_registry.
 * Will help if a facet cannot be created with the help of a constructor passing
 * the arguments from the registry.
 * @param reg facet registry
 * @param new_facet new facet to add
 * @tparam Facet the type of the facet to add
 * @tparam BaseFacet base class of facets for the registry
 * @tparam Args argument types for the facet registry
 * @pre The Facet type derives from BaseFacet
 * @exception std::logic_error in case if a facet already exists
 */
template < typename Facet, typename BaseFacet, typename ... Args >
void
add_facet(facet_registry< BaseFacet, Args ... >& reg, Facet* new_facet)
{
	reg.template add_facet<Facet>(new_facet);
}

/**
 * Check if facet of type already exists in the registry
 * @param reg facet registry
 * @return true if facet is there
 * @tparam Facet the type of the facet to check
 * @tparam BaseFacet base class of facets for the registry
 * @tparam Args argument types for the facet registry
 * @pre The Facet type derives from BaseFacet
 */
template < typename Facet, typename BaseFacet, typename ... Args >
bool
has_facet(facet_registry< BaseFacet, Args ... > const& reg)
{
	return reg.template has_facet<Facet>();
}

/**
 * Get a reference to a facet of given type, create a new facet instance
 * if facet of this type doesn't exist
 * @param reg facet registry
 * @return Reference to facet's instance
 * @tparam Facet the type of the facet to instantiate and use
 * @tparam BaseFacet base class of facets for the registry
 * @tparam Args argument types for the facet registry
 * @pre The Facet type derives from BaseFacet
 */
template < typename Facet, typename BaseFacet, typename ... Args >
Facet&
use_facet(facet_registry< BaseFacet, Args ... >& reg)
{
	return reg.template use_facet<Facet>();
}

}  // namespace util
}  // namespace tip



#endif /* TIP_UTIL_FACET_HPP_ */
