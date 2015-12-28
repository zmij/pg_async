
build_db_options(AUTHN)
build_db_options(LOBBY)

if(DB_AUTHN_DSN)
    set(_TEST_DATABASE "main=${DB_AUTHN_DSN}")
elseif(DB_LOBBY_DSN)
    set(_TEST_DATABASE "main=${DB_LOBBY_DSN}")
endif()

if(_TEST_DATABASE)
    message(STATUS "Will use ${_TEST_DATABASE} as test database")
    set(TEST_DATABASE ${_TEST_DATABASE})
else()
    message(WARNING "No test database is set")
endif()

