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

set(TEST_SOURCES
	test/mocks/Pipe.cpp
	test/mocks/XenCtrlMock.cpp
	test/mocks/XenEvtchnMock.cpp
	test/mocks/XenGnttabMock.cpp
	test/mocks/XenStoreMock.cpp
	test/test.cpp
	test/testFrontendHandler.cpp
	test/testRingBuffer.cpp
	test/testXenEvtchn.cpp
	test/testXenGnttab.cpp
	test/testXenStat.cpp
	test/testXenStore.cpp
)

include_directories(
	include/xen/be
	${XEN_INCLUDE_PATH}
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11 -Wall")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")

add_library(xenbe SHARED ${SOURCES})

add_executable(tests ${TEST_SOURCES})

target_link_libraries(tests xenbe pthread)

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
