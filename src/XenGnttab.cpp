/*
 *  Xen gnttab wrapper
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

#include "XenGnttab.hpp"

namespace XenBackend {

/*******************************************************************************
 * XenGnttab
 ******************************************************************************/

XenGnttab::XenGnttab()
{
	mHandle = xengnttab_open(nullptr, 0);

	if (!mHandle)
	{
		throw XenGnttabException("Can't open xc grant table", errno);
	}
}

XenGnttab::~XenGnttab()
{
	if (mHandle)
	{
		xengnttab_close(mHandle);
	}
}

xengnttab_handle* XenGnttab::getHandle()
{
	static XenGnttab gnttab;

	return gnttab.mHandle;
}

/*******************************************************************************
 * XenGnttabBuffer
 ******************************************************************************/

XenGnttabBuffer::XenGnttabBuffer(domid_t domId, grant_ref_t ref, int prot,
								 size_t offset) :
		XenGnttabBuffer(domId, &ref, 1, prot, offset)
{

}

XenGnttabBuffer::XenGnttabBuffer(domid_t domId, const grant_ref_t* refs,
								 size_t count, int prot, size_t offset) :
	mLog("XenGnttabBuffer")
{
	init(domId, refs, count, prot, offset);
}

XenGnttabBuffer::~XenGnttabBuffer()
{
	release();
}

/*******************************************************************************
 * Private
 ******************************************************************************/

void XenGnttabBuffer::init(domid_t domId, const grant_ref_t* refs,
						   size_t count, int prot, size_t offset)
{
	mHandle = XenGnttab::getHandle();
	mBuffer = nullptr;
	mOffset = offset;
	mCount = count;

	DLOG(mLog, DEBUG) << "Create grant table buffer, dom: " << domId
					  << ", count: " << count << ", ref: " << *refs
					  << ", buffer offset: " << offset;


	mBuffer = xengnttab_map_domain_grant_refs(mHandle, count, domId,
											  const_cast<grant_ref_t*>(refs),
											  PROT_READ | PROT_WRITE);

	if (!mBuffer)
	{
		throw XenGnttabException("Can't map buffer", errno);
	}

}

void XenGnttabBuffer::release()
{
	DLOG(mLog, DEBUG) << "Delete grant table buffer";

	if (mBuffer)
	{
		xengnttab_unmap(mHandle, mBuffer, mCount);
	}
}

#ifdef GNTDEV_DMA_FLAG_WC

/*******************************************************************************
 * XenGnttabDmaBufferExporter
 ******************************************************************************/

XenGnttabDmaBufferExporter::XenGnttabDmaBufferExporter(domid_t domId,
													   const GrantRefs &refs):
	mDmaBufFd(-1),
	mLog("XenGnttabDmaBufferExporter")
{
	try
	{
		init(domId, refs);
	}
	catch(const std::exception& e)
	{
		release();

		throw;
	}
}

XenGnttabDmaBufferExporter::~XenGnttabDmaBufferExporter()
{
	release();
}

/*******************************************************************************
 * Private
 ******************************************************************************/

void XenGnttabDmaBufferExporter::init(domid_t domId, const GrantRefs &refs)
{
	uint32_t fd;
	int ret;

	mHandle = XenGnttab::getHandle();

	DLOG(mLog, DEBUG) << "Produce DMA buffer from grant references, dom: "
					  << domId << ", count: " << refs.size();

	/* Always allocate the buffer to be DMA capable. */
	ret = xengnttab_dmabuf_exp_from_refs(mHandle, domId, GNTDEV_DMA_FLAG_WC,
										 refs.size(), refs.data(), &fd);

	if (ret)
	{
		throw XenGnttabException("Can't produce DMA buffer from grant references",
								 errno);
	}

	mDmaBufFd = fd;
}

int XenGnttabDmaBufferExporter::waitForReleased(int timeoutMs)
{
	uint32_t fd = mDmaBufFd;
	int ret;

	release();

	ret = xengnttab_dmabuf_exp_wait_released(mHandle, fd, timeoutMs);

	if (ret)
	{
		DLOG(mLog, ERROR) << "Wait for DMA buffer failed, fd: "
					  << fd << ", err: " << ret
					  << "(" << strerror(errno) << ")";
	}

	return ret;
}

void XenGnttabDmaBufferExporter::release()
{
	if (mDmaBufFd >= 0)
	{
		close(mDmaBufFd);
		mDmaBufFd = -1;
	}
}

/*******************************************************************************
 * XenGnttabDmaBufferImporter
 ******************************************************************************/

XenGnttabDmaBufferImporter::XenGnttabDmaBufferImporter(domid_t domId,
													   int fd,
													   GrantRefs &refs):
	mDmaBufFd(-1),
	mLog("XenGnttabDmaBufferImporter")
{
	try
	{
		init(domId, fd, refs);
	}
	catch(const std::exception& e)
	{
		release();

		throw;
	}
}

XenGnttabDmaBufferImporter::~XenGnttabDmaBufferImporter()
{
	release();
}

/*******************************************************************************
 * Private
 ******************************************************************************/

void XenGnttabDmaBufferImporter::init(domid_t domId, int fd, GrantRefs &refs)
{
	int ret;

	mHandle = XenGnttab::getHandle();

	DLOG(mLog, DEBUG) << "Produce grant references from DMA buffer, dom: "
					  << domId << ", fd: " << fd << ", count: " << refs.size();

	ret = xengnttab_dmabuf_imp_to_refs(mHandle, domId, fd,
									   refs.size(), refs.data());

	if (ret)
	{
		throw XenGnttabException("Can't produce grant references from DMA buffer",
								 errno);
	}

	mDmaBufFd = fd;
}

void XenGnttabDmaBufferImporter::release()
{
	if (mDmaBufFd >= 0)
	{
		int ret;

		DLOG(mLog, DEBUG) << "Release DMA buffer, fd: " << mDmaBufFd;

		ret = xengnttab_dmabuf_imp_release(mHandle, mDmaBufFd);

		if (ret)
		{
			throw XenGnttabException("Can't release DMA buffer", errno);
		}

		mDmaBufFd = -1;
	}
}

#endif	// GNTDEV_DMA_FLAG_WC

}
