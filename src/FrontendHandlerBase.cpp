/*
 *  Xen base frontend handler
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

#include "FrontendHandlerBase.hpp"

#include <algorithm>
#include <functional>
#include <sstream>

extern "C" {
#include "xenstore.h"
#include "xenctrl.h"
}

#include "BackendBase.hpp"
#include "Utils.hpp"

using std::bind;
using std::exception;
using std::find;
using std::lock_guard;
using std::make_pair;
using std::mutex;
using std::placeholders::_1;
using std::stoi;
using std::string;
using std::stringstream;
using std::to_string;
using std::vector;

namespace XenBackend {

/*******************************************************************************
 * FrontendHandlerBase
 ******************************************************************************/

FrontendHandlerBase::FrontendHandlerBase(const string& name,
										 BackendBase& backend,
										 domid_t domId,
										 int id) :
	mId(id),
	mDomId(domId),
	mBackend(backend),
	mBackendState(XenbusStateUnknown),
	mFrontendState(XenbusStateUnknown),
	mXenStore(bind(&FrontendHandlerBase::onError, this, _1)),
	mLog(name.empty() ? "Backend" : name)
{
	mLogId = Utils::logDomId(mDomId, mId) + " - ";

	LOG(mLog, DEBUG) << mLogId << "Create frontend handler";

	initXenStorePathes();

	setBackendState(XenbusStateInitialising);

	auto statePath = mXsFrontendPath + "/state";

	mXenStore.setWatch(statePath,
					   bind(&FrontendHandlerBase::frontendStateChanged,
					   this, statePath), true);
}

FrontendHandlerBase::~FrontendHandlerBase()
{
	setBackendState(XenbusStateClosing);

	// stop is required to prevent calling processRequest during deletion
	for(auto ringBuffer : mRingBuffers)
	{
		ringBuffer->stop();
	}

	mRingBuffers.clear();

	setBackendState(XenbusStateClosed);

	LOG(mLog, DEBUG) << mLogId << "Delete frontend handler";
}

/*******************************************************************************
 * Public
 ******************************************************************************/

bool FrontendHandlerBase::isTerminated()
{
	lock_guard<mutex> lock(mMutex);

	if (mBackendState == XenbusStateClosing ||
		mBackendState == XenbusStateClosed)
	{
		return true;
	}

	for(auto ringBuffer : mRingBuffers)
	{
		if (ringBuffer->isTerminated())
		{
			LOG(mLog, ERROR) << "Ring buffer port: " << ringBuffer->getPort()
							 << ", ref: " << ringBuffer->getRef()
							 << " is terminated";

			return true;
		}
	}

	return false;
}

/*******************************************************************************
 * Protected
 ******************************************************************************/

void FrontendHandlerBase::addRingBuffer(RingBufferPtr ringBuffer)
{
	lock_guard<mutex> lock(mMutex);

	LOG(mLog, INFO) << mLogId << "Add ring buffer, ref: "
					<< ringBuffer->getRef() << ", port: "
					<< ringBuffer->getPort();

	ringBuffer->start();

	mRingBuffers.push_back(ringBuffer);
}

void FrontendHandlerBase::setBackendState(xenbus_state state)
{
	lock_guard<mutex> lock(mMutex);

	if (state == mBackendState)
	{
		return;
	}

	LOG(mLog, INFO) << mLogId << "Set backend state to: "
					<< Utils::logState(state);

	auto path = mXsBackendPath + "/state";

	mBackendState = state;

	mXenStore.writeInt(path, state);
}

void FrontendHandlerBase::onFrontendStateChanged(xenbus_state state)
{
	switch(state)
	{
	case XenbusStateInitialising:

		if (mBackendState == XenbusStateConnected)
		{
			LOG(mLog, WARNING) << mLogId << "Frontend restarted";

			setBackendState(XenbusStateClosing);
		}

		if (mBackendState == XenbusStateInitialising)
		{
			setBackendState(XenbusStateInitWait);
		}

		break;

	case XenbusStateInitialised:

		onBind();

		setBackendState(XenbusStateConnected);

		break;

	case XenbusStateClosing:
	case XenbusStateClosed:

		if (mBackendState != XenbusStateInitialising)
		{
			setBackendState(XenbusStateClosing);
		}

		break;

	default:
		break;
	}
}

/*******************************************************************************
 * Private
 ******************************************************************************/

void FrontendHandlerBase::initXenStorePathes()
{
	stringstream ss;

	ss << mXenStore.getDomainPath(mDomId) << "/device/"
	   << mBackend.getDeviceName() << "/" << mId;

	mXsFrontendPath = ss.str();

	ss.str("");
	ss.clear();

	ss << mXenStore.getDomainPath(mBackend.getId()) << "/backend/"
	   << mBackend.getDeviceName() << "/" << mDomId << "/" << mBackend.getId();

	mXsBackendPath = ss.str();

	LOG(mLog, DEBUG) << "Frontend path: " << mXsFrontendPath;
	LOG(mLog, DEBUG) << "Backend path:  " << mXsBackendPath;
}

void FrontendHandlerBase::frontendStateChanged(const string& path)
{
	try
	{
		auto state = static_cast<xenbus_state>(mXenStore.readInt(path));

		if (state == mFrontendState)
		{
			return;
		}

		mFrontendState = state;

		if (mBackendState == XenbusStateClosing ||
			mBackendState == XenbusStateClosed)
		{
			LOG(mLog, WARNING) << mLogId << "Ignore frontend state: "
							<< Utils::logState(state);
		}
		else
		{
			LOG(mLog, INFO) << mLogId << "Frontend state changed to: "
							<< Utils::logState(state);

			onFrontendStateChanged(state);
		}
	}
	catch(const exception& e)
	{
		LOG(mLog, ERROR) << mLogId << e.what();

		setBackendState(XenbusStateClosing);
	}
}

void FrontendHandlerBase::onError(const std::exception& e)
{
	LOG(mLog, ERROR) << mLogId << e.what();

	setBackendState(XenbusStateClosing);
}

}
