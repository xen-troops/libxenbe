/*
 *  XenStoreMock
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

#ifndef TESTS_MOCKS_XENSTOREMOCK_HPP_
#define TESTS_MOCKS_XENSTOREMOCK_HPP_

#include <functional>
#include <list>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "../../tests/mocks/Pipe.hpp"

class XenStoreMock
{
public:

	XenStoreMock();
	~XenStoreMock();

	static void setErrorMode(bool errorMode)
	{
		std::lock_guard<std::mutex> lock(sMutex);

		sErrorMode = errorMode;
	}
	static bool getErrorMode()
	{
		std::lock_guard<std::mutex> lock(sMutex);

		return sErrorMode;
	}
	static void setDomainPath(unsigned int domId, const std::string& path);
	static const char* getDomainPath(unsigned int domId);
	static void writeValue(const std::string& path, const std::string& value);
	static const char* readValue(const std::string& path);
	static bool deleteEntry(const std::string& path);
	static std::vector<std::string> readDirectory(const std::string& path);

	int getFd() const { return mPipe.getFd(); }
	bool watch(const std::string& path);
	bool unwatch(const std::string& path);
	bool getChangedEntry(std::string& path);

	typedef std::function<void(const std::string& path,
							   const std::string& value)> Callback;

	static void setWriteValueCbk(Callback cbk)
	{
		std::lock_guard<std::mutex> lock(sMutex);

		sCallback = cbk;
	}

private:

	static std::mutex sMutex;
	static bool sErrorMode;
	static std::unordered_map<unsigned int, std::string> sDomPathes;
	static std::unordered_map<std::string, std::string> sEntries;
	static std::list<XenStoreMock*> sClients;
	static Callback sCallback;

	Pipe mPipe;

	std::list<std::string> mWatches;
	std::list<std::string> mChangedEntries;

	static void pushWatch(const std::string& path);
};

#endif /* TESTS_MOCKS_XENSTOREMOCK_HPP_ */
