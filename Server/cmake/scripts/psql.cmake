find_program(PSQL NAMES psql)

function(build_db_options ALIAS)
    if(DB_${ALIAS}_BUILD)
        set(mandatory YES)
    endif()
    if (mandatory)
        message(STATUS "${ALIAS} database is mandatory")
        if (NOT PSQL)
            message(FATAL_ERROR "psql program not found")
        endif()
    endif()
    if (DB_${ALIAS}_NAME)
        if (NOT DB_${ALIAS}_HOST)
            set(DB_${ALIAS}_HOST localhost)
        endif()
        if (NOT DB_${ALIAS}_PORT)
            set(DB_${ALIAS}_PORT 5432)
        endif()
        if (DB_${ALIAS}_USER)
            if (DB_${ALIAS}_PASS)
                set(_DB_${ALIAS}_ENV PGPASSWORD=DB_${ALIAS}_PASS PARENT_SCOPE)
                set(
                    _DB_${ALIAS}_DSN 
                    "tcp://${DB_${ALIAS}_USER}:${DB_${ALIAS}_PASS}@${DB_${ALIAS}_HOST}:${DB_${ALIAS}_PORT}[${DB_${ALIAS}_NAME}]"
                )
            else()    
                set(
                    _DB_${ALIAS}_DSN 
                    "tcp://${DB_${ALIAS}_USER}@${DB_${ALIAS}_HOST}:${DB_${ALIAS}_PORT}[${DB_${ALIAS}_NAME}]"
                )
            endif()
            set(
                _DB_${ALIAS}_OPTIONS
                -h ${DB_${ALIAS}_HOST}
                -p ${DB_${ALIAS}_PORT}
                -U ${DB_${ALIAS}_USER}
                ${DB_${ALIAS}_NAME}
            ) 
            
            message(STATUS "${ALIAS} DSN ${_DB_${ALIAS}_DSN}")
            message(STATUS "${ALIAS} options ${_DB_${ALIAS}_OPTIONS}")
            
            set(DB_${ALIAS}_OPTIONS ${_DB_${ALIAS}_OPTIONS} PARENT_SCOPE)
            set(DB_${ALIAS}_DSN ${_DB_${ALIAS}_DSN} PARENT_SCOPE)
        elseif(mandatory)
            message(FATAL_ERROR "${ALIAS} database user is not set (-DDB_${ALIAS}_USER=)")
        else()
            message(WARNING "${ALIAS} database user is not set (-DDB_${ALIAS}_USER=)")
        endif()
    elseif(mandatory)
        message(FATAL_ERROR "${ALIAS} database name is not set (-DDB_${ALIAS}_NAME=)")
    else()
        message(WARNING "${ALIAS} database name is not set (-DDB_${ALIAS}_NAME=)")
    endif()
endfunction()

function(build_database ALIAS)
    if (DB_${ALIAS}_BUILD)
        if (NOT PSQL)
            message(FATAL_ERROR "psql program not found")
        endif()
        if (NOT DB_${ALIAS}_OPTIONS)
            message(FATAL_ERROR "Database ${ALIAS} is not configured")
        endif()
        if (NOT ARGN)
            message(FATAL_ERROR "Database script files for ${ALIAS} not specified")
        endif()
        set(__sql_file_depends)
        set(__dbname ${DB_${ALIAS}_NAME})
        set(__out_files)
        foreach(sql_file ${ARGN})
            message(STATUS "DB ${ALIAS} script ${sql_file}")
            set(input ${CMAKE_CURRENT_SOURCE_DIR}/${sql_file})
            set(output_name ${__dbname}.${sql_file})
            string(REPLACE / _ output_name ${output_name})
            set(output ${CMAKE_CURRENT_BINARY_DIR}/${output_name})
            list(APPEND __sql_file_depends ${input})
            list(APPEND __out_files ${output})
            add_custom_command(
                OUTPUT ${output}
                DEPENDS ${__sql_file_depends}
                COMMENT "DB ${ALIAS}: Execute SQL script file ${sql_file} in database ${__dbname}"
                COMMAND ${_DB_${ALIAS}_} ${PSQL} ${DB_${ALIAS}_OPTIONS} -f ${input}
                COMMAND cp ${input} ${output}
            )
            list(APPEND __sql_file_depends ${output})
        endforeach()
        add_custom_target(
            "${ALIAS}_DB"
            DEPENDS ${__out_files}
            COMMENT "Create database ${ALIAS} (${__dbname})"
        )
    endif()
endfunction()
