/*
 *  XenGnttabMock
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

#include "XenGnttabMock.hpp"

#include <cstdlib>

extern "C" {
#include <xenctrl.h>
#include <xengnttab.h>
}

#include "XenException.hpp"

using XenBackend::XenException;

/*******************************************************************************
 * Xen interface
 ******************************************************************************/

struct xengntdev_handle
{
	XenGnttabMock* mock;
};

xengnttab_handle* xengnttab_open(xentoollog_logger* logger,
								 unsigned open_flags)
{
	xengnttab_handle* xgt = static_cast<xengnttab_handle*>(
			malloc(sizeof(xengnttab_handle)));

	xgt->mock = new XenGnttabMock();

	return xgt;
}

int xengnttab_close(xengnttab_handle* xgt)
{
	xgt->mock->checkMapBuffers();

	delete xgt->mock;

	free(xgt);

	return 0;
}

void* xengnttab_map_domain_grant_refs(xengnttab_handle* xgt,
									  uint32_t count,
									  uint32_t domid,
									  uint32_t *refs,
									  int prot)
{
	return xgt->mock->mapGrantRefs(count, domid, refs);
}

int xengnttab_unmap(xengnttab_handle* xgt, void* start_address, uint32_t count)
{
	xgt->mock->unmapGrantRefs(start_address, count);

	return 0;
}

/*******************************************************************************
 * XenGnttabMock
 ******************************************************************************/

XenGnttabMock* XenGnttabMock::sLastInstance = nullptr;

XenGnttabMock::XenGnttabMock() :
	mLastMappedAddress(nullptr)
{
	sLastInstance = this;
}

/*******************************************************************************
 * Public
 ******************************************************************************/

void* XenGnttabMock::mapGrantRefs(uint32_t count, uint32_t domId,
								  uint32_t *refs)
{
	MapBuffer buffer = { count, domId, count * XC_PAGE_SIZE };

	void* address = malloc(buffer.size);

	mMapBuffers[address] = buffer;

	mLastMappedAddress = address;

	return address;
}

void XenGnttabMock::unmapGrantRefs(void* address, uint32_t count)
{
	auto it = mMapBuffers.find(address);

	if (it == mMapBuffers.end())
	{
		throw XenException("Buffer not found");
	}

	if (count != it->second.count)
	{
		throw XenException("Wrong cound");
	}

	free(it->first);

	mMapBuffers.erase(it);
}

size_t XenGnttabMock::getMapBufferSize(void* address)
{
	auto it = mMapBuffers.find(address);

	if (it == mMapBuffers.end())
	{
		throw XenException("Buffer not found");
	}

	return it->second.size;
}

void XenGnttabMock::checkMapBuffers()
{
	if (mMapBuffers.size())
	{
		throw XenException("Not all buffers are unmapped");
	}
}
