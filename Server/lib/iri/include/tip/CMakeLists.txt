#	CMakeLists.txt for tip-iri includes
#	
#	@author zmij
#	@date Nov 30, 2015

cmake_minimum_required(VERSION 2.6)

set(iri_HDRS iri.hpp)
set(iri_grammar_HDRS iri/grammar/iri_parse.hpp iri/grammar/iri_generate.hpp)

install(
    FILES ${iri_HDRS}
    DESTINATION include/tip
)

install(
    FILES ${iri_grammar_HDRS}
    DESTINATION include/tip/iri/grammar
)
