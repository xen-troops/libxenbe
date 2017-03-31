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

BackendBase::BackendBase(const string& name, const string& deviceName,
						 domid_t domId) :
	mDomId(domId),
	mDeviceName(deviceName),
	mXenStore(),
	mLog(name.empty() ? "Backend" : name)
{
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

	string frontendListPath = mXenStore.getDomainPath(mDomId) + "/backend/" +
							  mDeviceName;

	mXenStore.setWatch(frontendListPath,
					   bind(&BackendBase::frontendListChanged, this, _1));
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
	if (getFrontendHandler(frontendHandler->getDomId(),
						   frontendHandler->getDevId()))
	{
		throw BackendException("Frontend already exists");
	}

	frontendHandler->start();

	mFrontendHandlers.push_back(frontendHandler);
}

/*******************************************************************************
 * Private
 ******************************************************************************/

void BackendBase::frontendListChanged(const string& path)
{
	LOG(mLog, DEBUG) << "Frontend list changed";

	// create list of existing doms
	list<domid_t> removeDomIds = mFrontendDomIds;

	for (auto frontend : mXenStore.readDirectory(path))
	{
		domid_t domId = stoi(frontend);

		auto it = find_if(mFrontendDomIds.begin(), mFrontendDomIds.end(),
						  [domId](domid_t val) { return val == domId; });

		if (it == mFrontendDomIds.end())
		{
			// add new dom
			mFrontendDomIds.push_back(domId);

			mXenStore.setWatch(path + "/" + to_string(domId),
							   bind(&BackendBase::deviceListChanged, this,
									_1, domId));
		}
		else
		{
			// remove existing from removing list
			removeDomIds.remove(domId);
		}
	}

	// remove not existing doms
	for(auto domId : removeDomIds)
	{
		mFrontendDomIds.remove(domId);

		mXenStore.clearWatch(path + "/" + to_string(domId));
	}
}

void BackendBase::deviceListChanged(const string& path, domid_t domId)
{
	LOG(mLog, DEBUG) << "Device list changed";

	list<FrontendHandlerPtr> removeFrontends;

	// create list of existing frontend
	for (auto frontend : mFrontendHandlers)
	{
		if (frontend->getDomId() == domId)
		{
			removeFrontends.push_back(frontend);
		}
	}

	for (auto device : mXenStore.readDirectory(path))
	{
		uint16_t devId = stoi(device);

		auto frontend = getFrontendHandler(domId, devId);

		if (!frontend)
		{
			LOG(mLog, INFO) << "Create new frontend: "
							<< Utils::logDomId(domId, devId);

			onNewFrontend(domId, devId);
		}
		else
		{
			// remove existing frontend from removing list
			removeFrontends.remove(frontend);
		}
	}

	// remove not existing frontends
	for(auto frontend : removeFrontends)
	{
		frontend->stop();

		mFrontendHandlers.remove(frontend);
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

}
