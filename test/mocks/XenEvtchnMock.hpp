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

#include <functional>
#include <list>
#include <mutex>

extern "C" {
#include <xenctrl.h>
#include <xenevtchn.h>
}

#include "Pipe.hpp"

class XenEvtchnMock
{
public:

	typedef std::function<void()> NotifyCbk;

	XenEvtchnMock();
	~XenEvtchnMock();

	static void setErrorMode(bool errorMode)
	{
		std::lock_guard<std::mutex> lock(sMutex);

		sErrorMode = errorMode;
	}
	static bool getErrorMode()
	{
		std::lock_guard<std::mutex> lock(sMutex);

		return sErrorMode;
	}
	static evtchn_port_t getLastNotifiedPort()
	{
		std::lock_guard<std::mutex> lock(sMutex);

		return sLastNotifiedPort;
	}
	static evtchn_port_t getLastBoundPort()
	{
		std::lock_guard<std::mutex> lock(sMutex);

		return sLastBoundPort;
	}
	static void signalPort(evtchn_port_t port);
	static void setNotifyCbk(evtchn_port_t port, NotifyCbk cbk);

	int getFd() const { return mPipe.getFd(); }
	evtchn_port_t bind(domid_t domId, evtchn_port_t remotePort);
	void unbind(evtchn_port_t port);
	void notifyPort(evtchn_port_t port);
	evtchn_port_t getPendingPort();

private:

	struct BoundPort
	{
		domid_t domId;
		evtchn_port_t remotePort;
		evtchn_port_t localPort;
	};

	static std::mutex sMutex;
	static evtchn_port_t sPort;
	static bool sErrorMode;
	static std::list<XenEvtchnMock*> sClients;
	static int sLastNotifiedPort;
	static int sLastBoundPort;

	Pipe mPipe;

	std::list<evtchn_port_t> mSignaledPorts;
	std::list<BoundPort> mBoundPorts;

	NotifyCbk mNotifyCbk;

	static XenEvtchnMock* getClientByPort(evtchn_port_t port);
	std::list<BoundPort>::iterator getBoundPort(evtchn_port_t port);
};

#endif /* TEST_MOCKS_XENEVTCHNMOCK_HPP_ */
