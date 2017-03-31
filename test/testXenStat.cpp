/*
 *  Test XenStat
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

#include <catch.hpp>

#include "mocks/XenCtrlMock.hpp"
#include "XenStat.hpp"

using XenBackend::XenStat;

TEST_CASE("XenStat", "[xenctrl]")
{
	XenStat xenStat;

	XenCtrlMock::setErrorMode(false);

	auto mock = XenCtrlMock::getLastInstance();

	xc_domaininfo_t info = {};

	domid_t domIds[] = { 1, 2, 3, 4, 5, 6, 7, 8 };

	for (int i = 0; i < 8; i++)
	{
		info.domain = domIds[i];

		if (i >= 5)
		{
			info.flags = XEN_DOMINF_running;
		}

		mock->addDomInfo(info);
	}

	SECTION("Check getters")
	{
		auto existDoms = xenStat.getExistingDoms();
		auto runDoms = xenStat.getRunningDoms();

		REQUIRE(existDoms.size() == 8);
		REQUIRE(runDoms.size() == 3);

		for (size_t i = 0; i < existDoms.size(); i++)
		{
			REQUIRE(existDoms[i] == domIds[i]);
		}

		for (size_t i = 0; i < runDoms.size(); i++)
		{
			REQUIRE(runDoms[i] == domIds[i + 5]);
		}
	}

	SECTION("Check errors")
	{
		XenCtrlMock::setErrorMode(true);

		REQUIRE_THROWS(xenStat.getExistingDoms());
	}
}

TEST_CASE("XenStatError", "[xenctrl]")
{
	XenCtrlMock::setErrorMode(true);

	REQUIRE_THROWS(XenStat());
}
