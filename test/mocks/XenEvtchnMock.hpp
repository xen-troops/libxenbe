/*
 *  XenEvtchnMock
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

#ifndef TEST_MOCKS_XENEVTCHNMOCK_HPP_
#define TEST_MOCKS_XENEVTCHNMOCK_HPP_

#include <list>

extern "C" {
#include <xenevtchn.h>
}

#include "Pipe.hpp"

class XenEvtchnMock
{
public:

	XenEvtchnMock();

	static XenEvtchnMock* getLastInstance() { return sLastInstance; }

	int getFd() const { return mPipe.getFd(); }
	evtchn_port_t getLastNotifiedPort() const { return mLastNotifiedPort; }
	evtchn_port_t bind(domid_t domId, evtchn_port_t remotePort);
	void unbind(evtchn_port_t port);
	void notifyPort(evtchn_port_t port);
	void signalPort(evtchn_port_t port);
	evtchn_port_t getPendingPort();

private:

	static XenEvtchnMock* sLastInstance;
	static evtchn_port_t sPort;

	struct BoundPort
	{
		domid_t domId;
		evtchn_port_t remotePort;
		evtchn_port_t localPort;
	};

	Pipe mPipe;

	std::list<BoundPort> mBoundPorts;
	std::list<evtchn_port_t> mSignaledPorts;

	int mLastNotifiedPort;

	std::list<BoundPort>::iterator getBoundPort(evtchn_port_t port);
};

#endif /* TEST_MOCKS_XENEVTCHNMOCK_HPP_ */
