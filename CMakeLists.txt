PROJECT(RISA_interface)
CMAKE_MINIMUM_REQUIRED(VERSION 3.12)

FIND_PACKAGE(Damaris 1.3.1 REQUIRED)
SET(RISA_INCLUDE ${Damaris_INCLUDE_DIRS} CACHE INTERNAL "")
FIND_PACKAGE(XercesC REQUIRED)

## set RISA sources
INCLUDE_DIRECTORIES(include)
INCLUDE_DIRECTORIES(${Damaris_INCLUDE_DIRS})
SET(risa_srcs
    core/risa-interface.c)

#SET(DAMARIS_LIBS "$ENV{LD_LIBRARY_PATH}")
#FOREACH(LIB ${Damaris_LIBRARIES})
#    SET(DAMARIS_LIBS "${DAMARIS_LIBS} -l${LIB}")
#ENDFOREACH()
SET(DAMARIS_LIBS "${DAMARIS_LIBS} -L -ldamaris ")
SET(DAMARIS_LIBS "${DAMARIS_LIBS} -L${BOOST_ROOT} -lboost_thread -lboost_log -lboost_log_setup
    -lboost_filesystem -lboost_date_time")
SET(DAMARIS_LIBS "${DAMARIS_LIBS} -L${XercesC_ROOT} -lxerces-c ")
SET(DAMARIS_LIBS "${DAMARIS_LIBS} -lstdc++ -lrt -ldl -lpthread -lm")

ADD_SUBDIRECTORY(plugins)

## print RISA Git Hash
SET(CMAKE_MODULE_PATH ${RISA_interface_SOURCE_DIR}/cmake/modules ${CMAKE_MODULE_PATH})
INCLUDE(GetGitRevisionDescription)
#GET_GIT_HEAD_REVISION(GIT_REFSPEC GIT_SHA1)
## from http://ipenguin.ws/2012/11/cmake-automatically-use-git-tags-as.html
git_describe(VERSION --tags --dirty=-d)

#parse the version information into pieces.
string(REGEX REPLACE "^v([0-9]+)\\..*" "\\1" VERSION_MAJOR "${VERSION}")
string(REGEX REPLACE "^v[0-9]+\\.([0-9]+).*" "\\1" VERSION_MINOR "${VERSION}")
string(REGEX REPLACE "^v[0-9]+\\.[0-9]+\\.([0-9]+).*" "\\1" VERSION_PATCH "${VERSION}")
string(REGEX REPLACE "^v[0-9]+\\.[0-9]+\\.[0-9]+(.*)" "\\1" VERSION_SHA1 "${VERSION}")
set(VERSION_SHORT "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules/version.cpp.in
                ${CMAKE_CURRENT_BINARY_DIR}/version.cpp)
set(version_file "${CMAKE_CURRENT_BINARY_DIR}/version.cpp")

#Add the version_file to the project build or it won't compile.
ADD_LIBRARY(RISA SHARED ${risa_srcs} ${version_file})

## link with Damaris and dependencies
TARGET_LINK_LIBRARIES(RISA PUBLIC -rdynamic ${Damaris_LIBRARIES})
TARGET_LINK_LIBRARIES(RISA PUBLIC -rdynamic RISA-plugins)
SET_PROPERTY(TARGET RISA PROPERTY POSITION_INDEPENDENT_CODE ON)

## for CODES pc file
CONFIGURE_FILE(risa.pc.in risa.pc @ONLY)
SET_SOURCE_FILES_PROPERTIES(${CMAKE_CURRENT_SOURCE_DIR}/risa.pc.in PROPERTIES GENERATED FALSE)
SET_SOURCE_FILES_PROPERTIES(${CMAKE_CURRENT_BINARY_DIR}/risa.pc PROPERTIES GENERATED TRUE)

## make install
INSTALL(DIRECTORY ${ROSS_SOURCE_DIR}/damaris/core DESTINATION include/damaris FILES_MATCHING PATTERN "*.h")
INSTALL(TARGETS RISA ARCHIVE DESTINATION lib LIBRARY DESTINATION lib)
INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/risa.pc DESTINATION lib/pkgconfig)

