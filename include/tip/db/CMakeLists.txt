#	CMakeLists.txt
#	
#	@author zmij
#	@date Aug 3, 2015

cmake_minimum_required(VERSION 2.6)

set(
    pg_async_root_HDRS
    pg.hpp
)

set(
    pg_async_HDRS
    pg/common.hpp
    pg/database.hpp
    pg/datatype_mapping.hpp
    pg/error.hpp
    pg/pg_types.hpp
    pg/protocol_io_traits.hpp
    pg/protocol_io_traits.inl
    pg/query.hpp
    pg/query.inl
    pg/resultset.hpp
    pg/resultset.inl
    pg/sqlstates.hpp
    pg/transaction.hpp
)

set(
    pg_async_detail_HDRS
    pg/detail/protocol_parsers.hpp
)

set(
    pg_async_io_DHRS
    pg/io/bytea.hpp
)

install(
    FILES ${pg_async_root_HDRS}
    DESTINATION include/tip/db
)

install(
    FILES ${pg_async_HDRS}
    DESTINATION include/tip/db/pg
)

install(
    FILES ${pg_async_detail_HDRS}
    DESTINATION include/tip/db/pg/detail
)

install(
    FILES ${pg_async_io_HDRS}
    DESTINATION include/tip/db/pg/io
)
