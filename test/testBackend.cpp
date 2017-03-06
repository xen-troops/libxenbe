#define CATCH_CONFIG_RUNNER
#define CATCH_CONFIG_COLOUR_NONE

#include <chrono>
#include <condition_variable>
#include <mutex>

#include <catch.hpp>

#include "BackendBase.hpp"
#include "Log.hpp"

#include "mocks/XenCtrlMock.hpp"
#include "mocks/XenStoreMock.hpp"

using std::chrono::milliseconds;
using std::condition_variable;
using std::mutex;
using std::string;
using std::to_string;
using std::unique_lock;

using XenBackend::BackendBase;
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

class TestBackend : public BackendBase
{
public:

	TestBackend() : BackendBase("TestBackend", gDevName, gDomId, gDevId)
	{}

private:

	void onNewFrontend(domid_t domId, uint16_t devId) override
	{
		unique_lock<mutex> lock(gMutex);

		gNewFrontDomId = domId;
		gNewFrontDevId = devId;

		gNewFrontend = true;

		gCondVar.notify_all();
	}
};

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
	TestBackend testBackend;

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
		auto ctrlMock = XenCtrlMock::getLastInstance();
		auto storeMock = XenStoreMock::getLastInstance();

		ctrlMock->addDomInfo({gFrontDomId});

		storeMock->setDomainPath(gFrontDomId, "/local/domain/" +
								 to_string(gFrontDomId));

		string statePath = string(storeMock->getDomainPath(gFrontDomId)) +
						   "/device/" + gDevName + "/" +
						   to_string(gFrontDevId) + "/state";

		storeMock->writeValue(statePath, to_string(XenbusStateUnknown));

		testBackend.start();

		REQUIRE(waitForFrontend());
		REQUIRE(gNewFrontDomId == gFrontDomId);
		REQUIRE(gNewFrontDevId == gFrontDevId);
	}
}

int main( int argc, char* argv[] )
{
	Log::setLogLevel(LogLevel::logDISABLE);
//	Log::setLogLevel(LogLevel::logDEBUG);

	int result = Catch::Session().run( argc, argv );

	return ( result < 0xff ? result : 0xff );
}
