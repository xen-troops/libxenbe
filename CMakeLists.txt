cmake_minimum_required(VERSION 3.0)

include_directories(
	include
	include/xen/be
	test
	${XEN_INCLUDE_PATH}
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11 -Wall")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")

link_directories(${XEN_LIB_PATH})

add_subdirectory(src)
add_subdirectory(example)

if (NOT SKIP_TESTS)
	enable_testing()
	add_subdirectory(test)
	if (CMAKE_BUILD_TYPE STREQUAL "Coverage")
		set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/CMakeModules)
		include(CodeCoverage)
		setup_target_for_coverage(coverage unitTests coverage)
	endif()
endif()

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
