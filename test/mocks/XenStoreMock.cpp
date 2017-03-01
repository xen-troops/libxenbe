/*
 * xenstore.c
 *
 *  Created on: Feb 27, 2017
 *      Author: al1
 */

#include "XenStoreMock.hpp"

#include <algorithm>
#include <cstdlib>
#include <cstring>

extern "C" {
	#include <xenstore.h>
}

struct xs_handle
{
	XenStoreMock* mock;
};

using std::find;
using std::string;
using std::vector;

/*******************************************************************************
 * Xen interface
 ******************************************************************************/

xs_handle* xs_open(unsigned long flags)
{
	xs_handle* h = static_cast<xs_handle*>(malloc(sizeof(xs_handle)));

	h->mock = XenStoreMock::getInstance();

	return h;
}

void xs_close(xs_handle* h)
{
	delete h->mock;

	free(h);
}

int xs_fileno(struct xs_handle* h)
{
	return h->mock->getFd();
}

char* xs_get_domain_path(xs_handle* h, unsigned int domid)
{
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
	h->mock->writeValue(path, static_cast<const char*>(data));

	return true;
}

bool xs_rm(xs_handle* h, xs_transaction_t t, const char* path)
{
	return h->mock->deleteEntry(path);
}

char **xs_directory(xs_handle* h, xs_transaction_t t,
					const char* path, unsigned int* num)
{
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
	return h->mock->watch(path);
}

bool xs_unwatch(xs_handle* h, const char* path, const char* token)
{
	return h->mock->unwatch(path);
}

char** xs_check_watch(xs_handle* h)
{
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

XenStoreMock* XenStoreMock::sInstance = nullptr;

XenStoreMock* XenStoreMock::getInstance()
{
	if (sInstance == nullptr)
	{
		sInstance = new XenStoreMock();
	}

	return sInstance;
}

XenStoreMock::~XenStoreMock()
{
	sInstance = nullptr;
}

/*******************************************************************************
 * Public
 ******************************************************************************/

void XenStoreMock::setDomainPath(unsigned int domId, const std::string& path)
{
	mDomPathes[domId] = path;
}

const char* XenStoreMock::getDomainPath(unsigned int domId)
{
	auto it = mDomPathes.find(domId);

	if (it != mDomPathes.end())
	{
		return it->second.c_str();
	}

	return nullptr;
}

void XenStoreMock::writeValue(const string& path, const string& value)
{
	mEntries[path] = value;

	auto it = find(mWatches.begin(), mWatches.end(), path);

	if (it != mWatches.end())
	{
		if (find(mChangedEntries.begin(), mChangedEntries.end(), *it) ==
			mChangedEntries.end())
		{
			mChangedEntries.push_back(*it);

			mPipe.write();
		}
	}
}

const char* XenStoreMock::readValue(const std::string& path)
{
	auto it = mEntries.find(path);

	if (it != mEntries.end())
	{
		return it->second.c_str();
	}

	return nullptr;
}

bool XenStoreMock::deleteEntry(const std::string& path)
{
	auto it = mEntries.find(path);

	if (it != mEntries.end())
	{
		mEntries.erase(it);

		return true;
	}

	return false;
}

vector<string> XenStoreMock::readDirectory(const string& path)
{
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
	if (find(mWatches.begin(), mWatches.end(), path) == mWatches.end())
	{
		mWatches.push_back(path);
	}

	return true;
}

bool XenStoreMock::unwatch(const std::string& path)
{
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
	if (mChangedEntries.size())
	{
		path = mChangedEntries.front();

		mChangedEntries.pop_front();

		mPipe.read();

		return true;
	}

	return false;
}