PROJECT(rdplugins)

INCLUDE_DIRECTORIES(${DAMARIS_INCLUDE} ${VISIT_DIR}/sim/V2/lib ${FLATBUF_INCLUDE})
ADD_DEFINITIONS(-std=gnu++11)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-declarations")
SET(plugin_srcs
    garbage_collection.cpp
    streaming.cpp
    streaming/stream-client.cpp)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})
ADD_LIBRARY(rdplugins SHARED ${plugin_srcs})
SET_PROPERTY(TARGET rdplugins PROPERTY POSITION_INDEPENDENT_CODE ON)

INSTALL(TARGETS rdplugins EXPORT rdplugins ARCHIVE DESTINATION lib LIBRARY DESTINATION lib)
