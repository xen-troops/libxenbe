/*
 *  Xen Ctrl wrapper
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

#include "XenCtrl.hpp"

using std::vector;

namespace XenBackend {

/*******************************************************************************
 * XenInterface
 ******************************************************************************/

XenInterface::XenInterface() :
	mLog("XenInterface")
{
	try
	{
		init();
	}
	catch(const std::exception& e)
	{
		release();

		throw;
	}
}

XenInterface::~XenInterface()
{
	release();
}

/*******************************************************************************
 * Public
 ******************************************************************************/

void XenInterface::getDomainsInfo(vector<xc_domaininfo_t>& infos)
{
	vector<xc_domaininfo_t> domainInfo(cDomInfoChunkSize);

	int newDomains = cDomInfoChunkSize;
	int startDomain = 0;

	infos.clear();

	while(newDomains == cDomInfoChunkSize)
	{
		newDomains = xc_domain_getinfolist(mHandle, startDomain,
										   cDomInfoChunkSize,
										   domainInfo.data());

		if (newDomains < 0)
		{
			throw XenCtrlException("Can't get domain info", errno);
		}

		if (newDomains)
		{
			startDomain = domainInfo[newDomains - 1].domain + 1;
		}

		for(auto i = 0; i < newDomains; i++)
		{
			infos.push_back(domainInfo[i]);
		}
	}
}

/*******************************************************************************
 * Private
 ******************************************************************************/

void XenInterface::init()
{
	mHandle = xc_interface_open(0,0,0);

	if (!mHandle)
	{
		throw XenCtrlException("Can't open xc interface", EINVAL);
	}

	DLOG(mLog, DEBUG) << "Create xen interface";
}

void XenInterface::release()
{
	if (mHandle)
	{
		xc_interface_close(mHandle);

		DLOG(mLog, DEBUG) << "Delete xen interface";
	}
}

}
