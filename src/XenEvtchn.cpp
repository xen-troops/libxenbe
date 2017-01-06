/*
 *  Xen evtchn wrapper
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

#include "XenEvtchn.hpp"

#include <poll.h>

using std::exception;
using std::lock_guard;
using std::mutex;
using std::thread;
using std::to_string;

namespace XenBackend {

/*******************************************************************************
 * XenEvtchn
 ******************************************************************************/

XenEvtchn::XenEvtchn(domid_t domId, evtchn_port_t port, Callback callback,
					 ErrorCallback errorCallback) :
	mPort(-1),
	mCallback(callback),
	mErrorCallback(errorCallback),
	mTerminate(false),
	mTerminated(true),
	mLog("XenEvtchn")
{
	try
	{
		init(domId, port);
	}
	catch(const XenException& e)
	{
		release();

		throw;
	}
}

XenEvtchn::~XenEvtchn()
{
	stop();
	release();
}

/*******************************************************************************
 * Public
 ******************************************************************************/

void XenEvtchn::start()
{
	DLOG(mLog, DEBUG) << "Start event channel, port: " << mPort;

	if (!mTerminated)
	{
		throw XenEvtchnException("Event channel is already started");
	}

	mTerminate = false;
	mTerminated = false;

	mThread = thread(&XenEvtchn::eventThread, this);
}

void XenEvtchn::stop()
{
	DLOG(mLog, DEBUG) << "Stop event channel, port: " << mPort;

	mTerminate = true;

	if (mThread.joinable())
	{
		mThread.join();
	}
}

void XenEvtchn::notify()
{
	DLOG(mLog, DEBUG) << "Notify event channel, port: " << mPort;

	if (xenevtchn_notify(mHandle, mPort) < 0)
	{
		throw XenEvtchnException("Can't notify event channel");
	}
}

/*******************************************************************************
 * Private
 ******************************************************************************/

void XenEvtchn::init(domid_t domId, evtchn_port_t port)
{
	mHandle = xenevtchn_open(nullptr, 0);

	if (!mHandle)
	{
		throw XenEvtchnException("Can't open event channel");
	}

	mPort = xenevtchn_bind_interdomain(mHandle, domId, port);

	if (mPort == -1)
	{
		throw XenEvtchnException("Can't bind event channel: " + to_string(port));
	}

	DLOG(mLog, DEBUG) << "Create event channel, dom: " << domId
					  << ", remote port: " << port << ", local port: "
					  << mPort;
}

void XenEvtchn::release()
{
	if (mPort != -1)
	{
		xenevtchn_unbind(mHandle, mPort);
	}

	if (mHandle)
	{
		xenevtchn_close(mHandle);

		DLOG(mLog, DEBUG) << "Delete event channel, port: " << mPort;
	}
}

void XenEvtchn::eventThread()
{
	try
	{
		while(!mTerminate)
		{
			if (waitEvent() && mCallback)
			{
				mCallback();
			}
		}
	}
	catch(const exception& e)
	{
		if (mErrorCallback)
		{
			mErrorCallback(e);
		}
		else
		{
			LOG(mLog, ERROR) << e.what();
		}
	}

	mTerminated = true;
}

bool XenEvtchn::waitEvent()
{
	pollfd fds;

	fds.fd = xenevtchn_fd(mHandle);
	fds.events = POLLIN;

	auto ret = poll(&fds, 1, cPoolEventTimeoutMs);

	if (ret < 0)
	{
		throw XenEvtchnException("Can't poll watches");
	}

	if (ret > 0)
	{
		auto port = xenevtchn_pending(mHandle);

		if (port < 0)
		{
			throw XenEvtchnException("Can't get pending port");
		}

		if (xenevtchn_unmask(mHandle, port) < 0)
		{
			throw XenEvtchnException("Can't unmask event channel");
		}

		if (port != mPort)
		{
			throw XenEvtchnException("Error port number: " +
									 to_string(port) + ", expected: " +
									 to_string(mPort));
		}

		DLOG(mLog, DEBUG) << "Event received, port: " << mPort;

		return true;
	}

	return false;
}

}
