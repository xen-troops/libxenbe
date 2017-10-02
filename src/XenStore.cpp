/*
 *  Xen Store wrapper
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
#include "XenStore.hpp"

#include <poll.h>

using std::exception;
using std::lock_guard;
using std::mutex;
using std::string;
using std::thread;
using std::to_string;
using std::vector;

namespace XenBackend {

/*******************************************************************************
 * XenStore
 ******************************************************************************/

XenStore::XenStore(ErrorCallback errorCallback) :
	mXsHandle(nullptr),
	mErrorCallback(errorCallback),
	mStarted(false),
	mLog("XenStore")
{
	try
	{
		init();
	}
	catch(const XenException& e)
	{
		release();

		throw;
	}
}

XenStore::~XenStore()
{
	clearWatches();

	stop();

	release();
}

/*******************************************************************************
 * Public
 ******************************************************************************/

string XenStore::getDomainPath(domid_t domId)
{
	auto domPath = xs_get_domain_path(mXsHandle, domId);

	if (!domPath)
	{
		throw XenStoreException("Can't get domain path");
	}

	string result(domPath);

	free(domPath);

	return result;
}

int XenStore::readInt(const string& path)
{
	int result = stoi(readString(path));

	LOG(mLog, DEBUG) << "Read int " << path << " : " << result;

	return result;
}

unsigned int XenStore::readUint(const string& path)
{
	unsigned int result = stoul(readString(path));

	LOG(mLog, DEBUG) << "Read unsigned int " << path << " : " << result;

	return result;
}

string XenStore::readString(const string& path)
{
	unsigned length;
	auto pData = static_cast<char*>(xs_read(mXsHandle, XBT_NULL, path.c_str(),
											&length));

	if (!pData)
	{
		throw XenStoreException("Can't read from: " + path);
	}

	string result(pData);

	free(pData);

	LOG(mLog, DEBUG) << "Read string " << path << " : " << result;

	return result;
}

void XenStore::writeInt(const string& path, int value)
{
	auto strValue = to_string(value);

	LOG(mLog, DEBUG) << "Write int " << path << " : " << value;

	writeString(path, strValue);
}

void XenStore::writeUint(const string& path, unsigned int value)
{
	auto strValue = to_string(value);

	LOG(mLog, DEBUG) << "Write uint " << path << " : " << value;

	writeString(path, strValue);
}

void XenStore::writeString(const string& path, const string& value)
{
	LOG(mLog, DEBUG) << "Write string " << path << " : " << value;

	if (!xs_write(mXsHandle, XBT_NULL, path.c_str(), value.c_str(),
				  value.length()))
	{
		throw XenStoreException("Can't write value to " + path);
	}
}

void XenStore::removePath(const string& path)
{
	LOG(mLog, DEBUG) << "Remove path " << path;

	if (!xs_rm(mXsHandle, XBT_NULL, path.c_str()))
	{
		throw XenStoreException("Can't remove path " + path);
	}
}

vector<string> XenStore::readDirectory(const string& path)
{
	unsigned int num;
	auto items = xs_directory(mXsHandle, XBT_NULL, path.c_str(), &num);

	if (items && num)
	{
		vector<string> result;

		result.reserve(num);

		for(unsigned int i = 0; i < num; i++)
		{
			result.push_back(items[i]);
		}

		free(items);

		return result;
	}

	return vector<string>();
}

bool XenStore::checkIfExist(const string& path)
{
	unsigned length;
	auto pData = xs_read(mXsHandle, XBT_NULL, path.c_str(), &length);

	if (!pData)
	{
		return false;
	}

	free(pData);

	return true;
}

void XenStore::setWatch(const string& path, WatchCallback callback)
{
	lock_guard<mutex> lock(mMutex);

	LOG(mLog, DEBUG) << "Set watch: " << path;

	if (!xs_watch(mXsHandle, path.c_str(), path.c_str()))
	{
		throw XenStoreException("Can't set xs watch for " + path);
	}

	mWatches[path] = callback;
}

void XenStore::clearWatch(const string& path)
{
	lock_guard<mutex> lock(mMutex);

	LOG(mLog, DEBUG) << "Clear watch: " << path;

	if (!xs_unwatch(mXsHandle, path.c_str(), path.c_str()))
	{
		LOG(mLog, ERROR) << "Failed to clear watch: " << path;
	}

	mWatches.erase(path);
}

void XenStore::clearWatches()
{
	lock_guard<mutex> lock(mMutex);

	if (mWatches.size())
	{
		LOG(mLog, DEBUG) << "Clear watches";

		for (auto watch : mWatches)
		{
			if (!xs_unwatch(mXsHandle, watch.first.c_str(), watch.first.c_str()))
			{
				LOG(mLog, ERROR) << "Failed to clear watch: " << watch.first;
			}
		}

		mWatches.clear();
	}
}

void XenStore::start()
{
	DLOG(mLog, DEBUG) << "Start";

	if (mStarted)
	{
		throw XenStoreException("XenStore is already started");
	}

	mStarted = true;

	mThread = thread(&XenStore::watchesThread, this);
}

void XenStore::stop()
{
	if (!mStarted)
	{
		return;
	}

	DLOG(mLog, DEBUG) << "Stop";

	if (mPollFd)
	{
		mPollFd->stop();
	}

	if (mThread.joinable())
	{
		mThread.join();
	}

	mStarted = false;
}

/*******************************************************************************
 * Private
 ******************************************************************************/

void XenStore::init()
{
	mXsHandle = xs_open(0);

	if (!mXsHandle)
	{
		throw XenStoreException("Can't open xs daemon");
	}

	mPollFd.reset(new PollFd(xs_fileno(mXsHandle), POLLIN));

	LOG(mLog, DEBUG) << "Create xen store";
}

void XenStore::release()
{
	if (mXsHandle)
	{
		xs_close(mXsHandle);

		LOG(mLog, DEBUG) << "Delete xen store";
	}
}

string XenStore::readXsWatch(string& token)
{
	string path;
	unsigned int num;

	auto result = xs_read_watch(mXsHandle, &num);

	if (result)
	{
		path = result[XS_WATCH_PATH];
		token = result[XS_WATCH_TOKEN];

		free(result);
	}

	return path;
}

XenStore::WatchCallback XenStore::getWatchCallback(const string& path)
{
	lock_guard<mutex> lock(mMutex);

	WatchCallback callback = nullptr;
/*
	auto result = find_if(mWatches.begin(), mWatches.end(),
						  [&path] (const pair<string, WatchCallback> &val)
						  { return path.compare(0, val.first.length(),
						                           val.first) == 0; });
*/
	auto result = mWatches.find(path);

	if (result != mWatches.end())
	{
		callback = result->second;
	}

	return callback;
}

void XenStore::watchesThread()
{
	try
	{
		while(mPollFd->poll())
		{
			string token;

			auto path = readXsWatch(token);

			if (!token.empty())
			{
				auto callback = getWatchCallback(token);

				if (callback)
				{
					LOG(mLog, DEBUG) << "Watch triggered: " << token;

					callback(token);
				}
			}
		}
	}
	catch(const exception& e)
	{
		if (mErrorCallback)
		{
			mErrorCallback(e);
		}
		else
		{
			LOG(mLog, ERROR) << e.what();
		}
	}
}

}
