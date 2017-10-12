/*
 *  Xen backend base class
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

#include "BackendBase.hpp"

#include <algorithm>
#include <chrono>

#include "Utils.hpp"

using std::bind;
using std::exception;
using std::find_if;
using std::list;
using std::make_pair;
using std::unique_ptr;
using std::pair;
using std::placeholders::_1;
using std::stoi;
using std::string;
using std::to_string;
using std::vector;

namespace XenBackend {

/***************************************************************************//**
 * @mainpage libxenbe
 *
 * @section Purpose
 *
 * This library is designed to help creating Xen PV backends. It detects when
 * a new frontend is up, handles backend state machine, simplifies the Xen store
 * operations and simplifies ring buffers frontend/backend communications.
 * It consists of two main classes and some helpers classes. Main classes are
 * BackendBase and FrontendHandlerBase. BackendBase class detects new frontends
 * and monitors their states. FrontendHandlerBase handles the backend state
 * machine and monitors ring buffers states.
 *
 * Helpers classes are C++ wrappers of main Xen primitives such as Xen store,
 * Xen control, Xen grant table, Xen event channel and ring buffers.
 *
 * @section Usage
 *
 * To create a Xen backend the client should have the header file with protocol
 * description (see  testProtocol.h). Then following classes shall be
 * implemented:
 *     - a backend class inherited from BackendBase;
 *     - a frontend handler class inherited from FrontendHandlerBase;
 *     - a input ring buffer class inherited from RingBufferInBase;
 *     - an optional output ring buffer class inherited from RingBufferOutBase.
 *
 * See examples for reference.
 *
 * @example ExampleBackend.cpp
 * @example ExampleBackend.hpp
 ******************************************************************************/

/*******************************************************************************
 * BackendBase
 ******************************************************************************/

BackendBase::BackendBase(const string& name, const string& deviceName) :
	mDomId(0),
	mDeviceName(deviceName),
	mXenStore(bind(&BackendBase::onError, this, _1)),
	mLog(name.empty() ? "Backend" : name)
{
	mDomId = mXenStore.readInt("domid");

	mFrontendsPath = mXenStore.getDomainPath(mDomId) + "/backend/" +
					 mDeviceName;

	LOG(mLog, DEBUG) << "Create backend, device: " << deviceName << ", "
					 << "dom Id: " << mDomId;
}

BackendBase::~BackendBase()
{
	stop();

	for(auto frontend : mFrontendHandlers)
	{
		frontend->stop();
	}

	mFrontendHandlers.clear();

	LOG(mLog, DEBUG) << "Delete";
}

/*******************************************************************************
 * Public
 ******************************************************************************/

void BackendBase::start()
{
	mXenStore.start();

	mXenStore.setWatch(mFrontendsPath,
					   bind(&BackendBase::domainListChanged, this, _1));
}

void BackendBase::stop()
{
	mXenStore.clearWatches();

	mXenStore.stop();
}

/*******************************************************************************
 * Protected
 ******************************************************************************/

void BackendBase::addFrontendHandler(FrontendHandlerPtr frontendHandler)
{
	auto domId = frontendHandler->getDomId();
	auto devId = frontendHandler->getDevId();

	if (getFrontendHandler(domId, devId))
	{
		throw BackendException("Frontend already exists");
	}

	auto frontendPath = mFrontendsPath + "/" + to_string(domId) + "/" +
						to_string(devId);

	mXenStore.setWatch(frontendPath,
					   bind(&BackendBase::frontendPathChanged, this,
							_1, domId, devId));

	frontendHandler->start();

	mFrontendHandlers.push_back(frontendHandler);
}

/*******************************************************************************
 * Private
 ******************************************************************************/

void BackendBase::domainListChanged(const string& path)
{
	for (auto domain : mXenStore.readDirectory(path))
	{
		domid_t domId = stoi(domain);

		if (find(mDomainList.begin(), mDomainList.end(), domId) ==
			mDomainList.end())
		{
			mXenStore.setWatch(mFrontendsPath + "/" + domain,
							   bind(&BackendBase::deviceListChanged, this,
									_1, domId));

			mDomainList.push_back(domId);
		}
	}
}

void BackendBase::deviceListChanged(const string& path, domid_t domId)
{
	if (!mXenStore.checkIfExist(path))
	{
		auto it = find(mDomainList.begin(), mDomainList.end(), domId);

		if (it != mDomainList.end())
		{
			mXenStore.clearWatch(path);
			mDomainList.erase(it);
		}

		return;
	}

	for (auto device : mXenStore.readDirectory(path))
	{
		uint16_t devId = stoi(device);

		try
		{
			if (!getFrontendHandler(domId, devId))
			{
				LOG(mLog, DEBUG) << "New frontend found, domid: "
						<< domId << ", devid: " << devId;

				onNewFrontend(domId, devId);
			}
		}
		catch(const exception& e)
		{
			LOG(mLog, ERROR) << e.what();
		}
	}
}

void BackendBase::frontendPathChanged(const string& path, domid_t domId,
									  uint16_t devId)
{
	LOG(mLog, DEBUG) << "Frontend path changed: " << path;

	if (!mXenStore.checkIfExist(path))
	{
		mXenStore.clearWatch(path);

		auto frontendHandler = getFrontendHandler(domId, devId);

		if (frontendHandler)
		{
			LOG(mLog, DEBUG) << "Delete frontend, domid: "
							 << domId << ", devid: " << devId;

			frontendHandler->stop();

			mFrontendHandlers.remove(frontendHandler);
		}
	}
}

FrontendHandlerPtr BackendBase::getFrontendHandler(domid_t domId,
												   uint16_t devId)
{
	auto it = find_if(mFrontendHandlers.begin(), mFrontendHandlers.end(),
					  [&](FrontendHandlerPtr frontend) {
							return (frontend->getDomId() == domId) &&
								   (frontend->getDevId() == devId); } );

	if (it != mFrontendHandlers.end())
	{
		return *it;
	}

	return FrontendHandlerPtr();
}

void BackendBase::onError(const exception& e)
{
	LOG(mLog, ERROR) << e.what();
}

}
