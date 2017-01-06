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

#ifndef INCLUDE_FRONTENDHANDLERBASE_HPP_
#define INCLUDE_FRONTENDHANDLERBASE_HPP_

#include <atomic>
#include <list>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

extern "C" {
#include <xen/io/xenbus.h>
}

#include "RingBufferBase.hpp"
#include "XenEvtchn.hpp"
#include "XenException.hpp"
#include "XenStore.hpp"
#include "Log.hpp"

namespace XenBackend {

/***************************************************************************//**
 * Exception generated by FrontendHandlerBase.
 * @ingroup Backend
 ******************************************************************************/
class FrontendHandlerException : public XenException
{
	using XenException::XenException;
};

class BackendBase;
class XenStore;

/***************************************************************************//**
 * Handles connected frontend.
 * The client should create a class inherited from FrontendHandlerBase and
 * implement onBind() method. This method is invoked when the frontend goes to
 * initialized state. The client should read the channel configuration
 * (ref for the ring buffer and port for the event channel), create DataChannel
 * instance and add with addChennel() method.
 * Example of the client frontend handler class:
 * @code{.cpp}
 * class MyFrontend : public XenBackend::FrontendHandlerBase
 * {
 *     using XenBackend::FrontendHandlerBase::FrontendHandlerBase;
 *
 * private:
 *
 *     void onBind(domid_t domId, int id)
 *     {
 *         auto port = getXenStore().readInt("/path/to/eventChannel/port");
 *         uint32_t ref = getXenStore().readInt("/path/to/ringBuffer/ref);
 *
 *         RingBufferPtr ringBuffer(new MyRingBuffer(id, type,
 *                                                   getDomId(), ref));
 *         addChannel(port, ringBuffer);
 *     }
 * };
 * @endcode
 * @ingroup Backend
 ******************************************************************************/
class FrontendHandlerBase
{
public:
	/**
	 * @param[in] name       optional frontend name
	 * @param[in] backend    reference to the backend instance
	 * @param[in] domId      frontend domain id
	 * @param[in] id         frontend instance id
	 */
	FrontendHandlerBase(const std::string& name,BackendBase& backend,
						bool waitForInitialized, domid_t domId, int id = 0);

	virtual ~FrontendHandlerBase();

	/**
	 * Returns frontend domain id
	 */
	domid_t getDomId() const { return mDomId; }

	/**
	 * Returns frontend instance id
	 */
	int getId() const {  return mId; }

	/**
	 * Returns frontend xen store base path
	 */
	const std::string& getXsFrontendPath() const { return mXsFrontendPath; }

	/**
	 * Returns reference to the xen store instance accociated with the frontend
	 */
	XenStore& getXenStore() {  return mXenStore; }

	/**
	 * Returns backend state
	 */
	xenbus_state getBackendState();

protected:
	/**
	 * Is called when the frontend goes to the initialized state.
	 * The client should override this method and create data channels when it
	 * is invoked.
	 */
	virtual void onBind() = 0;

	/**
	 * Is called when the frontend is being deleted.
	 * The client may override this method to perform some cleaning operation
	 * when frontend is being deleted.
	 */
	virtual void onDelete() {}

	/**
	 * Adds new ring buffer to the frontend handler.
	 * @param[in] ringBuffer the ring buffer instance
	 */
	void addRingBuffer(RingBufferPtr ringBuffer);

	/**
	 * Sets backend state.
	 * @param[in] state new state to set
	 */
	void setBackendState(xenbus_state state);

	/**
	 * Called when the frontend state is changed. This method can be overridden
	 * in order to implement custom frontend behavior.
	 * @param[in] state new state to set
	 */
	virtual void frontendStateChanged(xenbus_state state);

private:

	int mId;
	domid_t mDomId;
	BackendBase& mBackend;

	xenbus_state mBackendState;
	xenbus_state mFrontendState;

	XenStore mXenStore;

	std::string mXsBackendPath;
	std::string mXsFrontendPath;

	std::list<RingBufferPtr> mRingBuffers;

	bool mWaitForFrontendInitialising;

	std::string mLogId;

	mutable std::mutex mMutex;

	Log mLog;

	void run();

	void initXenStorePathes();
	void checkTerminatedChannels();
	void frontendPathChanged(const std::string& path);
	void onError(const std::exception& e);
};

typedef std::shared_ptr<FrontendHandlerBase> FrontendHandlerPtr;

}

#endif /* INCLUDE_FRONTENDHANDLERBASE_HPP_ */
