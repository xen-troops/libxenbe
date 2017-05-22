/*
 *  Test XenGnttab
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

#include "mocks/XenGnttabMock.hpp"
#include "XenGnttab.hpp"

using XenBackend::XenGnttabBuffer;

TEST_CASE("XenGnttab", "[xengnttab]")
{
	XenGnttabMock::setErrorMode(false);

	SECTION("Check one page")
	{
		XenGnttabBuffer xenBuffer(3, 14);

		auto mock = XenGnttabMock::getLastInstance();

		REQUIRE(xenBuffer.size() == mock->getMapBufferSize(xenBuffer.get()));
	}

	SECTION("Check multiple pages")
	{
		size_t count = 5;
		grant_ref_t refs[count] = { 1, 2, 3, 4, 5 };

		XenGnttabBuffer  xenBuffer(3, refs, count);

		auto mock = XenGnttabMock::getLastInstance();

		REQUIRE(xenBuffer.size() == mock->getMapBufferSize(xenBuffer.get()));
	}

	SECTION("Check errors")
	{
		XenGnttabMock::setErrorMode(true);

		REQUIRE_THROWS(XenGnttabBuffer(3, 14));
	}
}
