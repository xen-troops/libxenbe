/*
 *  Xen base ring buffer
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

#include "RingBufferBase.hpp"

#include "Log.hpp"

using std::bind;

namespace XenBackend {

/*******************************************************************************
 * RingBufferBase
 ******************************************************************************/

RingBufferBase::RingBufferBase(domid_t domId, evtchn_port_t port,
							   grant_ref_t ref) :
	mEventChannel(domId, port, [this] { onReceiveIndication(); }),
	mBuffer(domId, ref, PROT_READ | PROT_WRITE),
	mPort(port),
	mRef(ref),
	mLog("RingBuffer")
{
	LOG(mLog, DEBUG) << "Create ring buffer, port: " << mPort
					 << ", ref: " << mRef;
}

RingBufferBase::~RingBufferBase()
{
	stop();

	LOG(mLog, DEBUG) << "Delete ring buffer, port: " << mPort
					 << ", ref: " << mRef;
}

/*******************************************************************************
 * Public
 ******************************************************************************/

void RingBufferBase::start()
{
	mEventChannel.start();
}

void RingBufferBase::stop()
{
	mEventChannel.stop();
}

void RingBufferBase::setErrorCallback(ErrorCallback errorCallback)
{
	mEventChannel.setErrorCallback(errorCallback);
}

}
