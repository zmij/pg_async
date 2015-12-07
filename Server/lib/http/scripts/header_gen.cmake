set(GEN_HEADER_ENUM ${CMAKE_CURRENT_SOURCE_DIR}/scripts/headers-enum.pl)
set(GEN_HEADER_PARSER ${CMAKE_CURRENT_SOURCE_DIR}/scripts/headers-parse.pl)
set(GEN_HEADER_GENERATOR ${CMAKE_CURRENT_SOURCE_DIR}/scripts/headers-generate.pl)

function(filter_http_headers OUTFILE)
    add_custom_command(
        OUTPUT ${OUTFILE}
        DEPENDS ${ARGN}
        COMMENT "Filter HTTP headers definitions"
        COMMAND cat ${ARGN} | awk -F',' '$$3~/http/{print $$0}' | sort > ${OUTFILE}
    )
    add_custom_target(
        filtered_headers
        DEPENDS ${OUTFILE}
    )
endfunction()

function(generate_http_headers_enum INFILE OUTFILE)
    add_custom_command(
        OUTPUT ${OUTFILE}
        DEPENDS ${INFILE} ${GEN_HEADER_ENUM}
        COMMENT "Generate HTTP headers enumeration header"
        COMMAND cat ${INFILE} | ${GEN_HEADER_ENUM} > ${OUTFILE}
    )
    add_custom_target(
        http_enum_header
        DEPENDS filtered_headers ${OUTFILE}
    )
endfunction()

function(generate_http_header_parser_generator INFILE)
    add_custom_command(
        OUTPUT header_names_parser.cpp
        DEPENDS filtered_headers ${GEN_HEADER_PARSER}
        COMMENT "Generate HTTP header names parser"
        COMMAND cat ${INFILE} | ${GEN_HEADER_PARSER} > header_names_parser.cpp
    )
    
    add_custom_command(
        OUTPUT header_names_generator.cpp
        DEPENDS filtered_headers ${GEN_HEADER_GENERATOR}
        COMMENT "Generate HTTP header names generator"
        COMMAND cat ${INFILE} | ${GEN_HEADER_GENERATOR} > header_names_generator.cpp
    )
endfunction()
