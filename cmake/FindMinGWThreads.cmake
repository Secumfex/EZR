#
# Try to find GLEW library and include path.
# Once done this will define
#
# MINGW_THREADS_FOUND
# MINGW_THREADS_INCLUDE_PATH
# 

SET(MINGW_THREADS_SEARCH_PATHS
	$ENV{MINGW_THREADS_ROOT}
	${DEPENDENCIES_ROOT}
	/usr/local
	/usr)

FIND_PATH(MINGW_THREADS_INCLUDE_PATH
    NAMES
        mingw-std-threads/mingw.thread.h
    PATHS
        ${MINGW_THREADS_SEARCH_PATHS}
    PATH_SUFFIXES
        include
    DOC
        "The directory where mingw.thread.h resides"
)

SET(MINGW_THREADS_FOUND "NO")
IF (MINGW_THREADS_INCLUDE_PATH)
	SET(MINGW_THREADS_FOUND "YES")
    add_definitions(-DMINGW_THREADS)
    message("EXTERNAL LIBRARY 'MINGW_THREADS' FOUND")
ELSE()
    message("ERROR: EXTERNAL LIBRARY 'MINGW_THREADS' NOT FOUND")
ENDIF (MINGW_THREADS_INCLUDE_PATH)
