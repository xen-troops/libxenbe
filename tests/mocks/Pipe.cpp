/*
 *  Pipe
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

#include "Pipe.hpp"

#include <cstring>
#include <string>
#include <cstdint>
#include <unistd.h>

#include "Exception.hpp"

using std::string;

using XenBackend::Exception;

/*******************************************************************************
 * Pipe
 ******************************************************************************/

Pipe::Pipe()
{
	mFds[PipeType::READ] = -1;
	mFds[PipeType::WRITE] = -1;

	if (pipe(mFds) < 0)
	{
		throw Exception("Can't create pipe", errno);
	}
}

Pipe::~Pipe()
{
	if (mFds[PipeType::READ] >= 0)
	{
		close(mFds[PipeType::READ]);
	}

	if (mFds[PipeType::WRITE] >= 0)
	{
		close(mFds[PipeType::WRITE]);
	}
}

/*******************************************************************************
 * Public
 ******************************************************************************/
void Pipe::read()
{
	uint8_t data;

	if (::read(mFds[PipeType::READ], &data, sizeof(data)) < 0)
	{
		throw Exception("Error reading pipe", errno);
	}
}

void Pipe::write()
{
	uint8_t data = 0xFF;

	if (::write(mFds[PipeType::WRITE], &data, sizeof(data)) < 0)
	{
		throw Exception("Error writing pipe", errno);
	}
}
