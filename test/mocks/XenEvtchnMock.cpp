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

#include "Exception.hpp"

using std::find_if;
using std::list;
using std::lock_guard;
using std::mutex;

using XenBackend::Exception;

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
	xenevtchn_handle* xce = nullptr;

	if (!XenEvtchnMock::getErrorMode())
	{
		xce = static_cast<xenevtchn_handle*>(
				malloc(sizeof(xenevtchn_handle)));

		xce->mock = new XenEvtchnMock();
	}

	return xce;
}

int xenevtchn_close(xenevtchn_handle* xce)
{
	delete xce->mock;

	free(xce);

	if (XenEvtchnMock::getErrorMode())
	{
		return -1;
	}

	return 0;
}

xenevtchn_port_or_error_t
xenevtchn_bind_interdomain(xenevtchn_handle* xce, uint32_t domid,
						   evtchn_port_t remote_port)
{
	if (XenEvtchnMock::getErrorMode())
	{
		return -1;
	}

	return xce->mock->bind(domid, remote_port);
}

int xenevtchn_unbind(xenevtchn_handle* xce, evtchn_port_t port)
{
	if (XenEvtchnMock::getErrorMode())
	{
		return -1;
	}

	xce->mock->unbind(port);

	return 0;
}

int xenevtchn_notify(xenevtchn_handle* xce, evtchn_port_t port)
{
	if (XenEvtchnMock::getErrorMode())
	{
		return -1;
	}

	xce->mock->notifyPort(port);

	return 0;
}

int xenevtchn_fd(xenevtchn_handle* xce)
{
	if (XenEvtchnMock::getErrorMode())
	{
		return -1;
	}

	return xce->mock->getFd();
}

int xenevtchn_unmask(xenevtchn_handle* xce, evtchn_port_t port)
{
	if (XenEvtchnMock::getErrorMode())
	{
		return -1;
	}

	return 0;
}

xenevtchn_port_or_error_t xenevtchn_pending(xenevtchn_handle* xce)
{
	if (XenEvtchnMock::getErrorMode())
	{
		return -1;
	}

	return xce->mock->getPendingPort();
}

/*******************************************************************************
 * XenEvtchnMock
 ******************************************************************************/

mutex XenEvtchnMock::sMutex;
list<XenEvtchnMock*> XenEvtchnMock::sClients;
evtchn_port_t XenEvtchnMock::sPort = 0;
bool XenEvtchnMock::sErrorMode = false;
int XenEvtchnMock::sLastNotifiedPort = -1;
int XenEvtchnMock::sLastBoundPort = -1;

XenEvtchnMock::XenEvtchnMock()
{
	sClients.push_back(this);
}

XenEvtchnMock::~XenEvtchnMock()
{
	sClients.remove(this);
}

/*******************************************************************************
 * Public
 ******************************************************************************/

void XenEvtchnMock::signalPort(evtchn_port_t port)
{
	lock_guard<mutex> lock(sMutex);

	auto client = getClientByPort(port);

	client->mSignaledPorts.push_back(port);

	client->mPipe.write();
}

void XenEvtchnMock::setNotifyCbk(evtchn_port_t port, NotifyCbk cbk)
{
	getClientByPort(port)->mNotifyCbk = cbk;
}

evtchn_port_t XenEvtchnMock::bind(domid_t domId, evtchn_port_t remotePort)
{
	lock_guard<mutex> lock(sMutex);

	auto it = find_if(mBoundPorts.begin(), mBoundPorts.end(),
					  [&remotePort, &domId](const BoundPort& boundPort)
					  { return (boundPort.remotePort == remotePort) &&
							   (boundPort.domId == domId); });

	if (it != mBoundPorts.end())
	{
		throw Exception("Port already bound", EPERM);
	}

	BoundPort boundPort = { domId, remotePort, sPort++ };

	mBoundPorts.push_back(boundPort);

	sLastBoundPort = boundPort.localPort;

	return boundPort.localPort;
}

void XenEvtchnMock::unbind(evtchn_port_t port)
{
	lock_guard<mutex> lock(sMutex);

	auto it = getBoundPort(port);

	if (it == mBoundPorts.end())
	{
		throw Exception("Port not bound", EINVAL);
	}

	mBoundPorts.erase(it);
}

void XenEvtchnMock::notifyPort(evtchn_port_t port)
{
	lock_guard<mutex> lock(sMutex);

	sLastNotifiedPort = port;

	if (mNotifyCbk)
	{
		mNotifyCbk();
	}
}

evtchn_port_t XenEvtchnMock::getPendingPort()
{
	lock_guard<mutex> lock(sMutex);

	if (mSignaledPorts.size() == 0)
	{
		throw Exception("No pending ports", ENOENT);
	}

	evtchn_port_t port = mSignaledPorts.front();

	mSignaledPorts.pop_front();

	mPipe.read();

	return port;
}

/*******************************************************************************
 * Private

 ******************************************************************************/
XenEvtchnMock* XenEvtchnMock::getClientByPort(evtchn_port_t port)
{
	for(auto client : sClients)
	{
		if (client->getBoundPort(port) != client->mBoundPorts.end())
		{
			return client;
		}
	}

	throw Exception("Port not bound", ENOENT);
}

list<XenEvtchnMock::BoundPort>::iterator
XenEvtchnMock::getBoundPort(evtchn_port_t port)
{
	return find_if(mBoundPorts.begin(), mBoundPorts.end(),
				  [&port](const BoundPort& boundPort)
				  { return boundPort.localPort == port; });
}
