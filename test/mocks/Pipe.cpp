/*
 * Pipe.cpp
 *
 *  Created on: Feb 28, 2017
 *      Author: al1
 */

#include "Pipe.hpp"

#include <cstring>
#include <string>

#include <unistd.h>

#include "XenException.hpp"

using std::string;

using XenBackend::XenException;

/*******************************************************************************
 * Pipe
 ******************************************************************************/

Pipe::Pipe()
{
	mFds[PipeType::READ] = -1;
	mFds[PipeType::WRITE] = -1;

	if (pipe(mFds) < 0)
	{
		throw XenException("Can't create pipe: " + string(strerror(errno)));
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
		throw XenException("Error reading pipe: " + string(strerror(errno)));
	}
}

void Pipe::write()
{
	uint8_t data = 0xFF;

	if (::write(mFds[PipeType::WRITE], &data, sizeof(data)) < 0)
	{
		throw XenException("Error writing pipe: " + string(strerror(errno)));
	}
}
