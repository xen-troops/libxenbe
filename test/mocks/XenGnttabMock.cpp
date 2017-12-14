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

#include "Exception.hpp"

using std::lock_guard;
using std::mutex;
using std::unordered_map;

using XenBackend::Exception;

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
	xengnttab_handle* xgt = nullptr;

	if (!XenGnttabMock::getErrorMode())
	{
		xgt = static_cast<xengnttab_handle*>(malloc(sizeof(xengnttab_handle)));

		xgt->mock = new XenGnttabMock();
	}

	return xgt;
}

int xengnttab_close(xengnttab_handle* xgt)
{
	xgt->mock->checkMapBuffers();

	delete xgt->mock;

	free(xgt);

	if (XenGnttabMock::getErrorMode())
	{
		return -1;
	}

	return 0;
}

void* xengnttab_map_domain_grant_refs(xengnttab_handle* xgt,
									  uint32_t count,
									  uint32_t domid,
									  uint32_t *refs,
									  int prot)
{
	if (XenGnttabMock::getErrorMode())
	{
		return nullptr;
	}

	return xgt->mock->mapGrantRefs(count, domid, refs);
}

int xengnttab_unmap(xengnttab_handle* xgt, void* start_address, uint32_t count)
{
	if (XenGnttabMock::getErrorMode())
	{
		return -1;
	}

	xgt->mock->unmapGrantRefs(start_address, count);

	return 0;
}

/*******************************************************************************
 * XenGnttabMock
 ******************************************************************************/

mutex XenGnttabMock::sMutex;
void* XenGnttabMock::sLastMappedAddress = nullptr;
unordered_map<void*, XenGnttabMock::MapBuffer> XenGnttabMock::sMapBuffers;
bool XenGnttabMock::sErrorMode = false;

/*******************************************************************************
 * Public
 ******************************************************************************/

void* XenGnttabMock::mapGrantRefs(uint32_t count, uint32_t domId,
								  uint32_t *refs)
{
	lock_guard<mutex> lock(sMutex);

	MapBuffer buffer = { count, domId, count * XC_PAGE_SIZE };

	void* address = calloc(1, buffer.size);

	sMapBuffers[address] = buffer;

	sLastMappedAddress = address;

	return address;
}

void XenGnttabMock::unmapGrantRefs(void* address, uint32_t count)
{
	lock_guard<mutex> lock(sMutex);

	auto it = sMapBuffers.find(address);

	if (it == sMapBuffers.end())
	{
		throw Exception("Buffer not found", ENOENT);
	}

	if (count != it->second.count)
	{
		throw Exception("Wrong count", EINVAL);
	}

	free(it->first);

	sMapBuffers.erase(it);
}

size_t XenGnttabMock::getMapBufferSize(void* address)
{
	lock_guard<mutex> lock(sMutex);

	auto it = sMapBuffers.find(address);

	if (it == sMapBuffers.end())
	{
		throw Exception("Buffer not found", ENOENT);
	}

	return it->second.size;
}

size_t XenGnttabMock::checkMapBuffers()
{
	lock_guard<mutex> lock(sMutex);

	return sMapBuffers.size();
}
