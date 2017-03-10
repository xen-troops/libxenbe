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

#include <chrono>
#include <thread>

#include "Utils.hpp"

using std::chrono::milliseconds;
using std::exception;
using std::make_pair;
using std::unique_ptr;
using std::pair;
using std::stoi;
using std::string;
using std::this_thread::sleep_for;
using std::thread;
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
						 domid_t domId, uint16_t devId) :
	mDomId(domId),
	mDevId(devId),
	mDeviceName(deviceName),
	mXenStore(),
	mXenStat(),
	mTerminate(false),
	mTerminated(true),
	mLog(name.empty() ? "Backend" : name)
{
	LOG(mLog, DEBUG) << "Create backend, device: " << deviceName << ", "
					 << Utils::logDomId(domId, devId);
}

BackendBase::~BackendBase()
{
	stop();

	mFrontendHandlers.clear();

	LOG(mLog, DEBUG) << "Delete";
}

/*******************************************************************************
 * Public
 ******************************************************************************/

void BackendBase::start()
{
	if (!mTerminated)
	{
		throw BackendException("Already started");
	}

	LOG(mLog, INFO) << "Start";

	mTerminate = false;
	mTerminated = false;

	mThread = thread(&BackendBase::run, this);
}

void BackendBase::stop()
{
	LOG(mLog, INFO) << "Stop";

	mTerminate = true;

	waitForFinish();
}


void BackendBase::waitForFinish()
{
	if (mThread.joinable())
	{
		mThread.join();
	}
}

/*******************************************************************************
 * Protected
 ******************************************************************************/

void BackendBase::addFrontendHandler(FrontendHandlerPtr frontendHandler)
{
	FrontendKey key(make_pair(frontendHandler->getDomId(),
					frontendHandler->getDevId()));

	mFrontendHandlers.insert(make_pair(key, frontendHandler));
}

bool BackendBase::getNewFrontend(domid_t& domId, uint16_t& devId)
{
	for (auto dom : mXenStat.getExistingDoms())
	{
		if (dom == mDomId)
		{
			continue;
		}

		string basePath = mXenStore.getDomainPath(dom) +
						  "/device/" + mDeviceName;

		for (string strDev : mXenStore.readDirectory(basePath))
		{
			uint16_t dev = stoi(strDev);

			if (mFrontendHandlers.find(make_pair(dom, dev)) ==
				mFrontendHandlers.end())
			{
				string statePath = basePath + "/" + strDev + "/state";

				if (mXenStore.checkIfExist(statePath))
				{
					domId = dom;
					devId = dev;

					return true;
				}
			}
		}
	}

	return false;
}

/*******************************************************************************
 * Private
 ******************************************************************************/

void BackendBase::run()
{
	LOG(mLog, INFO) << "Wait for frontend";

	try
	{
		while(!mTerminate)
		{
			domid_t domId = 0;
			uint16_t devId = 0;

			if (getNewFrontend(domId, devId))
			{
				createFrontendHandler(make_pair(domId, devId));
			}

			checkTerminatedFrontends();

			sleep_for(milliseconds(cPollFrontendIntervalMs));
		}
	}
	catch(const exception& e)
	{
		LOG(mLog, ERROR) << e.what();
	}

	mTerminated = true;

	LOG(mLog, INFO) << "Terminated";
}

void BackendBase::createFrontendHandler(const FrontendKey& ids)
{
	if ((ids.first > 0) &&
		(mFrontendHandlers.find(ids) == mFrontendHandlers.end()))
	{
		LOG(mLog, INFO) << "Create new frontend: "
						<< Utils::logDomId(ids.first, ids.second);

		onNewFrontend(ids.first, ids.second);
	}
	else
	{
		LOG(mLog, WARNING) << "Domain already exists: "
						   << Utils::logDomId(ids.first, ids.second);
	}
}

void BackendBase::checkTerminatedFrontends()
{
	for (auto it = mFrontendHandlers.begin(); it != mFrontendHandlers.end();)
	{
		if (it->second->isTerminated())
		{
			LOG(mLog, INFO) << "Delete terminated frontend: "
							<< Utils::logDomId(it->first.first,
											   it->first.second);

			it = mFrontendHandlers.erase(it);
		}
		else
		{
			++it;
		}
	}
}

}
