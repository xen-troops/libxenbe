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

#ifndef TESTS_MOCKS_XENCTRLMOCK_HPP_
#define TESTS_MOCKS_XENCTRLMOCK_HPP_

#include <list>
#include <mutex>

extern "C" {
#include <xenctrl.h>
}

class XenCtrlMock
{
public:

	static void setErrorMode(bool errorMode)
	{
		std::lock_guard<std::mutex> lock(sMutex);

		sErrorMode = errorMode;
	}
	static bool getErrorMode()
	{
		std::lock_guard<std::mutex> lock(sMutex);

		return sErrorMode;
	}

	static void addDomInfo(const xc_domaininfo_t& info);
	static int getDomInfos(domid_t firstDom, unsigned int maxDoms,
						   xc_domaininfo_t* info);

private:

	static std::mutex sMutex;
	static bool sErrorMode;
	static std::list<xc_domaininfo_t> sDomInfos;
};

#endif /* TESTS_MOCKS_XENCTRLMOCK_HPP_ */
