PROJECT(ROSS_Damaris)

SET(DAMARIS_DIR $ENV{DAMARIS_DIR} CACHE STRING "Path to Damaris install")
OPTION(USE_VISIT "Use VisIt (requires Damaris)" OFF)

## set some internal cache vars that can be used in ROSS and model builds
SET(DAMARIS_INCLUDE ${DAMARIS_DIR}/include CACHE INTERNAL "")
SET(DAMARIS_LIBRARY ${DAMARIS_DIR}/lib CACHE INTERNAL "")

SET(LINKER_STR "-rdynamic -L${DAMARIS_LIBRARY} -Wl,--whole-archive,-ldamaris,--no-whole-archive -lxerces-c -lboost_log -lboost_log_setup -lboost_filesystem -lboost_system -lboost_date_time -lboost_thread -lstdc++ -lmpi_cxx -lrt -ldl -lpthread")
IF (USE_VISIT)
    SET(VISIT_DIR $ENV{VISIT_DIR} CACHE STRING "Path to VisIt lib dir")
    SET(LINKER_STR "${LINKER_STR} -L${VISIT_DIR} -lsimV2")
ENDIF(USE_VISIT)
SET(DAMARIS_LINKER_FLAGS ${LINKER_STR} CACHE INTERNAL "")

## set ross-damaris sources
INCLUDE_DIRECTORIES(${DAMARIS_INCLUDE})
SET(rd_srcs core/damaris.h core/damaris.c ${DAMARIS_DIR}/include/Damaris.h)

## print ROSS-Damaris Git Hash
LIST(APPEND CMAKE_MODULE_PATH "${ROSS_SOURCE_DIR}/cmake")
INCLUDE(GetGitRevisionDescription)
GET_GIT_HEAD_REVISION(GIT_REFSPEC GIT_SHA1_RD)

## build config files
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/ross-damaris-config.h.in ${CMAKE_BINARY_DIR}/ross-damaris-config.h @ONLY)
SET(rd_srcs ${rd_srcs} ross-damaris-config.h)
SET_SOURCE_FILES_PROPERTIES(${CMAKE_CURRENT_SOURCE_DIR}/ross-damaris-config.h.in PROPERTIES GENERATED FALSE)
SET_SOURCE_FILES_PROPERTIES(${CMAKE_CURRENT_SOURCE_DIR}/ross-damaris-config.h PROPERTIES GENERATED TRUE)

ADD_LIBRARY(ROSS_Damaris ${rd_srcs})

## link with ROSS library 
INCLUDE_DIRECTORIES(${ROSS_SOURCE_DIR} ${ROSS_BINARY_DIR})
TARGET_LINK_LIBRARIES(ROSS_Damaris ROSS)

# for CODES pc file
#CONFIGURE_FILE(ross-config.in ross-config @ONLY)
CONFIGURE_FILE(ross-damaris.pc.in ross-damaris.pc @ONLY)
SET_SOURCE_FILES_PROPERTIES(${CMAKE_CURRENT_SOURCE_DIR}/ross-damaris.pc.in PROPERTIES GENERATED FALSE)
SET_SOURCE_FILES_PROPERTIES(${CMAKE_CURRENT_BINARY_DIR}/ross-damaris.pc PROPERTIES GENERATED TRUE)

# make install
INSTALL(FILES ${CMAKE_BINARY_DIR}/ross-damaris-config.h DESTINATION include)
INSTALL(DIRECTORY ${ROSS_SOURCE_DIR}/damaris/core DESTINATION include/damaris FILES_MATCHING PATTERN "*.h")
INSTALL(TARGETS ROSS_Damaris EXPORT ROSS_Damaris ARCHIVE DESTINATION lib LIBRARY DESTINATION lib)
INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/ross-damaris.pc DESTINATION lib/pkgconfig)

