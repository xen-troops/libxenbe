project(unitTests)

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

add_executable(unitTests ${SOURCES})

target_link_libraries(unitTests xenbe pthread)

add_test(NAME Test COMMAND unitTests)
