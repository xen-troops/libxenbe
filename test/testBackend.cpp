#define CATCH_CONFIG_RUNNER
#define CATCH_CONFIG_COLOUR_NONE

#include <chrono>
#include <condition_variable>
#include <mutex>

#include <catch.hpp>

#include "Log.hpp"
#include "mocks/XenCtrlMock.hpp"
#include "mocks/XenEvtchnMock.hpp"
#include "mocks/XenGnttabMock.hpp"
#include "mocks/XenStoreMock.hpp"
#include "testBackend.hpp"
#include "testFrontendHandler.hpp"

using std::chrono::milliseconds;
using std::condition_variable;
using std::mutex;
using std::string;
using std::to_string;
using std::unique_lock;

using XenBackend::FrontendHandlerPtr;
using XenBackend::Log;
using XenBackend::LogLevel;

static mutex gMutex;
static condition_variable gCondVar;

static domid_t gDomId = 3;
static uint16_t gDevId = 4;
static domid_t gFrontDomId = 5;
static uint16_t gFrontDevId = 12;
static const char* gDevName = "test_device";

static bool gNewFrontend = false;
static domid_t gNewFrontDomId = 0;
static uint16_t gNewFrontDevId = 0;

void TestBackend::onNewFrontend(domid_t domId, uint16_t devId)
{
	unique_lock<mutex> lock(gMutex);

	gNewFrontDomId = domId;
	gNewFrontDevId = devId;


	FrontendHandlerPtr frontendHandler(new TestFrontendHandler(gDevName,
															   getDomId(),
															   domId,
															   getDevId(),
															   devId));

	addFrontendHandler(frontendHandler);

	gNewFrontend = true;

	gCondVar.notify_all();
}

bool waitForFrontend()
{
	unique_lock<mutex> lock(gMutex);

	if (!gCondVar.wait_for(lock, milliseconds(1000),
						   [] { return gNewFrontend; } ))
	{
		return false;
	}

	gNewFrontend = false;

	return true;
}

TEST_CASE("BackendHandler", "[backendhandler]")
{
	XenEvtchnMock::setErrorMode(false);
	XenGnttabMock::setErrorMode(false);
	XenStoreMock::setErrorMode(false);

	TestBackend testBackend(gDevName, gDomId, gDevId);

	gNewFrontend = false;
	gNewFrontDomId = 0;
	gNewFrontDevId = 0;

	SECTION("Check getters")
	{
		REQUIRE(testBackend.getDomId() == gDomId);
		REQUIRE(testBackend.getDevId() == gDevId);
		REQUIRE(testBackend.getDeviceName() == gDevName);
	}

	SECTION("Check adding frontend")
	{
		TestFrontendHandler::prepareXenStore("DomU", gDevName,
											 gDomId, gFrontDomId,
											 gDevId, gFrontDevId);

		auto ctrlMock = XenCtrlMock::getLastInstance();
		auto storeMock = XenStoreMock::getLastInstance();

		ctrlMock->addDomInfo({gFrontDomId});

		testBackend.start();

		REQUIRE(waitForFrontend());
		REQUIRE(gNewFrontDomId == gFrontDomId);
		REQUIRE(gNewFrontDevId == gFrontDevId);
	}

	SECTION("Prevent start if already started")
	{
		testBackend.start();

		REQUIRE_THROWS(testBackend.start());
	}
}

int main( int argc, char* argv[] )
{
	Log::setLogMask("*:Disable");

	int result = Catch::Session().run( argc, argv );

	return ( result < 0xff ? result : 0xff );
}
