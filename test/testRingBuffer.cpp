/*
 *  Test RingBuffer
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

#include <chrono>
#include <condition_variable>
#include <mutex>

#include <catch.hpp>

#include "RingBufferBase.hpp"
#include "mocks/XenEvtchnMock.hpp"
#include "mocks/XenGnttabMock.hpp"

extern "C" {
#include "testProtocol.h"
}

using std::chrono::milliseconds;
using std::cv_status;
using std::condition_variable;
using std::mutex;
using std::unique_lock;

using XenBackend::RingBufferInBase;
using XenBackend::RingBufferOutBase;

static bool gRespNtf = false;
static mutex gMutex;
static condition_variable gCondVar;

void sendReq(xentest_req& req, xen_test_front_ring& ring)
{
	auto evtchnMock = XenEvtchnMock::getLastInstance();

	*RING_GET_REQUEST(&ring, ring.req_prod_pvt) = req;

	ring.req_prod_pvt++;

	int notify;

	RING_PUSH_REQUESTS_AND_CHECK_NOTIFY(&ring, notify);

	if (notify)
	{
		evtchnMock->signalLastBoundPort();
	}
}

bool receiveResp(xentest_rsp& rsp, xen_test_front_ring& ring)
{
	unique_lock<mutex> lock(gMutex);

	if (!gCondVar.wait_for(lock, milliseconds(1000), [] { return gRespNtf; } ))
	{
		return false;
	}

	gRespNtf = false;

	int numPendingResps = 0;

	do
	{
		RING_IDX i, rp;

		rp = ring.sring->rsp_prod;

		xen_rmb();

		for (i = ring.rsp_cons; i != rp; i++)
		{
			rsp = *RING_GET_RESPONSE(&ring, i);
		}

		ring.rsp_cons = i;

		RING_FINAL_CHECK_FOR_RESPONSES(&ring, numPendingResps);
	}
	while(numPendingResps);

	return true;
}

void respNotification()
{
	unique_lock<mutex> lock(gMutex);

	gRespNtf = true;

	gCondVar.notify_all();
}

uint32_t calculateCommand(const xentest_req& req)
{
	uint32_t value = 0;

	switch(req.id)
	{
		case XENTEST_CMD1:
			value = req.op.command1.u32data1 +
					req.op.command1.u32data1;
		break;

		case XENTEST_CMD2:
			value = req.op.command2.u64data1;
		break;

		case XENTEST_CMD3:
			value = req.op.command3.u16data1 +
					req.op.command3.u16data2 +
					req.op.command3.u32data3;
		break;
	}

	return value;
}

class TestRingBufferIn : public RingBufferInBase<xen_test_back_ring,
												 xen_test_sring,
												 xentest_req,
												 xentest_rsp>
{
public:

	TestRingBufferIn() :
		RingBufferInBase<xen_test_back_ring, xen_test_sring,
						 xentest_req, xentest_rsp>(3, 4, 12)
		{}

private:

	void processRequest(const xentest_req& req) override
	{
		xentest_rsp rsp { req.id };

		rsp.seq = req.seq;
		rsp.status = 0;
		rsp.u32data = calculateCommand(req);

		sendResponse(rsp);
	}
};

TEST_CASE("RingBufferIn", "[ringbuffer]")
{
	TestRingBufferIn ringBuffer;

	ringBuffer.start();

	// get mocks
	auto gnttabMock = XenGnttabMock::getLastInstance();
	auto evtchnMock = XenEvtchnMock::getLastInstance();

	// check mocks
	REQUIRE(gnttabMock != nullptr);
	REQUIRE(gnttabMock->getLastBuffer() != nullptr);
	REQUIRE(gnttabMock->getMapBufferSize(gnttabMock->getLastBuffer()) ==
			XC_PAGE_SIZE);

	REQUIRE(evtchnMock != nullptr);

	evtchnMock->setNotifyCbk(respNotification);

	// init ring
	xen_test_front_ring ring;
	auto sring = static_cast<xen_test_sring*>(gnttabMock->getLastBuffer());

	SHARED_RING_INIT(sring);
	FRONT_RING_INIT(&ring, sring, XC_PAGE_SIZE);

	// prepare commands
	xentest_command1_req cmd1 {32, 32};
	xentest_command2_req cmd2 {64};
	xentest_command3_req cmd3 {16, 16, 32};
	xentest_req req[3] {{XENTEST_CMD1}, {XENTEST_CMD2}, {XENTEST_CMD3}};
	req[0].op.command1 = cmd1;
	req[1].op.command2 = cmd2;
	req[2].op.command3 = cmd3;

	// send and check
	int seqNumber = 0;

	for(int i = 0; i < 1000; i++)
	{
		for(int j = 0; j < 3; j++)
		{
			req[j].seq = seqNumber++;

			sendReq(req[j], ring);

			xentest_rsp rsp {};

			REQUIRE(receiveResp(rsp, ring));

			REQUIRE(req[j].seq == rsp.seq);
			REQUIRE(calculateCommand(req[j]) == rsp.u32data);

			REQUIRE_FALSE(ringBuffer.isTerminated());
		}
	}
}

uint32_t calculateEvent(const xentest_evt& evt)
{
	uint32_t value = 0;

	switch(evt.id)
	{
		case XENTEST_EVT1:
			value = evt.op.event1.u32data1 +
					evt.op.event1.u32data1;
		break;

		case XENTEST_EVT2:
			value = evt.op.event2.u64data1;
		break;

		case XENTEST_EVT3:
			value = evt.op.event3.u16data1 +
					evt.op.event3.u16data2 +
					evt.op.event3.u32data3;
		break;
	}

	return value;
}

bool receiveEvent(xentest_event_page* eventPage, xentest_evt* eventBuffer,
				  xentest_evt& evt)
{
	uint32_t numEvents = XENTEST_IN_RING_SIZE / sizeof(xentest_evt);

	if (eventPage->in_cons != eventPage->in_prod)
	{
		evt = eventBuffer[eventPage->in_cons % numEvents];

		eventPage->in_cons++;

		return true;
	}

	return false;
}

class TestRingBufferOut : public RingBufferOutBase<xentest_event_page,
												   xentest_evt>
{
public:
	TestRingBufferOut() :
		RingBufferOutBase<xentest_event_page, xentest_evt>(
			3, 4, 12, XENTEST_IN_RING_OFFS, XENTEST_IN_RING_SIZE)
	{}
};

TEST_CASE("RingBufferOut", "[ringbuffer]")
{
	TestRingBufferOut ringBuffer;

	ringBuffer.start();

	// get mocks
	auto gnttabMock = XenGnttabMock::getLastInstance();
	auto evtchnMock = XenEvtchnMock::getLastInstance();

	// check mocks
	REQUIRE(gnttabMock != nullptr);
	REQUIRE(gnttabMock->getLastBuffer() != nullptr);
	REQUIRE(gnttabMock->getMapBufferSize(gnttabMock->getLastBuffer()) ==
			XC_PAGE_SIZE);

	REQUIRE(evtchnMock != nullptr);

	// init ring
	xentest_event_page* eventPage = static_cast<xentest_event_page*>(
			gnttabMock->getLastBuffer());
	xentest_evt* eventBuffer = reinterpret_cast<xentest_evt*>(
			static_cast<uint8_t*>(gnttabMock->getLastBuffer()) +
			XENTEST_IN_RING_OFFS);

	eventPage->in_cons = 0;

	// prepare commands
	xentest_event1 evt1 {32, 32};
	xentest_event2 evt2 {64};
	xentest_event3 evt3 {16, 16, 32};
	xentest_evt events[3] {{XENTEST_EVT1}, {XENTEST_EVT2}, {XENTEST_EVT3}};
	events[0].op.event1 = evt1;
	events[1].op.event2 = evt2;
	events[2].op.event3 = evt3;

	// send and check
	int seqNumber = 0;

	for(int i = 0; i < 1000; i++)
	{
		for(int j = 0; j < 3; j++)
		{
			events[j].seq = seqNumber++;

			ringBuffer.sendEvent(events[j]);

			xentest_evt receivedEvt {};

			REQUIRE(receiveEvent(eventPage, eventBuffer, receivedEvt));

			REQUIRE(events[j].seq == receivedEvt.seq);
			REQUIRE(calculateEvent(events[j]) == calculateEvent(receivedEvt));
			REQUIRE_FALSE(ringBuffer.isTerminated());
		}
	}
}
