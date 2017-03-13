/*
 *  Test XenStore
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

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <mutex>

#include <catch.hpp>

#include "mocks/XenStoreMock.hpp"
#include "XenStore.hpp"

using std::chrono::milliseconds;
using std::condition_variable;
using std::exception;
using std::find;
using std::mutex;
using std::string;
using std::unique_lock;
using std::unique_ptr;
using std::vector;

using XenBackend::XenStore;
using XenBackend::XenStoreException;

static mutex gMutex;
static condition_variable gCondVar;

static int gNumErrors = 0;
static bool gWatchCbk1 = false;
static bool gWatchCbk2 = false;

static void errorHandling(const exception& e)
{
	gNumErrors++;
}

static void watchCbk1()
{
	unique_lock<mutex> lock(gMutex);

	gWatchCbk1 = true;

	gCondVar.notify_all();
}

static void watchCbk2()
{
	unique_lock<mutex> lock(gMutex);

	gWatchCbk2 = true;

	gCondVar.notify_all();
}

static void waitForWatch()
{
	unique_lock<mutex> lock(gMutex);

	gCondVar.wait_for(lock, milliseconds(100));
}

TEST_CASE("XenStore", "[xenstore]")
{
	XenStoreMock::setErrorMode(false);

	XenStore xenStore(errorHandling);
	auto mock = XenStoreMock::getLastInstance();

	SECTION("Check getting domain path")
	{
		string path = "/local/domain/3/";

		mock->setDomainPath(3, path);

		REQUIRE_THAT(xenStore.getDomainPath(3), Equals(path));
	}

	SECTION("Check getting domain path error")
	{
		XenStoreMock::setErrorMode(true);

		REQUIRE_THROWS(xenStore.getDomainPath(5));
	}

	SECTION("Check read/write")
	{
		string path = "/local/domain/3/value";
		int intVal = -34567;
		unsigned int uintVal = 23567;
		string strVal = "This is string value";

		xenStore.writeInt(path, intVal);
		REQUIRE(xenStore.readInt(path) == intVal);

		xenStore.writeUint(path, uintVal);
		REQUIRE(xenStore.readUint(path) == uintVal);

		xenStore.writeString(path, strVal);
		REQUIRE(xenStore.readString(path) == strVal);

		REQUIRE_THROWS(xenStore.readInt("/non/exist/entry"));
	}

	SECTION("Check read/write error")
	{
		XenStoreMock::setErrorMode(true);

		string path = "/local/domain/3/value";
		int intVal = -34567;
		unsigned int uintVal = 23567;
		string strVal = "This is string value";

		REQUIRE_THROWS(xenStore.writeInt(path, intVal));
		REQUIRE_THROWS(xenStore.readInt(path));

		REQUIRE_THROWS(xenStore.writeUint(path, uintVal));
		REQUIRE_THROWS(xenStore.readUint(path));

		REQUIRE_THROWS(xenStore.writeString(path, strVal));
		REQUIRE_THROWS(xenStore.readString(path));
	}

	SECTION("Check exist/remove")
	{
		string path = "/local/domain/3/exist";

		xenStore.writeString(path, "This entry exists");
		REQUIRE(xenStore.checkIfExist(path));

		xenStore.removePath(path);
		REQUIRE_FALSE(xenStore.checkIfExist(path));
	}

	SECTION("Check read directory")
	{
		string path = "/local/domain/3/directory/";
		vector<string> items = {"Item0", "Item1", "SubDir0", "SubDir1"};


		xenStore.writeString(path + items[0], "Entry 0");
		xenStore.writeString(path + items[1], "Entry 1");
		xenStore.writeString(path + items[2] + "/entry0", "Entry 0");
		xenStore.writeString(path + items[2] + "/entry1", "Entry 0");
		xenStore.writeString(path + items[3] + "/entry0", "Entry 0");
		xenStore.writeString(path + items[3] + "/entry1", "Entry 0");

		auto result = xenStore.readDirectory(path);

		for(auto item : items)
		{
			auto it = find(result.begin(), result.end(), item);

			REQUIRE(it != result.end());

			result.erase(it);
		}

		REQUIRE(result.size() == 0);

		result = xenStore.readDirectory("/non/exist/dir");

		REQUIRE(result.size() == 0);
	}

	SECTION("Check watches")
	{
		string path = "/local/domain/3/watch1";
		string value = "Value1";

		xenStore.setWatch(path, watchCbk1);

		mock->writeValue(path, "Changed");

		waitForWatch();

		REQUIRE(gWatchCbk1);

		xenStore.clearWatch(path);

		path = "/local/domain/3/watch2";
		value = "Value2";

		xenStore.setWatch(path, watchCbk2);

		mock->writeValue(path, value);

		waitForWatch();

		REQUIRE(gWatchCbk2);

		xenStore.clearWatch(path);
	}

	SECTION("Check watches error")
	{
		XenStoreMock::setErrorMode(true);

		string path = "/local/domain/3/watch1";
		string value = "Value1";

		REQUIRE_THROWS(xenStore.setWatch(path, watchCbk1));
	}

	SECTION("Check errors")
	{
		REQUIRE(gNumErrors == 0);
	}
}

TEST_CASE("XenStoreError", "[xenstore]")
{
	XenStoreMock::setErrorMode(true);

	REQUIRE_THROWS(XenStore xenStore(errorHandling));
}
