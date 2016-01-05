#
# Try to find ASSIMP library and include path.
# Once done this will define
#
# ASSIMP_FOUND
# ASSIMP_INCLUDE_PATH
# ASSIMP_LIBRARY 

SET(ASSIMP_SEARCH_PATHS
	$ENV{ASSIMP_ROOT}
	${DEPENDENCIES_ROOT}
	/usr 
	/usr/local
	/opt/local)

IF (WIN32)
FIND_PATH(ASSIMP_INCLUDE_PATH
    NAMES
        assimp/ai_assert.h
    PATHS
        ${ASSIMP_SEARCH_PATHS}
    PATH_SUFFIXES
        include
    DOC
        "The directory where assimp/ai_assert.h resides"
)

FIND_LIBRARY(ASSIMP_LIBRARY
    NAMES
       assimp-vc110-mtd.lib
    PATHS
        ${ASSIMP_SEARCH_PATHS}
    PATH_SUFFIXES
        lib
    DOC
        "The directory where assimp-vc110-mtd.lib resides"
)

ELSE()
	FIND_PATH(ASSIMP_INCLUDE_PATH assimp/ai_assert.h)
	FIND_LIBRARY(ASSIMP_LIBRARY
    	NAMES 
            ASSIMP ASSIMP32 ASSIMP ASSIMP32s 
        PATH_SUFFIXES 
            lib64
    )
ENDIF()
    
SET(ASSIMP_FOUND "NO")
IF (ASSIMP_INCLUDE_PATH AND ASSIMP_LIBRARY)
	SET(ASSIMP_LIBRARIES ${ASSIMP_LIBRARY})
	SET(ASSIMP_FOUND "YES")
    message("EXTERNAL LIBRARY 'ASSIMP' FOUND")
ELSE()
    message("ERROR: EXTERNAL LIBRARY 'ASSIMP' NOT FOUND")
ENDIF (ASSIMP_INCLUDE_PATH AND ASSIMP_LIBRARY)
