
IF (ZYPP_PREFIX)
  MESSAGE(STATUS "ZYpp library prefix set to ${ZYPP_PREFIX}")
ELSE (ZYPP_PREFIX)
  MESSAGE(STATUS "ZYpp path not set. Looking for it.")
ENDIF (ZYPP_PREFIX)

if(ZYPP_INCLUDE_DIR AND ZYPP_LIBRARY)
	# Already in cache, be silent
	set(ZYPP_FIND_QUIETLY TRUE)	
endif(ZYPP_INCLUDE_DIR AND ZYPP_LIBRARY)

set(ZYPP_LIBRARY)
set(ZYPP_INCLUDE_DIR)

FIND_PATH(ZYPP_INCLUDE_DIR zypp/ZYpp.h
	/usr/include
	/usr/local/include
	${ZYPP_PREFIX}/include
)

FIND_LIBRARY(ZYPP_LIBRARY NAMES zypp
	PATHS
	/usr/lib
	/usr/local/lib
	${ZYPP_PREFIX}/lib
)

if(ZYPP_INCLUDE_DIR AND ZYPP_LIBRARY)
   MESSAGE( STATUS "ZYpp found: includes in ${ZYPP_INCLUDE_DIR}, library in ${ZYPP_LIBRARY}")
   set(ZYPP_FOUND TRUE)
else(ZYPP_INCLUDE_DIR AND ZYPP_LIBRARY)
   MESSAGE( FATAL "ZYpp not found")
endif(ZYPP_INCLUDE_DIR AND ZYPP_LIBRARY)

MARK_AS_ADVANCED(ZYPP_INCLUDE_DIR ZYPP_LIBRARY)