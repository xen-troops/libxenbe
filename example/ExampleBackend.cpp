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

#include "ExampleBackend.hpp"

#include <csignal>

using XenBackend::FrontendHandlerPtr;
using XenBackend::RingBufferPtr;

//! [processRequest]
void ExampleInRingBuffer::processRequest(const xentest_req& req)
{
	LOG(mLog, DEBUG) << "Receive request, id: " << req.id;

	xentest_rsp rsp;

	rsp.seq = req.seq;
	rsp.status = 0;

	// process commands

	switch(req.id)
	{
		case XENTEST_CMD1:

			// process CMD1: req.op.command1

		break;

		case XENTEST_CMD2:

			// process CMD2: req.op.command2

		break;

		case XENTEST_CMD3:

			// process CMD3: req.op.command3

		break;

		default:

			// set error status
			rsp.status = 1;

		break;
	}

	// send response
	sendResponse(rsp);
}
//! [processRequest]

//! [onBind]
void ExampleFrontendHandler::onBind()
{
	LOG(mLog, DEBUG) << "Bind, dom id: " << getDomId();

	// get out ring buffer event channel port
	evtchn_port_t port = getXenStore().readInt(getXsFrontendPath() +
											   "/path/to/out/port");
	// get out ring buffer grant table reference
	uint32_t ref = getXenStore().readInt(getXsFrontendPath() +
										 "/path/to/out/ref");
	// create out ring buffer
	mOutRingBuffer.reset(new ExampleOutRingBuffer(getDomId(), port, ref));
	// add ring buffer
	addRingBuffer(mOutRingBuffer);

	// get in ring buffer event channel port
	port = getXenStore().readInt(getXsFrontendPath() +
								 "/path/to/in/port");
	// get in ring buffer grant table reference
	ref = getXenStore().readInt(getXsFrontendPath() +
								"/path/to/in/ref");
	// create in ring buffer
	RingBufferPtr outRingBuffer(
			new ExampleOutRingBuffer(getDomId(), port, ref));
	// add ring buffer
	addRingBuffer(outRingBuffer);
}
//! [onBind]

//! [onClosing]
void ExampleFrontendHandler::onClosing()
{
	// free allocate on bind resources
	mOutRingBuffer.reset();
}

//! [onSomeEvent]
void ExampleFrontendHandler::onSomeEvent()
{
	if (mOutRingBuffer)
	{
		xentest_evt evt = {};

		// fill and send evt

		LOG(mLog, DEBUG) << "Send event, id: " << evt.id;

		mOutRingBuffer->sendEvent(evt);
	}
}
//! [onSomeEvent]

//! [onNewFrontend]
void ExampleBackend::onNewFrontend(domid_t domId, uint16_t devId)
{
	LOG(mLog, DEBUG) << "New frontend, dom id: " << domId;

	// create new example frontend handler
	addFrontendHandler(FrontendHandlerPtr(
			new ExampleFrontendHandler(getDeviceName(), domId)));
}
//! [onNewFrontend]

void waitSignals()
{
	sigset_t set;
	int signal;

	sigemptyset(&set);
	sigaddset(&set, SIGINT);
	sigaddset(&set, SIGTERM);
	sigprocmask(SIG_BLOCK, &set, nullptr);

	sigwait(&set,&signal);
}

//! [main]
int main(int argc, char *argv[])
{
	try
	{
		// Create backend
		ExampleBackend exampleBackend;

		exampleBackend.start();

		waitSignals();

		exampleBackend.stop();
	}
	catch(const std::exception& e)
	{
		LOG("Main", ERROR) << e.what();
	}

	return 0;
}
//! [main]
