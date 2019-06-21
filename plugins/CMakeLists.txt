PROJECT(RISA-plugins)

CMAKE_POLICY(SET CMP0025 NEW)
#ADD_DEFINITIONS(-std=c++17)
SET(CMAKE_CXX_STANDARD 11)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)
SET(CMAKE_CXX_EXTENSIONS OFF)
ADD_DEFINITIONS(-std=c++11)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-declarations")

# Need MPICXX
INCLUDE(SetupMPI)
IF(MPI_CXX_FOUND)
    INCLUDE_DIRECTORIES(${MPI_CXX_INCLUDE_PATH})
    LIST(APPEND RISA_EXTERNAL_LIBS ${MPI_CXX_LIBRARIES})
ELSE(MPI_CXX_FOUND)
    MESSAGE("WARNING: Could not find MPI (C++)!")
    MESSAGE("         Either add an MPI compiler to your path (using modules)")
    MESSAGE("         Or force CMake to build using the correct compiler (`export CXX=mpicxx`)")
ENDIF(MPI_CXX_FOUND)

# find flatbuffers
FIND_PACKAGE(Flatbuffers 1.10.0 REQUIRED)

# find boost
ADD_DEFINITIONS(-DBOOST_LOG_DYN_LINK)
SET(Boost_USE_STATIC_LIBS OFF)
FIND_PACKAGE(Boost 1.67.0 REQUIRED COMPONENTS program_options log log_setup filesystem system date_time thread)
INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})
LINK_DIRECTORIES(${Boost_LIBRARY_DIRS})

# find argobots
SET(ENV{PKG_CONFIG_PATH} "$ENV{PKG_CONFIG_PATH}")
FIND_PACKAGE(PkgConfig REQUIRED)
PKG_CHECK_MODULES(ARGOBOTS REQUIRED argobots)

INCLUDE_DIRECTORIES(${Damaris_INCLUDE_DIRS} ${ARGOBOTS_INCLUDE_DIRS})

# VTK
FIND_PACKAGE(VTK 8.2)
INCLUDE(${VTK_USE_FILE})

INCLUDE_DIRECTORIES(../include)
SET(plugin_srcs
    util/garbage-collection.cpp
    util/damaris-util.cpp
    util/SimConfig.cpp
    server/RDServer.cpp
    server/events.cpp
    data/SampleFBBuilder.cpp
    data/ArgobotsManager.cpp
    data/TableBuilder.cpp
    data/FeatureExtractor.cpp
    data/Aggregator.cpp
    data/LPAnalyzer.cpp
    data/MovingAvgData.cpp
    data/analysis-tasks.cpp
    streaming/StreamClient.cpp)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR}/include)
ADD_LIBRARY(RISA-plugins SHARED ${plugin_srcs})
SET_PROPERTY(TARGET RISA-plugins PROPERTY POSITION_INDEPENDENT_CODE ON)
TARGET_LINK_LIBRARIES(RISA-plugins PUBLIC -rdynamic ${Damaris_LIBRARIES} ${VTK_LIBRARIES} ${Boost_LIBRARIES} ${ARGOBOTS_LDFLAGS})

INSTALL(TARGETS RISA-plugins EXPORT RISA-plugins ARCHIVE DESTINATION lib LIBRARY DESTINATION lib)
