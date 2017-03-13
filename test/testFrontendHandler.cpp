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

#include "testFrontendHandler.hpp"
#include "mocks/XenEvtchnMock.hpp"
#include "mocks/XenGnttabMock.hpp"
#include "mocks/XenStoreMock.hpp"
#include "testRingBuffer.hpp"

using std::bind;
using std::chrono::milliseconds;
using std::condition_variable;
using std::mutex;
using std::stoi;
using std::string;
using std::this_thread::sleep_for;
using std::to_string;
using std::unique_lock;

using XenBackend::FrontendHandlerBase;
using XenBackend::RingBufferInBase;
using XenBackend::RingBufferPtr;

static mutex gMutex;
static condition_variable gCondVar;

static domid_t gDomId = 3;
static uint16_t gDevId = 4;
static const char* gDomName = "DomU";
static const char* gDevName = "test_device";

static XenbusState gBeState = XenbusStateUnknown;
static bool gOnBind = false;
static bool gBeStateChanged = false;

void TestFrontendHandler::prepareXenStore(const string& domName,
										  const string& devName,
										  domid_t beDomId, domid_t feDomId,
										  uint16_t beDevId, uint16_t feDevId)
{

	XenStoreMock storeMock;

	string feDomPath = "/local/domain/" + to_string(feDomId);
	string beDomPath = "/local/domain/" + to_string(beDomId);

	storeMock.setDomainPath(feDomId, feDomPath);
	storeMock.setDomainPath(beDomId, beDomPath);

	storeMock.writeValue(feDomPath + "/name", domName);

	string fePath = feDomPath + "/device/" + devName + "/" + to_string(feDevId);
	string bePath = beDomPath + "/backend/" + devName + "/" +
					to_string(beDomId) + "/" + to_string(beDevId);

	storeMock.writeValue(fePath + "/state", to_string(XenbusStateUnknown));
}

void TestFrontendHandler::onBind()
{
	RingBufferPtr ringBuffer(new TestRingBufferIn(gDomId, 12, 165));

	addRingBuffer(ringBuffer);

	gOnBind = true;
}

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
	XenEvtchnMock::setErrorMode(false);
	XenGnttabMock::setErrorMode(false);
	XenStoreMock::setErrorMode(false);

	TestFrontendHandler::prepareXenStore(gDomName, gDevName, 0,
										 gDomId, 0, gDevId);

	gOnBind = false;
	gBeStateChanged = false;

	TestFrontendHandler frontendHandler(gDevName, 0, gDomId, 0, gDevId);

	auto storeMock = XenStoreMock::getLastInstance();

	auto fePath = frontendHandler.getXsFrontendPath();

	frontendHandler.getXenStore().setWatch(
		frontendHandler.getXsBackendPath() + "/state", bind(backendStateChanged,
		frontendHandler.getXsBackendPath() + "/state"));

	SECTION("Check getters")
	{
		REQUIRE(frontendHandler.getDomId() == gDomId);
		REQUIRE(frontendHandler.getDevId() == gDevId);
		REQUIRE(frontendHandler.getDomName() == gDomName);
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

	SECTION("Check error")
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

		XenEvtchnMock::setErrorMode(true);

		auto mockEvtchn = XenEvtchnMock::getLastInstance();

		mockEvtchn->signalLastBoundPort();

		// wait when error is detected
		sleep_for(milliseconds(200));

		REQUIRE(frontendHandler.isTerminated());
	}
}
