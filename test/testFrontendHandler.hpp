/*
 *  Test Frontend Handler
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

#ifndef TEST_TESTFRONTENDHANDLER_HPP_
#define TEST_TESTFRONTENDHANDLER_HPP_

#include "FrontendHandlerBase.hpp"

class TestFrontendHandler : public XenBackend::FrontendHandlerBase
{
public:

	TestFrontendHandler(const std::string& devName,
						domid_t beDomId, domid_t feDomId, uint16_t devId) :
		XenBackend::FrontendHandlerBase("TestFrontend", devName,
										beDomId, feDomId, devId)
	{}

	static void prepareXenStore(const std::string& devName,
								domid_t beDomId, domid_t feDomId,
								uint16_t devId);

private:

	void onBind() override;
	void onClosing() override;
};


#endif /* TEST_TESTFRONTENDHANDLER_HPP_ */
