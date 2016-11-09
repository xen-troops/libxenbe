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
using std::make_pair;
using std::unique_ptr;
using std::pair;
using std::shared_ptr;
using std::stoi;
using std::string;
using std::this_thread::sleep_for;
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

BackendBase::BackendBase(int domId, const string& deviceName, int id) :
	mId(id),
	mDomId(domId),
	mDeviceName(deviceName),
	mXenStore(),
	mXenStat(),
	mTerminate(false),
	mLog("Backend")
{
	LOG(mLog, DEBUG) << "Create backend: " << deviceName << ", " << id;
}

BackendBase::~BackendBase()
{
	mFrontendHandlers.clear();

	LOG(mLog, DEBUG) << "Delete backend: " << mDeviceName << ", " << mId;
}

/*******************************************************************************
 * Public
 ******************************************************************************/

void BackendBase::run()
{
	LOG(mLog, INFO) << "Wait for frontend";

	while(!mTerminate)
	{
		int domId = -1, id = -1;

		if (getNewFrontend(domId, id))
		{
			createFrontendHandler(make_pair(domId, id));
		}

		checkTerminatedFrontends();

		sleep_for(milliseconds(cPollFrontendIntervalMs));
	}
}

void BackendBase::stop()
{
	LOG(mLog, INFO) << "Stop";

	mTerminate = true;
}

/*******************************************************************************
 * Protected
 ******************************************************************************/

void BackendBase::addFrontendHandler(shared_ptr<FrontendHandlerBase>
									 frontendHandler)
{
	pair<int, int> ids(make_pair(frontendHandler->getDomId(),
					   frontendHandler->getId()));

	mFrontendHandlers.insert(make_pair(ids, frontendHandler));
}

bool BackendBase::getNewFrontend(int& domId, int& id)
{
	for (auto dom : mXenStat.getExistingDoms())
	{
		if (dom == mDomId)
		{
			continue;
		}

		string basePath = mXenStore.getDomainPath(dom) +
						  "/device/" + mDeviceName;

		for (string strInstance : mXenStore.readDirectory(basePath))
		{
			auto instance = stoi(strInstance);

			if (mFrontendHandlers.find(make_pair(dom, instance)) ==
				mFrontendHandlers.end())
			{
				string statePath = basePath + "/" + strInstance + "/state";

				if (mXenStore.checkIfExist(statePath))
				{
					domId = dom;
					id = instance;

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

void BackendBase::createFrontendHandler(const std::pair<int, int>& ids)
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
		if (it->second->getBackendState() == XenbusStateClosing)
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
