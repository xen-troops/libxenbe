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

#ifndef TEST_MOCKS_XENCTRLMOCK_HPP_
#define TEST_MOCKS_XENCTRLMOCK_HPP_

#include <list>

extern "C" {
#include <xenctrl.h>
}

class XenCtrlMock
{
public:

	XenCtrlMock();

	static XenCtrlMock* getLastInstance() { return sLastInstance; }

	void addDomInfo(const xc_domaininfo_t& info);
	int getDomInfos(domid_t firstDom, unsigned int maxDoms,
					xc_domaininfo_t* info);
private:

	static XenCtrlMock* sLastInstance;

	std::list<xc_domaininfo_t> mDomInfos;
};

#endif /* TEST_MOCKS_XENCTRLMOCK_HPP_ */
