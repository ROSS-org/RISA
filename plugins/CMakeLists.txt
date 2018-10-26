PROJECT(rdplugins)

# find boost
SET(Boost_USE_STATIC_LIBS OFF)
FIND_PACKAGE(Boost 1.62.0 COMPONENTS program_options log log_setup filesystem system date_time thread)
IF (NOT Boost_FOUND)
    MESSAGE("rdplugins: Boost (at least version 1.62) not found! Checking in Damaris Install directory")
    SET(BOOST_ROOT ${DAMARIS_DIR})
    #SET(Boost_NO_SYSTEM_PATHS ON)
    #SET(BOOST_LIBRARYDIR ${BOOST_ROOT}/lib)
    FIND_PACKAGE(Boost 1.62.0 COMPONENTS program_options log log_setup filesystem system date_time thread)
ENDIF(NOT Boost_FOUND)

IF (Boost_FOUND)
    MESSAGE("rdplugins: found boost v${Boost_VERSION} \nInclude: ${Boost_INCLUDE_DIRS}\nLibrary dir: ${Boost_LIBRARY_DIRS} \nlibraries: ${Boost_LIBRARIES}")
    INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})
    ADD_DEFINITIONS(-DBOOST_LOG_DYN_LINK)
ENDIF(Boost_FOUND)

SET(ENV{PKG_CONFIG_PATH} "$ENV{PKG_CONFIG_PATH}:/home/rossc3/argobots-install/lib/pkgconfig")
MESSAGE("pkg config path: $ENV{PKG_CONFIG_PATH}")
FIND_PACKAGE(PkgConfig REQUIRED)
PKG_CHECK_MODULES(ARGOBOTS REQUIRED argobots)

INCLUDE_DIRECTORIES(${DAMARIS_INCLUDE} ${FLATBUF_INCLUDE} ${ARGOBOTS_INCLUDE_DIRS})
ADD_DEFINITIONS(-std=c++11)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-declarations")
INCLUDE_DIRECTORIES(../include)
SET(plugin_srcs
    util/garbage-collection.cpp
    util/damaris-util.cpp
    util/SimConfig.cpp
    util/FlatBufferHelper.cpp
    util/model.cpp
    server/RDServer.cpp
    server/events.cpp
    data/DataProcessor.cpp
    data/ArgobotsManager.cpp
    data/DataSample.cpp
    data/DataManager.cpp
    streaming/StreamClient.cpp)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR}/include)
ADD_LIBRARY(rdplugins SHARED ${plugin_srcs})
SET_PROPERTY(TARGET rdplugins PROPERTY POSITION_INDEPENDENT_CODE ON)
TARGET_LINK_LIBRARIES(rdplugins ${Boost_LIBRARIES} ${ARGOBOTS_LDFLAGS})

INSTALL(TARGETS rdplugins EXPORT rdplugins ARCHIVE DESTINATION lib LIBRARY DESTINATION lib)
