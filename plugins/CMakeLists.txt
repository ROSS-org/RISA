PROJECT(rdplugins)

INCLUDE_DIRECTORIES(${DAMARIS_INCLUDE} ${VISIT_DIR}/sim/V2/lib ${FLATBUF_INCLUDE})
ADD_DEFINITIONS(-std=gnu++11)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-declarations -Wall -Wextra -Wpedantic")
INCLUDE_DIRECTORIES(include)
SET(plugin_srcs
    util/garbage-collection.cpp
    util/damaris-util.cpp
    util/fb-util.cpp
    util/config.cpp
    streaming/streaming.cpp
    streaming/stream-client.cpp)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR}/include)
ADD_LIBRARY(rdplugins SHARED ${plugin_srcs})
SET_PROPERTY(TARGET rdplugins PROPERTY POSITION_INDEPENDENT_CODE ON)

INSTALL(TARGETS rdplugins EXPORT rdplugins ARCHIVE DESTINATION lib LIBRARY DESTINATION lib)
