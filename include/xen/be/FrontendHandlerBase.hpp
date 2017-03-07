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
 * @ingroup backend
 ******************************************************************************/
class FrontendHandlerException : public XenException
{
	using XenException::XenException;
};

class BackendBase;
class XenStore;

/***************************************************************************//**
 * Handles the connected frontend.
 *
 * The frontend handler implements the frontend-backend state machine, keeps
 * and monitors associated ring buffers.
 *
 * Backend state machine:
 * @dot
 * digraph stateMachine {
 *     rankdir=LR;
 *     size="8,5"
 *     node [shape = rectangle];
 *     Initializing -> InitWait [ label = "Initializing" ]
 *     InitWait ->  Connected [ label = "Initialized" ]
 *     Connected -> Closing [ label = "Closing" ]
 *     Connected -> Closing [ label = "Closed" ]
 *     Connected -> Closing [ label = "Initializing" ]
 *     Closing -> Closed
 * }
 * @enddot
 *
 * Initially backend is in XenbusStateInitialising. When the frontend goes to
 * XenbusStateInitialising, the backend switches to XenbusStateInitWait.
 * Once the frontend creates event channels and ring buffers for communication,
 * it goes to XenbusStateInitialised state. It means that the backend can open
 * and map event channels and ring buffers on its side. Before going to
 * XenbusStateConnected state the frontend handler call onBind() method.
 * Usually the client should create ring buffers or perform other initialization
 * inside onBind() method. Created ring buffers should be added by
 * addRingBuffer() method in order to allow the frontend handler to monitor
 * their states.
 *
 * If the client desires to change the default state machine behavior, it may
 * override some of onState...() methods. These methods are called when the
 * frontend goes to appropriate state:
 *
 * <table>
 * <tr><th>State                    <th>Method
 * <tr><td>XenbusStateInitialising  <td>onStateInitializing()
 * <tr><td>XenbusStateInitWait      <td>onStateInitWait()
 * <tr><td>XenbusStateInitialised   <td>onStateInitialized()
 * <tr><td>XenbusStateConnected     <td>onStateConnected()
 * <tr><td>XenbusStateClosing       <td>onStateInitialized()
 * <tr><td>XenbusStateClosed        <td>onStateConnected()
 * <tr><td>XenbusStateReconfiguring <td>onStateReconfiguring()
 * <tr><td>XenbusStateReconfigured  <td>onStateReconfigured()
 * </table>
 *
 * The client should implement a class inherited from FrontendHandlerBase and
 * override onBind() method. The instance of this class should be created in
 * BackendBase::onNewFrontend(). The client should read the event channel and
 * ring buffer configurations from Xen store or using any other mechanism
 * according to the protocol. Then the client should create appropriate
 * ring buffers with negotiated parameters and add them with addRingBuffer().
 *
 * Example of the client frontend handler class:
 * @code{.cpp}
 * class MyFrontend : public XenBackend::FrontendHandlerBase
 * {
 *     using XenBackend::FrontendHandlerBase::FrontendHandlerBase;
 *
 * private:
 *
 *     void onBind(domid_t domId, uint16_t devId)
 *     {
 *         auto port = getXenStore().readInt("/path/to/eventChannel/port");
 *         uint32_t ref = getXenStore().readInt("/path/to/ringBuffer/ref);
 *
 *         RingBufferPtr ringBuffer(new MyRingBuffer(getDomId(), port, ref));
 *
 *         addRingBuffer(ringBuffer);
 *     }
 * };
 * @endcode
 * @ingroup backend
 ******************************************************************************/
class FrontendHandlerBase
{
public:
	/**
	 * @param[in] name                optional frontend name
	 * @param[in] devName             device name
	 * @param[in] beDomId             backend domain id
	 * @param[in] feDomId             frontend domain id
	 * @param[in] beDevId             backend device id
	 * @param[in] feDevId             frontend device id
	 */
	FrontendHandlerBase(const std::string& name, const std::string& devName,
						domid_t beDomId, domid_t feDomId,
						uint16_t beDevId = 0, uint16_t feDevId = 0);

	virtual ~FrontendHandlerBase();

	/**
	 * Returns frontend domain id
	 */
	domid_t getDomId() const { return mFeDomId; }

	/**
	 * Returns frontend device id
	 */
	uint16_t getDevId() const {  return mFeDevId; }

	/**
	 * Returns domain name
	 */
	std::string getDomName() const {  return mDomName; }

	/**
	 * Returns frontend xen store base path
	 */
	std::string getXsFrontendPath() const { return mXsFrontendPath; }

	/**
	 * Returns backend xen store base path
	 */
	std::string getXsBackendPath() const { return mXsBackendPath; }

	/**
	 * Returns reference to the xen store instance accociated with the frontend
	 */
	XenStore& getXenStore() {  return mXenStore; }

	/**
	 * Checks if frontend is terminated
	 */
	bool isTerminated();

protected:
	/**
	 * Is called when the frontend goes to the initialized state.
	 * The client should override this method and create data channels when it
	 * is invoked.
	 */
	virtual void onBind() = 0;

	/**
	 * Adds new ring buffer to the frontend handler.
	 * @param[in] ringBuffer the ring buffer instance
	 */
	void addRingBuffer(RingBufferPtr ringBuffer);

	/**
	 * Returns current backend state.
	 */
	xenbus_state getBackendState() const { return mBackendState; }

	/**
	 * Sets backend state.
	 * @param[in] state new state to set
	 */
	void setBackendState(xenbus_state state);

	/**
	 * Called when the frontend state changed to XenbusStateInitialising
	 */
	virtual void onStateInitializing();

	/**
	 * Called when the frontend state changed to XenbusStateInitWait
	 */
	virtual void onStateInitWait();

	/**
	 * Called when the frontend state changed to XenbusStateInitialized
	 */
	virtual void onStateInitialized();

	/**
	 * Called when the frontend state changed to XenbusStateConnected
	 */
	virtual void onStateConnected();

	/**
	 * Called when the frontend state changed to XenbusStateClosing
	 */
	virtual void onStateClosing();

	/**
	 * Called when the frontend state changed to XenbusStateClosed
	 */
	virtual void onStateClosed();

	/**
	 * Called when the frontend state changed to XenbusStateReconfiguring
	 */
	virtual void onStateReconfiguring();

	/**
	 * Called when the frontend state changed to XenbusStateReconfigured
	 */
	virtual void onStateReconfigured();

private:

	typedef void(FrontendHandlerBase::*StateFn)();

	domid_t mBeDomId;
	domid_t mFeDomId;
	uint16_t mBeDevId;
	uint16_t mFeDevId;
	std::string mDevName;
	std::string mDomName;

	xenbus_state mBackendState;
	xenbus_state mFrontendState;

	XenStore mXenStore;

	std::string mXsBackendPath;
	std::string mXsFrontendPath;

	std::vector<RingBufferPtr> mRingBuffers;

	std::string mLogId;

	mutable std::mutex mMutex;

	Log mLog;

	void run();

	void initXenStorePathes();
	void checkTerminatedChannels();
	void frontendStateChanged(const std::string& path);
	void onFrontendStateChanged(xenbus_state state);
	void onError(const std::exception& e);
};

typedef std::shared_ptr<FrontendHandlerBase> FrontendHandlerPtr;

}

#endif /* INCLUDE_FRONTENDHANDLERBASE_HPP_ */
