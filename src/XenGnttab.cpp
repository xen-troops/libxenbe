/*
 *  Xen gnttab wrapper
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

#include "XenGnttab.hpp"

namespace XenBackend {

/*******************************************************************************
 * XenGnttab
 ******************************************************************************/

XenGnttab::XenGnttab()
{
	mHandle = xengnttab_open(nullptr, 0);

	if (!mHandle)
	{
		throw XenGnttabException("Can't open xc grant table");
	}
}

XenGnttab::~XenGnttab()
{
	if (mHandle)
	{
		xengnttab_close(mHandle);
	}
}

/*******************************************************************************
 * XenGnttabBuffer
 ******************************************************************************/

XenGnttabBuffer::XenGnttabBuffer(int domId, uint32_t ref, int prot) :
		XenGnttabBuffer(domId, &ref, 1, prot)
{

}

XenGnttabBuffer::XenGnttabBuffer(int domId, const uint32_t* refs, size_t count,
								 int prot) :
	mDomId(domId),
	mLog("XenGnttabBuffer")
{
	init(refs, count, prot);
}

XenGnttabBuffer::~XenGnttabBuffer()
{
	release();
}

/*******************************************************************************
 * Private
 ******************************************************************************/

void XenGnttabBuffer::init(const uint32_t* refs, size_t count, int prot)
{
	static XenGnttab gnttab;

	mHandle = gnttab.getHandle();
	mBuffer = nullptr;
	mCount = count;

	DLOG(mLog, DEBUG) << "Create grant table buffer, dom: " << mDomId
					  << ", count: " << count << ", ref: " << *refs;


	mBuffer = xengnttab_map_domain_grant_refs(mHandle, count, mDomId,
											  const_cast<uint32_t*>(refs),
											  PROT_READ | PROT_WRITE);

	if (!mBuffer)
	{
		throw XenGnttabException("Can't map buffer");
	}

}

void XenGnttabBuffer::release()
{
	DLOG(mLog, DEBUG) << "Delete grant table buffer, dom: " << mDomId;

	if (mBuffer)
	{
		xengnttab_unmap(mHandle, mBuffer, mCount);
	}
}

}

