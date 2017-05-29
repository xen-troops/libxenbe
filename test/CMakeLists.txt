project(unitTests)

################################################################################
# Includes
################################################################################

include_directories( . )

################################################################################
# Sources
################################################################################

set(SOURCES
	mocks/Pipe.cpp
	mocks/XenCtrlMock.cpp
	mocks/XenEvtchnMock.cpp
	mocks/XenGnttabMock.cpp
	mocks/XenStoreMock.cpp
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

add_executable(unitTests ${SOURCES})


################################################################################
# Libraries
################################################################################

target_link_libraries(unitTests xenbe pthread)

add_test(NAME Test COMMAND unitTests)
