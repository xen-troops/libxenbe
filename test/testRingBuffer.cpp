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

#include "testRingBuffer.hpp"

#include <chrono>
#include <condition_variable>
#include <mutex>

#include <catch.hpp>

#include "mocks/XenEvtchnMock.hpp"
#include "mocks/XenGnttabMock.hpp"

using std::chrono::milliseconds;
using std::condition_variable;
using std::mutex;
using std::this_thread::sleep_for;
using std::unique_lock;

using XenBackend::RingBufferInBase;
using XenBackend::RingBufferOutBase;

static domid_t gDomId = 3;
static evtchn_port_t gPort = 65;
static grant_ref_t gRef = 23;

static bool gRespNtf = false;
static mutex gMutex;
static condition_variable gCondVar;

static bool gError = false;

void sendReq(xentest_req& req, xen_test_front_ring& ring)
{
	*RING_GET_REQUEST(&ring, ring.req_prod_pvt) = req;

	ring.req_prod_pvt++;

	int notify;

	RING_PUSH_REQUESTS_AND_CHECK_NOTIFY(&ring, notify);

	if (notify)
	{
		XenEvtchnMock::signalPort(XenEvtchnMock::getLastBoundPort());
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

		XenEvtchnMock::signalPort(XenEvtchnMock::getLastBoundPort());

		return true;
	}

	return false;
}

void TestRingBufferIn::processRequest(const xentest_req& req)
{
	xentest_rsp rsp { req.id };

	rsp.seq = req.seq;
	rsp.status = 0;
	rsp.u32data = calculateCommand(req);

	sendResponse(rsp);
}

void errorCallback(const std::exception& e)
{
	gError = true;
}

TEST_CASE("RingBufferIn", "[ringbuffer]")
{
	XenEvtchnMock::setErrorMode(false);
	XenGnttabMock::setErrorMode(false);

	gError = false;

	TestRingBufferIn ringBuffer(gDomId, gPort, gRef);

	ringBuffer.setErrorCallback(errorCallback);

	REQUIRE(ringBuffer.getPort() == gPort);
	REQUIRE(ringBuffer.getRef() == gRef);

	ringBuffer.start();

	// check mocks
	REQUIRE(XenGnttabMock::getLastBuffer() != nullptr);
	REQUIRE(XenGnttabMock::getMapBufferSize(XenGnttabMock::getLastBuffer()) ==
			XC_PAGE_SIZE);

	XenEvtchnMock::setNotifyCbk(XenEvtchnMock::getLastBoundPort(),
								respNotification);

	// init ring
	xen_test_front_ring ring;
	auto sring = static_cast<xen_test_sring*>(XenGnttabMock::getLastBuffer());

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

	int seqNumber = 0;

	SECTION("Send and receive")
	{
		// send and check
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

				REQUIRE_FALSE(gError);
			}
		}
	}

	SECTION("Check overflow")
	{
		sring->req_prod = ring.nr_ents + 1;

		XenEvtchnMock::signalPort(XenEvtchnMock::getLastBoundPort());

		// wait when error is detected
		sleep_for(milliseconds(100));

		REQUIRE(gError);
	}
}

TEST_CASE("RingBufferOut", "[ringbuffer]")
{
	XenEvtchnMock::setErrorMode(false);
	XenGnttabMock::setErrorMode(false);

	gError = false;

	TestRingBufferOut ringBuffer(gDomId, gPort, gRef);

	ringBuffer.setErrorCallback(errorCallback);

	ringBuffer.start();

	// check mocks
	REQUIRE(XenGnttabMock::getLastBuffer() != nullptr);
	REQUIRE(XenGnttabMock::getMapBufferSize(XenGnttabMock::getLastBuffer()) ==
			XC_PAGE_SIZE);

	// init ring
	xentest_event_page* eventPage = static_cast<xentest_event_page*>(
			XenGnttabMock::getLastBuffer());
	xentest_evt* eventBuffer = reinterpret_cast<xentest_evt*>(
			static_cast<uint8_t*>(XenGnttabMock::getLastBuffer()) +
			XENTEST_IN_RING_OFFS);

	eventPage->in_cons = 0;
	eventPage->in_prod = 0;

	// prepare commands
	xentest_event1 evt1 {32, 32};
	xentest_event2 evt2 {64};
	xentest_event3 evt3 {16, 16, 32};
	xentest_evt events[3] {{XENTEST_EVT1}, {XENTEST_EVT2}, {XENTEST_EVT3}};
	events[0].op.event1 = evt1;
	events[1].op.event2 = evt2;
	events[2].op.event3 = evt3;

	int seqNumber = 0;

	SECTION("Send and receive")
	{
		// send and check
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
				REQUIRE_FALSE(gError);
			}
		}

		ringBuffer.stop();
	}
}
