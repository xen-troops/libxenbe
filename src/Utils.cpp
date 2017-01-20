/*
 *  Backend utils
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

#include "Utils.hpp"

#include <vector>

#include "XenException.hpp"

using std::exception;
using std::string;
using std::to_string;
using std::vector;

namespace XenBackend {

/*******************************************************************************
 * Utils
 ******************************************************************************/

string Utils::logDomId(domid_t domId, int id)
{
	return string("Dom(" + to_string(domId) + "/" + to_string(id) + ")");
}

string Utils::logState(xenbus_state state)
{
	static const vector<string> strStates = {"Unknown", "Initializing",
											 "InitWait", "Initialized",
											 "Connected",
											 "Closing", "Closed",
											 "Reconfiguring", "Reconfigured"};

	if (state >= strStates.size() || state < 0)
	{
		return "Error!!!";
	}
	else
	{
		return "[" + strStates[state] + "]";
	}
}

/*******************************************************************************
 * PollFd
 ******************************************************************************/

PollFd::PollFd(int fd, short int events)
{
	try
	{
		init(fd, events);
	}
	catch(const exception& e)
	{
		release();

		throw;
	}
}

PollFd::~PollFd()
{
	release();
}

bool PollFd::poll()
{
	mFds[PollIndex::FILE].revents = 0;
	mFds[PollIndex::PIPE].revents = 0;

	if (::poll(mFds, 2, -1) < 0)
	{
		throw XenException("Error polling files");
	}

	if (mFds[PollIndex::PIPE].revents & POLLIN)
	{
		uint8_t data;

		if (read(mFds[PollIndex::PIPE].fd, &data, sizeof(data)) < 0)
		{
			throw XenException("Error reading pipe");
		}

		return false;
	}

	if (mFds[PollIndex::FILE].revents & (~mFds[PollIndex::FILE].events))
	{
		throw XenException("Error reading file");
	}

	return true;
}

void PollFd::stop()
{
	uint8_t data = 0xFF;

	if (write(mPipeFds[PipeType::WRITE], &data, sizeof(data)) < 0)
	{
		throw XenException("Error writing pipe");
	}
}

void PollFd::init(int fd, short int events)
{
	mPipeFds[PipeType::READ] = -1;
	mPipeFds[PipeType::WRITE] = -1;

	if (pipe(mPipeFds) < 0)
	{
		throw XenException("Can't create pipe");
	}

	mFds[PollIndex::FILE].fd = fd;
	mFds[PollIndex::FILE].events = events;

	mFds[PollIndex::PIPE].fd = mPipeFds[0];
	mFds[PollIndex::PIPE].events = POLLIN;
}

void PollFd::release()
{
	if (mPipeFds[PipeType::READ] >= 0)
	{
		close(mPipeFds[PipeType::READ]);
	}

	if (mPipeFds[PipeType::WRITE] >= 0)
	{
		close(mPipeFds[PipeType::WRITE]);
	}
}

}
