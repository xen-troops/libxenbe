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

#ifndef TEST_MOCKS_XENGNTTABMOCK_HPP_
#define TEST_MOCKS_XENGNTTABMOCK_HPP_

#include <unordered_map>

class XenGnttabMock
{
public:

	XenGnttabMock();

	static XenGnttabMock* getLastInstance() { return sLastInstance; }
	static void setErrorMode(bool errorMode) { mErrorMode = errorMode; }
	static bool getErrorMode() { return mErrorMode; }

	void* mapGrantRefs(uint32_t count, uint32_t domId, uint32_t *refs);
	void unmapGrantRefs(void* address, uint32_t count);
	void* getLastBuffer() const { return mLastMappedAddress; }
	size_t getMapBufferSize(void* address);
	void checkMapBuffers();

private:

	static XenGnttabMock* sLastInstance;
	static bool mErrorMode;

	struct MapBuffer
	{
		uint32_t count;
		uint32_t domId;
		size_t size;
	};

	void* mLastMappedAddress;
	std::unordered_map<void*, MapBuffer> mMapBuffers;
};

#endif /* TEST_MOCKS_XENGNTTABMOCK_HPP_ */
