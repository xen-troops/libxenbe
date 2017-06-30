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

#ifndef TEST_MOCKS_XENSTOREMOCK_HPP_
#define TEST_MOCKS_XENSTOREMOCK_HPP_

#include <functional>
#include <list>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "Pipe.hpp"

class XenStoreMock
{
public:

	XenStoreMock();

	static XenStoreMock* getLastInstance() { return sLastInstance; }
	static void setErrorMode(bool errorMode) { mErrorMode = errorMode; }
	static bool getErrorMode() { return mErrorMode; }

	int getFd() const { return mPipe.getFd(); }
	void setDomainPath(unsigned int domId, const std::string& path);
	const char* getDomainPath(unsigned int domId);
	void writeValue(const std::string& path, const std::string& value);
	const char* readValue(const std::string& path);
	bool deleteEntry(const std::string& path);
	std::vector<std::string> readDirectory(const std::string& path);
	bool watch(const std::string& path);
	bool unwatch(const std::string& path);
	bool getChangedEntry(std::string& path);

	typedef std::function<void(const std::string& path,
							   const std::string& value)> Callback;

	void setWriteValueCbk(Callback cbk) { mCallback = cbk; }

private:

	static XenStoreMock* sLastInstance;
	static bool mErrorMode;

	std::mutex mMutex;

	Pipe mPipe;

	static std::unordered_map<unsigned int, std::string> mDomPathes;
	static std::unordered_map<std::string, std::string> mEntries;

	std::list<std::string> mWatches;
	std::list<std::string> mChangedEntries;
	Callback mCallback;

	void pushWatch(const std::string& path);
};

#endif /* TEST_MOCKS_XENSTOREMOCK_HPP_ */
