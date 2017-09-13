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

#include <atomic>
#include <condition_variable>
#include <list>
#include <string>
#include <thread>

#include <poll.h>
#include <unistd.h>

extern "C" {
#include <xenctrl.h>
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
	 * Returns string which represents domain id and device id for logging
	 * @param[in] domId domain id
	 * @param[in] devId device id
	 * @return string representation of domain id and device id
	 */
	static std::string logDomId(domid_t domId, uint16_t devId);

	/**
	 * Returns string representation of xen domain state
	 * @param[in] state xen domain state
	 * @return string representation of xen domain state
	 */
	static std::string logState(xenbus_state state);

	/**
	 * Returns lib xenbe version
	 */
	static std::string getVersion();
};

/***************************************************************************//**
 * Class to poll file descriptor.
 *
 * The PollFd class also opens an additional pipe. On poll() method it waits
 * for both: the defined file descriptor and the internal pipe file descriptor.
 * The internal pipe file descriptor breaks poll() when stop() method is
 * invoked. It is used to unblock poll() when an object using PollFd is been
 * deleted.
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

/***************************************************************************//**
 * Implements asynchronous context
 *
 * This class allows to call a function asynchronously.
 *
 * @ingroup backend
 ******************************************************************************/
class AsyncContext
{
public:

	typedef std::function<void()> AsyncCall;

	AsyncContext();
	~AsyncContext();

	/**
	 * Stops async thread
	 */
	void stop();

	/**
	 * Adds a function to be called asynchronously
	 * @param f callback
	 */
	void call(AsyncCall f);

private:

	std::atomic_bool mTerminate;
	std::mutex mMutex;
	std::condition_variable mCondVar;
	std::thread mThread;

	std::list<AsyncCall> mAsyncCalls;

	void run();
};

/***************************************************************************//**
 * Implements timer
 *
 * This class allows to call event in scheduled time or periodically
 *
 * @ingroup backend
 ******************************************************************************/
class Timer
{
public:

	typedef std::function<void()> Callback;

	Timer(Callback callback, std::chrono::milliseconds time,
		  bool periodic = false);
	~Timer();

	/**
	 * Starts timer
	 */
	void start();

	/**
	 * Stops timer
	 */
	void stop();

private:

	Callback mCallback;
	std::chrono::milliseconds mTime;
	bool mPeriodic;

	std::atomic_bool mTerminate;
	std::thread mThread;
	std::mutex mMutex;
	std::mutex mItfMutex;
	std::condition_variable mCondVar;

	void run();
};

}

#endif /* SRC_XEN_UTILS_HPP_ */
