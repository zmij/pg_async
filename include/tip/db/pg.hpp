/**
 * pg.hpp
 *
 *  @date: Jul 16, 2015
 *  @author: zmij
 *
 * @page Datatype conversions
 *	## PostrgreSQL to C++ datatype conversions
 *
 *	### Numeric types
 *	[PostgreSQL documentation](http://www.postgresql.org/docs/9.4/static/datatype-numeric.html)
 *	PostgreSQL			| C++
 *	------------------- | -------------------
 *	smallint			| int16_t
 *	integer				| int32_t
 *	bigint				| int64_t
 *	decimal				| no mapping
 *	numeric				| no mapping
 *  real				| float
 *  double precision	| double
 *  smallserial			| int16_t
 *  serial				| int32_t
 *  bigserial			| int64_t
 *
 *  ### Character types
 *  [PostgreSQL documentation](http://www.postgresql.org/docs/9.4/static/datatype-character.html)
 *	PostgreSQL			| C++
 *	------------------- | -------------------
 *  varchar(n)			| std::string
 *  character(n)		| std::string
 *  text				| std::string
 *
 *  ### Monetary type
 *  [PostgreSQL documentation](http://www.postgresql.org/docs/9.4/static/datatype-money.html)
 *	PostgreSQL			| C++
 *	------------------- | -------------------
 *	money				| no mapping
 *
 *  ### Binary type
 *  [PostgreSQL documentation](http://www.postgresql.org/docs/9.4/static/datatype-binary.html)
 *	PostgreSQL			| C++
 *	------------------- | -------------------
 *	bytea				| std::vector<char>
 *
 *  ### Datetime typs
 *  [PostgreSQL documentation](http://www.postgresql.org/docs/9.4/static/datatype-datetime.html)
 *  [PostgreSQL date/time support](http://www.postgresql.org/docs/9.4/static/datetime-appendix.html)
 *	PostgreSQL			| C++
 *	------------------- | -------------------
 *  timestamp			|
 *  timestamptz			|
 *  date				|
 *  time				|
 *  time with tz		|
 *  interval			|
 *
 *
 *	### Boolean type
 *  [PostgreSQL documentation](http://www.postgresql.org/docs/9.4/static/datatype-boolean.html)
 *	PostgreSQL			| C++
 *	------------------- | -------------------
 *	boolean				| bool
 *
 *  ### Enumerated types
 *  [PostgreSQL documentation](http://www.postgresql.org/docs/9.4/static/datatype-enum.html)
 *
 *  ### Geometric types
 *  [PostgreSQL documentation](http://www.postgresql.org/docs/9.4/static/datatype-geometric.html)
 *
 *  ### Network address types
 *  [PostgreSQL documentation](http://www.postgresql.org/docs/9.4/static/datatype-net-types.html)
 *	PostgreSQL			| C++
 *	------------------- | -------------------
 *	cidr				|
 *	inet				|
 *	macaddr				|
 *
 *  ### Bit String types
 *  [PostgreSQL documentation](http://www.postgresql.org/docs/9.4/static/datatype-bit.html)
 *	PostgreSQL			| C++
 *	------------------- | -------------------
 *	bit(n)				|
 *	bit varying(n)		|
 *
 *	### Text search types
 *	[PostgreSQL documentation](http://www.postgresql.org/docs/9.4/static/datatype-textsearch.html)
 *
 *	### UUID type
 *	[PostgreSQL documentation](http://www.postgresql.org/docs/9.4/static/datatype-uuid.html)
 *	PostgreSQL			| C++
 *	------------------- | -------------------
 *	uuid				| boost::uuid
 *
 *	### XML type
 *	[PostgreSQL documentation](http://www.postgresql.org/docs/9.4/static/datatype-xml.html)
 *
 *	### JSON types
 *	[PostgreSQL documentation](http://www.postgresql.org/docs/9.4/static/datatype-json.html)
 *
 *	### Arrays
 *	[PostgreSQL documentation](http://www.postgresql.org/docs/9.4/static/arrays.html)
 *	PostgreSQL			| C++
 *	------------------- | -------------------
 *	type array(n)		| std::vector< type mapping >
 *
 *	### Composite types
 *	[PostgreSQL documentation](http://www.postgresql.org/docs/9.4/static/rowtypes.html)
 *
 *	### Range types
 *	[PostgreSQL documentation](http://www.postgresql.org/docs/9.4/static/rangetypes.html)
 *	PostgreSQL			| C++
 *	------------------- | -------------------
 *	int4range			|
 *	int8range			|
 *	numrange			|
 *	tsrange				|
 *	tstzrange			|
 *	daterange			|
 */

#ifndef TIP_DB_PG_HPP_
#define TIP_DB_PG_HPP_

#include <tip/db/pg/database.hpp>
#include <tip/db/pg/query.hpp>
#include <tip/db/pg/resultset.hpp>
#include <tip/db/pg/error.hpp>

#endif /* TIP_DB_PG_HPP_ */
