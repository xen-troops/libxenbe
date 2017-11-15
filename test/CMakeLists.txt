project(unitTests)

################################################################################
# Includes
################################################################################

include_directories( . )

################################################################################
# Sources
################################################################################

set(MOCK_SOURCES
	mocks/Pipe.cpp
	mocks/XenCtrlMock.cpp
	mocks/XenEvtchnMock.cpp
	mocks/XenGnttabMock.cpp
	mocks/XenStoreMock.cpp
)

set(TEST_SOURCES
	testBackend.cpp
	testFrontendHandler.cpp
	testRingBuffer.cpp
	testXenEvtchn.cpp
	testXenGnttab.cpp
	testXenStat.cpp
	testXenStore.cpp
)

################################################################################
# Targets
################################################################################

add_library(xenmock STATIC ${MOCK_SOURCES})

add_executable(unitTests ${TEST_SOURCES})

target_link_libraries(unitTests xenmock)

################################################################################
# Libraries
################################################################################

target_link_libraries(unitTests xenbe pthread)

add_test(NAME Test COMMAND unitTests)
