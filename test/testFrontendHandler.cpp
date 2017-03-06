/*
 *  Test FrontendHandler
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 * Copyright (C) 2016 EPAM Systems Inc.
 */

#include <chrono>
#include <condition_variable>
#include <mutex>

#include <catch.hpp>

#include "FrontendHandlerBase.hpp"
#include "mocks/XenStoreMock.hpp"

using std::bind;
using std::chrono::milliseconds;
using std::condition_variable;
using std::mutex;
using std::stoi;
using std::string;
using std::to_string;
using std::unique_lock;

using XenBackend::FrontendHandlerBase;

static mutex gMutex;
static condition_variable gCondVar;

static domid_t gDomId = 3;
static uint16_t gDevId = 4;
static const char* gDomName = "DomU";
static const char* gDevName = "test_device";

static XenbusState gBeState = XenbusStateUnknown;
static bool gOnBind = false;
static bool gBeStateChanged = false;

class TestFrontendHandler : public FrontendHandlerBase
{
public:

	TestFrontendHandler() : FrontendHandlerBase("TestFrontend",
												gDevName,
												0, gDomId,
												0, gDevId)
	{

	}

private:

	void onBind() override
	{
		gOnBind = true;
	}

};

void backendStateChanged(const string& path)
{
	unique_lock<mutex> lock(gMutex);

	gBeState = static_cast<XenbusState>(
			stoi(XenStoreMock::getLastInstance()->readValue(path)));

	gBeStateChanged = true;

	gCondVar.notify_all();
}

bool waitBeStateChanged()
{
	unique_lock<mutex> lock(gMutex);

	if (!gCondVar.wait_for(lock, milliseconds(1000),
						   [] { return gBeStateChanged; } ))
	{
		return false;
	}

	gBeStateChanged = false;

	return true;
}

TEST_CASE("FrontendHandler", "[frontendhandler]")
{
	auto storeMock = XenStoreMock::createExternInstance();

	string feDomPath = "/local/domain/" + to_string(gDomId);
	string beDomPath = "/local/domain/0";
	storeMock->setDomainPath(gDomId, feDomPath);
	storeMock->setDomainPath(0, beDomPath);
	storeMock->writeValue(feDomPath + "/name", gDomName);

	string fePath = feDomPath + "/device/" + gDevName + "/" + to_string(gDevId);
	string bePath = beDomPath + "/backend/" + gDevName + "/" +
					to_string(gDomId) + "/0";

	storeMock->writeValue(fePath + "/state", to_string(XenbusStateUnknown));

	gOnBind = false;
	gBeStateChanged = false;

	TestFrontendHandler frontendHandler;

	frontendHandler.getXenStore().setWatch(
		bePath + "/state", bind(backendStateChanged,
		bePath + "/state"));

	SECTION("Check getters")
	{
		REQUIRE(frontendHandler.getDomId() == gDomId);
		REQUIRE(frontendHandler.getDevId() == gDevId);
		REQUIRE(frontendHandler.getDomName() == gDomName);
		REQUIRE(frontendHandler.getXsFrontendPath() == fePath);
		REQUIRE(frontendHandler.getXsBackendPath() == bePath);
		REQUIRE_FALSE(frontendHandler.isTerminated());
	}

	SECTION("Check states 1")
	{
		// Initialize -> InitWait
		storeMock->writeValue(fePath + "/state",
							  to_string(XenbusStateInitialising));

		REQUIRE(waitBeStateChanged());
		REQUIRE(gBeState == XenbusStateInitWait);

		// Initialized -> Connected
		storeMock->writeValue(fePath + "/state",
							  to_string(XenbusStateInitialised));

		REQUIRE(waitBeStateChanged());
		REQUIRE(gBeState == XenbusStateConnected);
		REQUIRE(gOnBind);

		// Closing -> Closing
		storeMock->writeValue(fePath + "/state",
							  to_string(XenbusStateClosing));

		REQUIRE(waitBeStateChanged());
		REQUIRE(gBeState == XenbusStateClosing);

		REQUIRE(frontendHandler.isTerminated());
	}

	SECTION("Check states 2")
	{
		// Connected -> Connected
		storeMock->writeValue(fePath + "/state",
							  to_string(XenbusStateConnected));

		REQUIRE(waitBeStateChanged());
		REQUIRE(gBeState == XenbusStateConnected);

		REQUIRE(gOnBind);

		// Closed -> Closing
		storeMock->writeValue(fePath + "/state",
							  to_string(XenbusStateClosed));

		REQUIRE(waitBeStateChanged());
		REQUIRE(gBeState == XenbusStateClosing);

		REQUIRE(frontendHandler.isTerminated());
	}

	SECTION("Check states 3")
	{
		// Connected -> Connected
		storeMock->writeValue(fePath + "/state",
							  to_string(XenbusStateConnected));

		REQUIRE(waitBeStateChanged());
		REQUIRE(gBeState == XenbusStateConnected);

		REQUIRE(gOnBind);

		// Initializing -> Closing
		storeMock->writeValue(fePath + "/state",
							  to_string(XenbusStateInitialising));

		REQUIRE(waitBeStateChanged());
		REQUIRE(gBeState == XenbusStateClosing);

		REQUIRE(frontendHandler.isTerminated());
	}
}
