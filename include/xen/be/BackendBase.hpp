/*
 *  Xen backend base class
 *
 *  Based on
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

#ifndef INCLUDE_BACKENDBASE_HPP_
#define INCLUDE_BACKENDBASE_HPP_

#include <atomic>
#include <map>
#include <memory>
#include <string>
#include <utility>

#include "FrontendHandlerBase.hpp"
#include "XenException.hpp"
#include "XenStore.hpp"
#include "XenStat.hpp"
#include "Log.hpp"

namespace XenBackend {

/***************************************************************************//**
 * @defgroup Backend
 * Contains classes and primitives to create the full featured Xen backend.
 ******************************************************************************/

/***************************************************************************//**
 * Exception generated by BackendBase.
 * @ingroup Backend
 ******************************************************************************/
class BackendException : public XenException
{
	using XenException::XenException;
};

/***************************************************************************//**
 * The entry class to implement the backend.
 * The client should create a class inherited from BackendBase and implement
 * onNewFrontend() method.
 * Also the way in which the new domain is detected can be changed by overriding
 * getNewFrontend().
 * Example of the client backend class:
 *
 * @code{.cpp}
 * class MyBackend : public XenBackend::BackendBase
 * {
 *     using XenBackend::BackendBase::BackendBase;
 *
 * private:
 *
 *     void onNewFrontend(domid_t domId, int id)
 *     {
 *         addFrontendHandler(FrontendHandlerPtr(
 *                            new MyFrontendHandler(domId, *this, id)));
 *     }
 * };
 * @endcode
 * @ingroup Backend
 ******************************************************************************/
class BackendBase
{
public:
	/**
	 * @param[in] name       optional backend name
	 * @param[in] deviceName device name
	 * @param[in] domId      domain id
	 * @param[in] id         instance id
	 */
	BackendBase(const std::string& name, const std::string& deviceName,
				domid_t domId, int id = 0);
	virtual ~BackendBase();

	/**
	 * Starts backend.
	 */
	void start();

	/**
	 * Stops backend.
	 */
	void stop();

	/**
	 * Waits for backend is finished.
	 */
	void waitForFinish();

	/**
	 * Returns backend device name
	 */
	const std::string& getDeviceName() const { return mDeviceName; }

	/**
	 * Returns instance id
	 */
	int getId() const { return mId; }

	/**
	 * Returns domain id
	 */
	domid_t getDomId() const { return mDomId; }

protected:
	/**
	 * Is called periodically to check if a new frontend's appeared.
	 * In order to change the way a new domain is detected the client may
	 * override this method.
	 *
	 * @param[out] domId domain id
	 * @param[out] id    instance id
	 * @return <i>true</i> if new frontend is detected
	 */
	virtual bool getNewFrontend(domid_t& domId, int& id);

	/**
	 * Is called when new frontend detected.
	 * Basically the client should create
	 * the instance of FrontendHandlerBase class and pass it to
	 * addFrontendHandler().
	 * @param[in] domId domain id
	 * @param[in] id    instance id
	 */
	virtual void onNewFrontend(domid_t domId, int id) = 0;

	/**
	 * Adds new frontend handler
	 * @param[in] frontendHandler frontend instance
	 */
	void addFrontendHandler(FrontendHandlerPtr frontendHandler);

private:

	const int cPollFrontendIntervalMs = 500; //!< Frontend poll interval in msec

	typedef std::pair<domid_t, int> FrontendKey;

	int mId;
	domid_t mDomId;
	std::string mDeviceName;
	XenStore mXenStore;
	XenStat mXenStat;

	std::map<FrontendKey, FrontendHandlerPtr> mFrontendHandlers;

	std::thread mThread;
	std::atomic_bool mTerminate;
	std::atomic_bool mTerminated;

	Log mLog;

	void run();
	void createFrontendHandler(const FrontendKey& ids);
	void checkTerminatedFrontends();
};

}

#endif /* INCLUDE_BACKENDBASE_HPP_ */
