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

#include "XenStoreMock.hpp"

#include <algorithm>
#include <cstdlib>
#include <cstring>

extern "C" {
#include <xenctrl.h>
#include <xenstore.h>
}

using std::find;
using std::lock_guard;
using std::mutex;
using std::string;
using std::unordered_map;
using std::vector;

/*******************************************************************************
 * Xen interface
 ******************************************************************************/

struct xs_handle
{
	XenStoreMock* mock;
};

xs_handle* xs_open(unsigned long flags)
{
	xs_handle* h = nullptr;

	if (!XenStoreMock::getErrorMode())
	{
		h = static_cast<xs_handle*>(malloc(sizeof(xs_handle)));

		h->mock = new XenStoreMock();
	}

	return h;
}

void xs_close(xs_handle* h)
{
	delete h->mock;

	free(h);
}

int xs_fileno(struct xs_handle* h)
{
	if (XenStoreMock::getErrorMode())
	{
		return -1;
	}

	return h->mock->getFd();
}

char* xs_get_domain_path(xs_handle* h, unsigned int domid)
{
	if (XenStoreMock::getErrorMode())
	{
		return nullptr;
	}

	auto path = h->mock->getDomainPath(domid);

	char* result = nullptr;

	if (path)
	{
		result = static_cast<char*>(malloc(strlen(path) + 1));

		strcpy(result, path);
	}

	return result;
}

void* xs_read(xs_handle* h, xs_transaction_t t,
			  const char* path, unsigned int* len)
{
	if (XenStoreMock::getErrorMode())
	{
		return nullptr;
	}

	auto value = h->mock->readValue(path);

	char* result = nullptr;
	*len = 0;

	if (value)
	{
		*len = strlen(value);

		result = static_cast<char*>(malloc(*len + 1));

		strcpy(result, value);
	}

	return result;
}

bool xs_write(xs_handle* h, xs_transaction_t t,
			  const char* path, const void* data, unsigned int len)
{
	if (XenStoreMock::getErrorMode())
	{
		return false;
	}

	h->mock->writeValue(path, static_cast<const char*>(data));

	return true;
}

bool xs_rm(xs_handle* h, xs_transaction_t t, const char* path)
{
	if (XenStoreMock::getErrorMode())
	{
		return false;
	}

	return h->mock->deleteEntry(path);
}

char **xs_directory(xs_handle* h, xs_transaction_t t,
					const char* path, unsigned int* num)
{
	if (XenStoreMock::getErrorMode())
	{
		return nullptr;
	}

	auto result = h->mock->readDirectory(path);

	size_t totalLength = 0;

	// calculate num of bytes for all strings plus terminate
	for(auto element : result)
	{
		totalLength += (element.length() + 1);
	}

	// add array size
	totalLength += result.size() * sizeof(char*);

	char** value = static_cast<char**>(malloc(totalLength));

	// buffer: char* array then data
	char* pos = reinterpret_cast<char*>(&value[result.size()]);

	for(size_t i = 0; i < result.size(); i++)
	{
		value[i] = pos;

		strcpy(pos, result[i].c_str());

		pos += (result[i].length() + 1);
	}

	*num = result.size();

	return value;
}

bool xs_watch(xs_handle* h, const char* path, const char* token)
{
	if (XenStoreMock::getErrorMode())
	{
		return false;
	}

	return h->mock->watch(path);
}

bool xs_unwatch(xs_handle* h, const char* path, const char* token)
{
	if (XenStoreMock::getErrorMode())
	{
		return false;
	}

	return h->mock->unwatch(path);
}

char **xs_read_watch(struct xs_handle *h, unsigned int *num)
{
	if (XenStoreMock::getErrorMode())
	{
		return nullptr;
	}

	char** value = nullptr;
	string path;

	while (h->mock->getChangedEntry(path))
	{
		size_t totalLength = 2 * sizeof(char*) + 2 * (path.length() + 1);

		value = static_cast<char**>(malloc(totalLength));
		char* pos = reinterpret_cast<char*>(&value[2]);

		value[0] = pos;

		strcpy(value[0], path.c_str());

		value[1] = pos + path.length() + 1;

		strcpy(value[1], path.c_str());
	}

	return value;

}

