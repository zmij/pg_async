# tip-facet

Facet utility for adding arbitrary opaque data to an object

### Concepts

A facet is some opaque data bound to an object. The object doesn't know anything 
about the facet's nature, the only thing the object cares about is deleting the 
facet at it's own destruction.

An object can have only one instance of a facet of a given type.

All facets bound to an object should have a common base, the type of common base 
is specified when instantiating the `facet_registry` template. The rest of 
template parameters are types for facet's constructor arguments. The facet registry 
instance creates instances of facets with parameters passed to it. All facets 
will be created using the same set of arguments.

### Usage

```cpp
 
class A {
public:
	class facet; // Common base for class A's facets.
private:
	typedef tip::util::facet_registry< facet, int, std::string > facet_registry_type;
	facet_registry_type facet_registry_;
	
	template < typename Facet >
	friend bool
	has_facet(A*);
	
	template < typename Facet >
	friend Facet&
	use_facet(A*);
};

class A::facet : private boost::noncopyable {
public:
	facet(int, std::string const&) {}
	virtual ~facet() {}
	
	int
	get_int();
	std::string const&
	get_string();
};

template < typename Facet >
Facet&
use_facet(A* a)
{
	return use_facet< Facet >(a->facet_registry_);
}

template < typename Facet >
bool
has_facet(A* a)
{
	return has_facet< Facet >(a->facet_registry_);
}

class data_a : public A::facet {
	data_a(int i, std::string const& s) : facet(i, s) {}
};
class data_b : public A::facet {
	data_b(int i, std::string const& s) : facet(i, s) {}
};

int
main(int argc, char* argv[])
{
	A a;
	
	if (has_facet<data_a>(&a)) {
		// No facet there, it can be instantiated with use_facet or add_facet
	}
	
	data_a& da = use_facet<data_a>(&a); // Will instantiate data_a class and add it to a's registry
	data_b& db = use_facet<data_b>(&a); // Will instantiate data_b class and add it to a's registry
	
	// After the a goes out of scope, all facets instances will be destroyed.
}

```