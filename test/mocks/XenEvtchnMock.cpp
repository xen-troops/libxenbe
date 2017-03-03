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

#include "XenEvtchnMock.hpp"

#include <algorithm>

#include <cstdlib>

#include "XenException.hpp"

using std::find_if;
using std::list;

using XenBackend::XenException;

/*******************************************************************************
 * Xen interface
 ******************************************************************************/

struct xenevtchn_handle
{
	XenEvtchnMock* mock;
};

xenevtchn_handle* xenevtchn_open(struct xentoollog_logger* logger,
								 unsigned open_flags)
{
	xenevtchn_handle* xce = static_cast<xenevtchn_handle*>(
			malloc(sizeof(xenevtchn_handle)));

	xce->mock = new XenEvtchnMock();

	return xce;
}

int xenevtchn_close(xenevtchn_handle* xce)
{
	delete xce->mock;

	free(xce);

	return 0;
}

xenevtchn_port_or_error_t
xenevtchn_bind_interdomain(xenevtchn_handle* xce, uint32_t domid,
						   evtchn_port_t remote_port)
{
	return xce->mock->bind(domid, remote_port);
}

int xenevtchn_unbind(xenevtchn_handle* xce, evtchn_port_t port)
{
	xce->mock->unbind(port);

	return 0;
}

int xenevtchn_notify(xenevtchn_handle* xce, evtchn_port_t port)
{
	xce->mock->notifyPort(port);

	return 0;
}

int xenevtchn_fd(xenevtchn_handle* xce)
{
	return xce->mock->getFd();
}

int xenevtchn_unmask(xenevtchn_handle* xce, evtchn_port_t port)
{
	return 0;
}

xenevtchn_port_or_error_t xenevtchn_pending(xenevtchn_handle* xce)
{
	return xce->mock->getPendingPort();
}

/*******************************************************************************
 * XenEvtchnMock
 ******************************************************************************/

XenEvtchnMock* XenEvtchnMock::sLastInstance = nullptr;
evtchn_port_t XenEvtchnMock::sPort = 0;

XenEvtchnMock::XenEvtchnMock() :
	mLastNotifiedPort(-1),
	mLastBoundPort(-1)
{
	sLastInstance = this;
}

/*******************************************************************************
 * Public
 ******************************************************************************/

evtchn_port_t XenEvtchnMock::bind(domid_t domId, evtchn_port_t remotePort)
{
	auto it = find_if(mBoundPorts.begin(), mBoundPorts.end(),
					  [&remotePort, &domId](const BoundPort& boundPort)
					  { return (boundPort.remotePort == remotePort) &&
							   (boundPort.domId == domId); });

	if (it != mBoundPorts.end())
	{
		throw XenException("Port already bound");
	}

	BoundPort boundPort = { domId, remotePort, sPort++ };

	mBoundPorts.push_back(boundPort);

	mLastBoundPort = boundPort.localPort;

	return boundPort.localPort;
}

void XenEvtchnMock::unbind(evtchn_port_t port)
{
	mBoundPorts.erase(getBoundPort(port));
}

void XenEvtchnMock::notifyPort(evtchn_port_t port)
{
	getBoundPort(port);

	mLastNotifiedPort = port;

	if (mNotifyCbk)
	{
		mNotifyCbk();
	}
}

void XenEvtchnMock::signalPort(evtchn_port_t port)
{
	getBoundPort(port);

	mSignaledPorts.push_back(port);

	mPipe.write();
}

void XenEvtchnMock::signalLastBoundPort()
{
	signalPort(mLastBoundPort);
}

evtchn_port_t XenEvtchnMock::getPendingPort()
{
	if (mSignaledPorts.size() == 0)
	{
		throw XenException("No pending ports");
	}

	evtchn_port_t port = mSignaledPorts.front();

	mSignaledPorts.pop_front();

	mPipe.read();

	return port;
}

void XenEvtchnMock::setNotifyCbk(NotifyCbk cbk)
{
	mNotifyCbk = cbk;
}

/*******************************************************************************
 * Private
 ******************************************************************************/

list<XenEvtchnMock::BoundPort>::iterator
XenEvtchnMock::getBoundPort(evtchn_port_t port)
{
	auto it = find_if(mBoundPorts.begin(), mBoundPorts.end(),
					  [&port](const BoundPort& boundPort)
					  { return boundPort.localPort == port; });

	if (it == mBoundPorts.end())
	{
		throw XenException("Port not bound");
	}

	return it;
}
