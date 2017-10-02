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
using std::unordered_map;
using std::vector;

namespace XenBackend {

/*******************************************************************************
 * FrontendHandlerBase
 ******************************************************************************/

FrontendHandlerBase::FrontendHandlerBase(const string& name,
										 const string& devName,
										 domid_t beDomId, domid_t feDomId,
										 uint16_t devId) :
	mBeDomId(beDomId),
	mFeDomId(feDomId),
	mDevId(devId),
	mDevName(devName),
	mBackendState(XenbusStateUnknown),
	mFrontendState(XenbusStateUnknown),
	mXenStore(bind(&FrontendHandlerBase::onError, this, _1)),
	mLog(name.empty() ? "FrontendHandler" : name)
{
	LOG(mLog, DEBUG) << Utils::logDomId(mFeDomId, mDevId)
					 << "Create frontend handler";

	initXenStorePathes();

	mDomName = mXenStore.readString(mXenStore.getDomainPath(mFeDomId) +
									"/name");
}

FrontendHandlerBase::~FrontendHandlerBase()
{
	stop();

	LOG(mLog, DEBUG) << Utils::logDomId(mFeDomId, mDevId)
					 << "Delete frontend handler";
}

/*******************************************************************************
 * Public
 ******************************************************************************/

void FrontendHandlerBase::start()
{
	lock_guard<mutex> lock(mMutex);

	mXenStore.start();

	setBackendState(XenbusStateInitialising);

	mXenStore.setWatch(mFeStatePath,
					   bind(&FrontendHandlerBase::frontendStateChanged, this));

	mXenStore.setWatch(mBeStatePath,
					   bind(&FrontendHandlerBase::backendStateChanged, this));
}

void FrontendHandlerBase::stop()
{
	mXenStore.clearWatches();

	mXenStore.stop();

	lock_guard<mutex> lock(mMutex);

	close(XenbusStateClosed);
}

/*******************************************************************************
 * Protected
 ******************************************************************************/

void FrontendHandlerBase::addRingBuffer(RingBufferPtr ringBuffer)
{
	LOG(mLog, INFO) << Utils::logDomId(mFeDomId, mDevId)
					<< "Add ring buffer, ref: "
					<< ringBuffer->getRef() << ", port: "
					<< ringBuffer->getPort();

	ringBuffer->setErrorCallback(bind(&FrontendHandlerBase::onError, this, _1));
	ringBuffer->start();

	mRingBuffers.push_back(ringBuffer);
}

void FrontendHandlerBase::setBackendState(xenbus_state state)
{
	if (state == mBackendState)
	{
		return;
	}

	LOG(mLog, INFO) << Utils::logDomId(mFeDomId, mDevId)
					<< "Set backend state to: "
					<< Utils::logState(state);

	auto path = mXsBackendPath + "/state";

	mBackendState = state;

	if (mXenStore.checkIfExist(mBeStatePath))
	{
		mXenStore.writeInt(path, state);
	}
}

void FrontendHandlerBase::onStateUnknown()
{
}

void FrontendHandlerBase::onStateInitializing()
{
	if (mBackendState == XenbusStateConnected)
	{
		LOG(mLog, WARNING) << Utils::logDomId(mFeDomId, mDevId)
						   << "Frontend restarted";

		close(XenbusStateInitWait);
	}

	if (mBackendState == XenbusStateInitialising ||
		mBackendState == XenbusStateClosed)
	{
		setBackendState(XenbusStateInitWait);
	}
}

void FrontendHandlerBase::onStateInitWait()
{

}

void FrontendHandlerBase::onStateInitialized()
{
	if (mBackendState == XenbusStateInitialising ||
		mBackendState == XenbusStateInitWait)
	{
		onBind();

		setBackendState(XenbusStateConnected);
	}
}

void FrontendHandlerBase::onStateConnected()
{
	if (mBackendState == XenbusStateInitialising ||
		mBackendState == XenbusStateInitWait)
	{
		onBind();

		setBackendState(XenbusStateConnected);
	}
}

void FrontendHandlerBase::onStateClosing()
{
	if (mBackendState == XenbusStateInitialised ||
		mBackendState == XenbusStateConnected)
	{
		close(XenbusStateInitWait);
	}
}

void FrontendHandlerBase::onStateClosed()
{
	if (mBackendState == XenbusStateInitialised ||
		mBackendState == XenbusStateConnected)
	{
		close(XenbusStateInitWait);
	}
}

void FrontendHandlerBase::onStateReconfiguring()
{

}

void FrontendHandlerBase::onStateReconfigured()
{

}

/*******************************************************************************
 * Private
 ******************************************************************************/

void FrontendHandlerBase::initXenStorePathes()
{
	stringstream ss;

	ss << mXenStore.getDomainPath(mFeDomId) << "/device/"
	   << mDevName << "/" << mDevId;

	mXsFrontendPath = ss.str();

	ss.str("");
	ss.clear();

	ss << mXenStore.getDomainPath(mBeDomId) << "/backend/"
	   << mDevName << "/"
	   << mFeDomId << "/" << mDevId;

	mXsBackendPath = ss.str();

	mFeStatePath = mXsFrontendPath + "/state";
	mBeStatePath = mXsBackendPath + "/state";


	LOG(mLog, DEBUG) << "Frontend path: " << mXsFrontendPath;
	LOG(mLog, DEBUG) << "Backend path:  " << mXsBackendPath;
}

void FrontendHandlerBase::frontendStateChanged()
{
	lock_guard<mutex> lock(mMutex);

	if (!mXenStore.checkIfExist(mFeStatePath))
	{
		return;
	}

	auto state = static_cast<xenbus_state>(mXenStore.readInt(mFeStatePath));

	if (state == mFrontendState)
	{
		return;
	}

	mFrontendState = state;

	LOG(mLog, INFO) << Utils::logDomId(mFeDomId, mDevId)
					<< "Frontend state changed to: "
					<< Utils::logState(state);

	onFrontendStateChanged(mFrontendState);
}

void FrontendHandlerBase::backendStateChanged()
{
	lock_guard<mutex> lock(mMutex);

	if (!mXenStore.checkIfExist(mBeStatePath))
	{
		return;
	}

	auto state = static_cast<xenbus_state>(mXenStore.readInt(mBeStatePath));

	if (state == mBackendState)
	{
		return;
	}

	mBackendState = state;

	LOG(mLog, INFO) << Utils::logDomId(mFeDomId, mDevId)
					<< "Backend state changed to: "
					<< Utils::logState(state);

	onBackendStateChanged(mBackendState);
}

void FrontendHandlerBase::onFrontendStateChanged(xenbus_state state)
{
	static unordered_map<int, StateFn> sStateTable = {
		{XenbusStateUnknown,       &FrontendHandlerBase::onStateUnknown},
		{XenbusStateInitialising,  &FrontendHandlerBase::onStateInitializing},
		{XenbusStateInitWait,      &FrontendHandlerBase::onStateInitWait},
		{XenbusStateInitialised,   &FrontendHandlerBase::onStateInitialized},
		{XenbusStateConnected,     &FrontendHandlerBase::onStateConnected},
		{XenbusStateClosing,       &FrontendHandlerBase::onStateClosing},
		{XenbusStateClosed,        &FrontendHandlerBase::onStateClosed},
		{XenbusStateReconfiguring, &FrontendHandlerBase::onStateReconfiguring},
		{XenbusStateReconfigured,  &FrontendHandlerBase::onStateReconfigured},
	};

	if (state < sStateTable.size())
	{
		(this->*sStateTable.at(state))();
	}
	else
	{
		LOG(mLog, WARNING) << Utils::logDomId(mFeDomId, mDevId)
						   << "Invalid state: " << state;
	}
}

void FrontendHandlerBase::onBackendStateChanged(xenbus_state state)
{
	if (state == XenbusStateClosing || state == XenbusStateClosed)
	{
		close(XenbusStateClosed);
	}
	else if (state == XenbusStateInitialising)
	{
		setBackendState(XenbusStateInitWait);
	}
}

void FrontendHandlerBase::onError(const std::exception& e)
{
	LOG(mLog, ERROR) << Utils::logDomId(mFeDomId, mDevId) << e.what();

	close(XenbusStateClosed);
}

void FrontendHandlerBase::release()
{
	// stop is required to prevent calling processRequest during deletion

	for(auto ringBuffer : mRingBuffers)
	{
		ringBuffer->stop();
	}

	mRingBuffers.clear();
}

void FrontendHandlerBase::close(xenbus_state stateAfterClose)
{
	if (mBackendState != XenbusStateClosed &&
		mBackendState != XenbusStateUnknown)
	{
		setBackendState(XenbusStateClosing);

		onClosing();

		release();

		setBackendState(XenbusStateClosed);

		setBackendState(stateAfterClose);
	}
}

}
