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
 * This library is designed to help creating Xen backends. It contains classes
 * which implement backand/frontend states handling, data channels (a ring
 * buffer with associated event channel) and wrappers which provide basic Xen
 * tools functionality but in more easy and convenient way.
 *
 * @section Using
 *
 * To create a Xen backend the client should implement few classes inherited
 * from ones provided by libxenbe and override some virtual functions. The
 * classes to be inherited are: XenBackend::BackendBase,
 * XenBackend::FrontendHandlerBase and XenBackend::RingBufferBase.
 *
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
	LOG(mLog, DEBUG) << "Create backend: " << deviceName << ", " << mDevId;
}

BackendBase::~BackendBase()
{
	stop();

	mFrontendHandlers.clear();

	LOG(mLog, DEBUG) << "Delete backend: " << mDeviceName << ", " << mDevId;
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
