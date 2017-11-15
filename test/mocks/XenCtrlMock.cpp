/*
 *  XenCtrlMock
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

#include "XenCtrlMock.hpp"

#include <algorithm>

#include <cstdlib>

using std::find_if;
using std::list;
using std::lock_guard;
using std::mutex;

/*******************************************************************************
 * Xen interface
 ******************************************************************************/

struct xc_interface_core
{
	XenCtrlMock* mock;
};

xc_interface* xc_interface_open(xentoollog_logger* logger,
								xentoollog_logger* dombuild_logger,
								unsigned open_flags)
{
	xc_interface* xch = nullptr;

	if (!XenCtrlMock::getErrorMode())
	{
		xch = static_cast<xc_interface*>(malloc(sizeof(xc_interface)));

		xch->mock = new XenCtrlMock();
	}

	return xch;
}

int xc_interface_close(xc_interface* xch)
{
	delete xch->mock;

	free(xch);

	if (XenCtrlMock::getErrorMode())
	{
		return -1;
	}

	return 0;
}

int xc_domain_getinfolist(xc_interface* xch,
						  uint32_t first_domain,
						  unsigned int max_domains,
						  xc_domaininfo_t* info)
{
	if (XenCtrlMock::getErrorMode())
	{
		return -1;
	}

	return xch->mock->getDomInfos(first_domain, max_domains, info);
}

/*******************************************************************************
 * XenCtrlMock
 ******************************************************************************/

mutex XenCtrlMock::sMutex;
bool XenCtrlMock::sErrorMode = false;
list<xc_domaininfo_t> XenCtrlMock::sDomInfos;

/*******************************************************************************
 * Public
 ******************************************************************************/

void XenCtrlMock::addDomInfo(const xc_domaininfo_t& info)
{
	lock_guard<mutex> lock(sMutex);

	auto it = find_if(sDomInfos.begin(), sDomInfos.end(),
					 [&info](const xc_domaininfo_t& item)
					 { return item.domain == info.domain; });

	if (it != sDomInfos.end())
	{
		*it = info;
	}

	sDomInfos.push_back(info);
}

int XenCtrlMock::getDomInfos(domid_t firstDom, unsigned int maxDoms,
							 xc_domaininfo_t* info)
{
	lock_guard<mutex> lock(sMutex);

	unsigned int count = 0;

	auto it = find_if(sDomInfos.begin(), sDomInfos.end(),
					 [&firstDom](const xc_domaininfo_t& item)
					 { return item.domain == firstDom; });

	if (it == sDomInfos.end())
	{
		it = sDomInfos.begin();
	}

	for(; it != sDomInfos.end(); it++)
	{
		info[count++] = *it;

		if (count == maxDoms)
		{
			break;
		}
	}

	return count;
}
