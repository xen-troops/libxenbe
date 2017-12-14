/*
 *  Test XenEvtchn
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

#include "mocks/XenEvtchnMock.hpp"
#include "XenEvtchn.hpp"

using std::chrono::milliseconds;
using std::condition_variable;
using std::mutex;
using std::unique_lock;

using XenBackend::XenEvtchn;

static mutex gMutex;
static condition_variable gCondVar;

static int gNumErrors = 0;
static bool gEventChannelCbk = false;

static void errorHandling(const std::exception& e)
{
	gNumErrors++;
}

static void eventChannelCbk()
{
	unique_lock<mutex> lock(gMutex);

	gEventChannelCbk = true;

	gCondVar.notify_all();
}

static void waitForCbk()
{
	unique_lock<mutex> lock(gMutex);

	gCondVar.wait_for(lock, milliseconds(100));
}

TEST_CASE("XenEvtchn", "[xenevtchn]")
{
	XenEvtchnMock::setErrorMode(false);

	XenEvtchn eventChannel(3, 24, eventChannelCbk, errorHandling);

	eventChannel.start();

	SECTION("Check notification")
	{
		gEventChannelCbk = 0;
		gNumErrors = 0;

		eventChannel.notify();

		REQUIRE(eventChannel.getPort() == XenEvtchnMock::getLastNotifiedPort());

		XenEvtchnMock::signalPort(eventChannel.getPort());

		waitForCbk();

		REQUIRE(gEventChannelCbk);

		REQUIRE(gNumErrors == 0);
	}

	SECTION("Check second start")
	{
		REQUIRE_THROWS(eventChannel.start());
	}

	SECTION("Check error notify")
	{
		XenEvtchnMock::setErrorMode(true);

		REQUIRE_THROWS(eventChannel.notify());

		XenEvtchnMock::setErrorMode(false);
	}

	SECTION("Check error in thread")
	{
		gEventChannelCbk = 0;
		gNumErrors = 0;

		eventChannel.notify();

		XenEvtchnMock::setErrorMode(true);

		XenEvtchnMock::signalPort(eventChannel.getPort());

		waitForCbk();

		REQUIRE_FALSE(gNumErrors == 0);
	}
}

TEST_CASE("XenEvtchnError", "[xenevtchn]")
{
	XenEvtchnMock::setErrorMode(true);

	REQUIRE_THROWS(XenEvtchn(3, 24, eventChannelCbk, errorHandling));
}
