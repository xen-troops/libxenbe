/*
 *  Implementation of example backend
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

#ifndef EXAMPLE_EXAMPLEBACKEND_HPP_
#define EXAMPLE_EXAMPLEBACKEND_HPP_

#include <memory>

#include <xen/be/BackendBase.hpp>
#include <xen/be/FrontendHandlerBase.hpp>
#include <xen/be/RingBufferBase.hpp>

#include "tests/testProtocol.h"

//! [ExampleOutRingBuffer]
class ExampleOutRingBuffer : public XenBackend::RingBufferOutBase
								   <xentest_event_page, xentest_evt>
{
public:

	ExampleOutRingBuffer(domid_t domId, evtchn_port_t port, grant_ref_t ref) :
		XenBackend::RingBufferOutBase<xentest_event_page, xentest_evt>
			(domId, port, ref, XENTEST_IN_RING_OFFS, XENTEST_IN_RING_SIZE) {}

};
//! [ExampleOutRingBuffer]

//! [ExampleInRingBuffer]
class ExampleInRingBuffer : public XenBackend::RingBufferInBase
								  <xen_test_back_ring, xen_test_sring,
								   xentest_req, xentest_rsp>
{
public:

	ExampleInRingBuffer(domid_t domId, evtchn_port_t port, grant_ref_t ref) :
		XenBackend::RingBufferInBase<xen_test_back_ring, xen_test_sring,
									 xentest_req, xentest_rsp>
									(domId, port, ref),
		mLog("InRingBuffer")
	{
		LOG(mLog, DEBUG) << "Create out ring buffer, dom id: " << domId;
	}


private:

	// Override receiving requests
	virtual void processRequest(const xentest_req& req) override;

	// XenBackend::Log can be used by backend
	XenBackend::Log mLog;
};
//! [ExampleInRingBuffer]

//! [ExampleFrontend]
class ExampleFrontendHandler : public XenBackend::FrontendHandlerBase
{
public:

	ExampleFrontendHandler(const std::string& devName, domid_t feDomId) :
		FrontendHandlerBase("FrontendHandler", "example_dev", 0, feDomId),
		mLog("FrontendHandler")
	{
		LOG(mLog, DEBUG) << "Create example frontend handler, dom id: "
						 << feDomId;
	}

private:

	// Override onBind method
	void onBind() override;

	// Override onClosing method
	void onClosing() override;

	// Called when we need to send optional event to the frontend
	void onSomeEvent();

	// XenBackend::Log can be used by backend
	XenBackend::Log mLog;

	// Store out ring buffer
	std::shared_ptr<ExampleOutRingBuffer> mOutRingBuffer;
};
//! [ExampleFrontend]

//! [ExampleBackend]
class ExampleBackend : public XenBackend::BackendBase
{
public:

	ExampleBackend() :
		BackendBase("ExampleBackend", "example_dev", 0),
		mLog("ExampleBackend")
	{
		LOG(mLog, DEBUG) << "Create example backend";
	}

private:

	// override onNewFrontend method
	void onNewFrontend(domid_t domId, uint16_t devId) override;

	// XenBackend::Log can be used by backend
	XenBackend::Log mLog;
};
//! [ExampleBackend]

#endif /* EXAMPLE_EXAMPLEBACKEND_HPP_ */
