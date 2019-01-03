PROJECT(ROSS_Damaris)

SET(DAMARIS_DIR $ENV{DAMARIS_DIR} CACHE STRING "Path to Damaris install")
OPTION(USE_VISIT "Use VisIt (requires Damaris)" OFF)

SET(FLATBUF_DIR $ENV{FLATBUF_DIR} CACHE STRING "Path to Flatbuffers install")

## set some internal cache vars that can be used in ROSS and model builds
SET(DAMARIS_INCLUDE ${DAMARIS_DIR}/include CACHE INTERNAL "")
SET(DAMARIS_LIBRARY ${DAMARIS_DIR}/lib CACHE INTERNAL "")
SET(FLATBUF_INCLUDE ${FLATBUF_DIR}/include CACHE INTERNAL "")

# not exactly sure why this is needed, but quiets a boost warning
CMAKE_POLICY(SET CMP0054 NEW)
# find boost
SET(Boost_USE_STATIC_LIBS ON)
FIND_PACKAGE(Boost 1.67.0 COMPONENTS program_options log log_setup filesystem system date_time thread)
IF (NOT Boost_FOUND)
    MESSAGE("Boost (at least version 1.62) not found! Checking in Damaris Install directory")
    SET(BOOST_ROOT ${DAMARIS_DIR})
    #SET(Boost_NO_SYSTEM_PATHS ON)
    #SET(BOOST_LIBRARYDIR ${BOOST_ROOT}/lib)
    FIND_PACKAGE(Boost 1.67.0 COMPONENTS program_options log log_setup filesystem system date_time thread)
ENDIF(NOT Boost_FOUND)

IF (Boost_FOUND)
    MESSAGE("found boost v${Boost_VERSION} \nInclude: ${Boost_INCLUDE_DIRS}\nLibrary dir: ${Boost_LIBRARY_DIRS} \nlibraries: ${Boost_LIBRARIES}")
    INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})
    ADD_DEFINITIONS(-DBOOST_LOG_DYN_LINK)
ENDIF(Boost_FOUND)

SET(LINKER_STR "-rdynamic -L${DAMARIS_LIBRARY} -ldamaris -lxerces-c ${Boost_LIBRARY_DIRS}/libboost_log.a ${Boost_LIBRARY_DIRS}/libboost_log_setup.a -lboost_filesystem -lboost_system -lboost_date_time -lboost_thread -lstdc++ -lmpi_cxx -lrt -ldl -lpthread")
IF (USE_VISIT)
    SET(VISIT_DIR $ENV{VISIT_DIR} CACHE STRING "Path to VisIt src dir")
    SET(LINKER_STR "${LINKER_STR} -L${VISIT_DIR}/lib -lsimV2")
    INCLUDE_DIRECTORIES(${VISIT_DIR}/sim/V2/lib)
ENDIF(USE_VISIT)
SET(DAMARIS_LINKER_FLAGS ${LINKER_STR} CACHE INTERNAL "")

## set ross-damaris sources
INCLUDE_DIRECTORIES(${DAMARIS_INCLUDE} ${FLATBUF_INCLUDE})
INCLUDE_DIRECTORIES(include)
SET(rd_srcs
    include/core/damaris.h
    include/plugins/util/config-c.h
    core/damaris.c
    ${DAMARIS_DIR}/include/Damaris.h)

ADD_SUBDIRECTORY(plugins)

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

## link with Damaris and dependencies
TARGET_LINK_LIBRARIES(ROSS_Damaris ${DAMARIS_LINKER_FLAGS})
TARGET_LINK_LIBRARIES(ROSS_Damaris rdplugins)

# for CODES pc file
#CONFIGURE_FILE(ross-config.in ross-config @ONLY)
CONFIGURE_FILE(ross-damaris.pc.in ross-damaris.pc @ONLY)
SET_SOURCE_FILES_PROPERTIES(${CMAKE_CURRENT_SOURCE_DIR}/ross-damaris.pc.in PROPERTIES GENERATED FALSE)
SET_SOURCE_FILES_PROPERTIES(${CMAKE_CURRENT_BINARY_DIR}/ross-damaris.pc PROPERTIES GENERATED TRUE)

# make install
INSTALL(FILES ${CMAKE_BINARY_DIR}/ross-damaris-config.h DESTINATION include)
INSTALL(DIRECTORY ${ROSS_SOURCE_DIR}/damaris/core DESTINATION include/damaris FILES_MATCHING PATTERN "*.h")
INSTALL(TARGETS ROSS_Damaris ARCHIVE DESTINATION lib LIBRARY DESTINATION lib)
INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/ross-damaris.pc DESTINATION lib/pkgconfig)

