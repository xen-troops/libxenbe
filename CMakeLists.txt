cmake_minimum_required(VERSION 3.0)

project(libxenbe)

set(SOURCES
	src/BackendBase.cpp
	src/FrontendHandlerBase.cpp
	src/RingBufferBase.cpp
	src/Log.cpp
	src/Utils.cpp
	src/XenCtrl.cpp
	src/XenEvtchn.cpp
	src/XenGnttab.cpp
	src/XenStat.cpp
	src/XenStore.cpp
)

include_directories(
	include/xen/be
	${XEN_INCLUDE_PATH}
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11 -Wall")

add_library(xenbe SHARED ${SOURCES})

install(TARGETS xenbe LIBRARY DESTINATION lib)

install(
	DIRECTORY include/
	DESTINATION include
	FILES_MATCHING PATTERN "*.hpp"
)

find_package(Doxygen)
if(DOXYGEN_FOUND)
	configure_file(${CMAKE_CURRENT_SOURCE_DIR}/doxygen.cfg
				   ${CMAKE_CURRENT_BINARY_DIR}/doxygen.cfg @ONLY)
				   
	add_custom_target(
		doc ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/doxygen.cfg
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
		COMMENT "Generating API documentation with Doxygen" VERBATIM
	)
endif(DOXYGEN_FOUND)