char** xs_check_watch(xs_handle* h)
{
	if (XenStoreMock::getErrorMode())
	{
		return nullptr;
	}

	char** value = nullptr;
	string path;

	if (h->mock->getChangedEntry(path))
	{
		size_t totalLength = 2 * sizeof(char*) + path.length() + 1;

		value = static_cast<char**>(malloc(totalLength));
		char* pos = reinterpret_cast<char*>(&value[2]);

		value[0] = pos;

		strcpy(pos, path.c_str());

		value[1] = nullptr;
	}

	return value;
}

/*******************************************************************************
 * XenStoreMock
 ******************************************************************************/

XenStoreMock* XenStoreMock::sLastInstance = nullptr;
bool XenStoreMock::mErrorMode = false;

unordered_map<unsigned int, string> XenStoreMock::mDomPathes;
unordered_map<string, string> XenStoreMock::mEntries;

XenStoreMock::XenStoreMock()
{
	sLastInstance = this;
}

/*******************************************************************************
 * Public
 ******************************************************************************/

void XenStoreMock::setDomainPath(unsigned int domId, const std::string& path)
{
	lock_guard<mutex> lock(mMutex);

	mDomPathes[domId] = path;
}

const char* XenStoreMock::getDomainPath(unsigned int domId)
{
	lock_guard<mutex> lock(mMutex);

	auto it = mDomPathes.find(domId);

	if (it != mDomPathes.end())
	{
		return it->second.c_str();
	}

	return nullptr;
}

void XenStoreMock::writeValue(const string& path, const string& value)
{
	lock_guard<mutex> lock(mMutex);

	mEntries[path] = value;

	if (mCallback)
	{
		mCallback(path, value);
	}

	pushWatch(path);
}

const char* XenStoreMock::readValue(const std::string& path)
{
	lock_guard<mutex> lock(mMutex);

	auto it = mEntries.find(path);

	if (it != mEntries.end())
	{
		return it->second.c_str();
	}

	for(auto entry : mEntries)
	{
		if (entry.first.compare(0, path.length(), path) == 0)
		{
			return "";
		}
	}

	return nullptr;
}

bool XenStoreMock::deleteEntry(const std::string& path)
{
	lock_guard<mutex> lock(mMutex);

	auto it = mEntries.find(path);

	if (it != mEntries.end())
	{
		mEntries.erase(it);

		pushWatch(path);

		return true;
	}

	return false;
}

vector<string> XenStoreMock::readDirectory(const string& path)
{
	lock_guard<mutex> lock(mMutex);

	vector<string> result;

	string dirPath = path;

	if (dirPath.back() != '/')
	{
		dirPath.append("/");
	}

	for(auto entry : mEntries)
	{
		auto element = entry.first;

		// find if entry begins with path
		if (element.find(dirPath) == 0)
		{
			// remove path
			element.erase(0, dirPath.length());

			if (!element.empty())
			{
				// remove leading '/' symbol
				if (element.front() == '/')
				{
					element.erase(element.begin());
				}

				// get element from beginning till first '/'
				auto value = element.substr(0, element.find_first_of('/'));

				// add it to result if not exists
				if (find(result.begin(), result.end(), value) == result.end())
				{
					result.push_back(value);
				}
			}
		}
	}

	return result;
}

bool XenStoreMock::watch(const std::string& path)
{
	lock_guard<mutex> lock(mMutex);

	if (find(mWatches.begin(), mWatches.end(), path) == mWatches.end())
	{
		mWatches.push_back(path);
	}

	pushWatch(path);

	return true;
}

bool XenStoreMock::unwatch(const std::string& path)
{
	lock_guard<mutex> lock(mMutex);

	auto it = find(mWatches.begin(), mWatches.end(), path);

	if (it != mWatches.end())
	{
		mWatches.erase(it);

		return true;
	}

	return false;
}

bool XenStoreMock::getChangedEntry(std::string& path)
{
	lock_guard<mutex> lock(mMutex);

	if (mChangedEntries.size())
	{
		path = mChangedEntries.front();

		mChangedEntries.pop_front();

		mPipe.read();

		return true;
	}

	return false;
}

void XenStoreMock::pushWatch(const std::string& path)
{
	if (find(mWatches.begin(), mWatches.end(), path) != mWatches.end())
	{
		mChangedEntries.push_back(path);
		mPipe.write();
	}
}
