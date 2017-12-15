/*
 *  Xen Exception
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
 *
 */

#ifndef XENBE_EXCEPTION_HPP_
#define XENBE_EXCEPTION_HPP_

#include <exception>
#include <functional>
#include <string>

#include <cstring>

namespace XenBackend {

/**
 * Callback which is called when an error occurs
 */
typedef std::function<void(const std::exception&)> ErrorCallback;

/***************************************************************************//**
 * Base class for all Xen exception.
 * @ingroup xen
 ******************************************************************************/
class Exception : public std::exception
{
public:
	/**
	 * @param msg error message
	 */
	explicit Exception(const std::string& msg, int errCode) :
		mMsg(msg), mErrCode(errCode) {};

	/**
	 * returns error message
	 */
	const char* what() const throw() override
	{
		if (mWhat.empty())
		{
			mWhat = formatMessage(mMsg, mErrCode);
		}

		return mWhat.c_str();
	}

	/**
	 * returns error code
	 */
	int getErrno() const { return mErrCode; }

private:
	std::string mMsg;
	int mErrCode;
	mutable std::string mWhat;

	virtual std::string formatMessage(const std::string& msg, int errCode) const
	{
		return msg + " (" + strerror(errCode) + ")";
	}
};

}

#endif /* XENBE_EXCEPTION_HPP_ */
