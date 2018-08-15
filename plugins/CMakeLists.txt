PROJECT(rdplugins)

INCLUDE_DIRECTORIES(${DAMARIS_INCLUDE} ${VISIT_DIR}/sim/V2/lib)
SET(plugin_srcs 
    garbage_collection.cpp
    opt-debug.C
    rng-check.cpp
    util.cpp
    Event.h
    Event.cpp
    EventIndex.h)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})
ADD_LIBRARY(rdplugins SHARED ${plugin_srcs})
SET_PROPERTY(TARGET rdplugins PROPERTY POSITION_INDEPENDENT_CODE ON)

INSTALL(TARGETS rdplugins EXPORT rdplugins ARCHIVE DESTINATION lib LIBRARY DESTINATION lib)
