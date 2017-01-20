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

#ifndef SRC_XEN_UTILS_HPP_
#define SRC_XEN_UTILS_HPP_

#include <string>

#include <poll.h>
#include <unistd.h>

extern "C" {
#include <xen/xen.h>
#include <xen/io/xenbus.h>
}

namespace XenBackend {

/***************************************************************************//**
 * Different helpers.
 * @ingroup backend
 ******************************************************************************/
class Utils
{
public:

	/**
	 * Returns string which represents domain id and instance id for logging
	 * @param[in] domId domain id
	 * @param[in] id    instance id
	 * @return string representation of domain id and istance id
	 */
	static std::string logDomId(domid_t domId, int id);

	/**
	 * Returns string representation of xen domain state
	 * @param[in] state xen domain state
	 * @return string representation of xen domain state
	 */
	static std::string logState(xenbus_state state);
};

/***************************************************************************//**
 * Class to poll file descriptor.
 * It can be interripted by calling stop() method.
 * @ingroup backend
 ******************************************************************************/
class PollFd
{
public:

	/**
	 * @param fd     file descriptor
	 * @param events events to poll (same as in system poll function)
	 */
	PollFd(int fd, short int events);
	~PollFd();

	/**
	 * Polls the file descriptors for defined events
	 * @return <i>true</i> if one of defined events occurred and <i>false</i>
	 * if the method was interrupted by calling stop()
	 */
	bool poll();

	/**
	 * Stops polling
	 */
	void stop();

private:

	enum PipeType
	{
		READ = 0,
		WRITE = 1
	};

	enum PollIndex
	{
		FILE = 0,
		PIPE = 1
	};

	pollfd mFds[2];
	int mPipeFds[2];

	void init(int fd, short int events);
	void release();
};

}

#endif /* SRC_XEN_UTILS_HPP_ */
