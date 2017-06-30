/*
 *  Xen Ctrl wrapper
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


#ifndef SRC_XEN_XENCTRL_HPP_
#define SRC_XEN_XENCTRL_HPP_

#include <vector>

extern "C" {
#include <xenctrl.h>
}

#include "XenException.hpp"
#include "Log.hpp"

namespace XenBackend {

/***************************************************************************//**
 * Exception generated by XenInterface.
 * @ingroup xen
 ******************************************************************************/
class XenCtrlException : public XenException
{
	using XenException::XenException;
};

/***************************************************************************//**
 * Provides Xen interface functionality.
 * @ingroup xen
 ******************************************************************************/
class XenInterface
{
public:

	XenInterface();
	XenInterface(const XenInterface&) = delete;
	XenInterface& operator=(XenInterface const&) = delete;
	~XenInterface();

	/**
	 * Returns all domains info
	 * @param[out] infos
	 */
	void getDomainsInfo(std::vector<xc_domaininfo_t>& infos);

private:

	const int cDomInfoChunkSize = 64;

	xc_interface* mHandle;

	Log mLog;

	void init();
	void release();
};

}

#endif /* SRC_XEN_XENCTRL_HPP_ */
