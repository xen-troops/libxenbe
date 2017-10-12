/*
 *  Test Backend
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

#ifndef TEST_TESTBACKEND_HPP_
#define TEST_TESTBACKEND_HPP_

#include "BackendBase.hpp"

class TestBackend : public XenBackend::BackendBase
{
public:

	TestBackend(const std::string& devName) :
		XenBackend::BackendBase("TestBackend", devName)
	{}

private:

	void onNewFrontend(domid_t domId, uint16_t devId) override;
};

#endif /* TEST_TESTBACKEND_HPP_ */
