if (DEFINED ENV{GITVER})
    string(STRIP $ENV{GITVER} GITVER)
    set( GIT_VERSION_STR_POSTFIX ${GITVER} )
elseif ( NOT DEFINED GITVER )
    execute_process( COMMAND bash -c "git describe --long --tags --dirty --always" 
        OUTPUT_VARIABLE GITVER )
    string(STRIP "${GITVER}" GITVER)
    set( GIT_VERSION_STR_POSTFIX ${GITVER} ) 
endif()

