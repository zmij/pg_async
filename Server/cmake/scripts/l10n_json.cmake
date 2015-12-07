if(NOT XJSONGETTEXT)
    find_file(
        XJSONGETTEXT xjsongettext
        NO_DEFAULT_PATH
        HINTS ${CMAKE_SOURCE_DIR}/scripts
        DOC "Script for extracting l10n messages from JSON files"
    )
endif()

function(localize_json)
    if (XJSONGETTEXT)
        xgettext(PROGRAM ${XJSONGETTEXT} ${ARGN})
        set(L10N_DOMAINS ${L10N_DOMAINS} PARENT_SCOPE)    
    endif()
endfunction()
