#	/awm-server/sql/create/CMakeLists.txt
#	
#	@author zmij
#	@date Dec 9, 2015

cmake_minimum_required(VERSION 2.6)

set(AUTHN_DB_SQL
    core.sql
    authn/authentication.sql
)

set(LOBBY_DB_SQL
    core.sql
    lobby/sessions.sql
)

build_database(AUTHN ${AUTHN_DB_SQL})
build_database(LOBBY ${LOBBY_DB_SQL})
