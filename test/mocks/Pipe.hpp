/*
 * Pipe.hpp
 *
 *  Created on: Feb 28, 2017
 *      Author: al1
 */

#ifndef TEST_MOCKS_PIPE_HPP_
#define TEST_MOCKS_PIPE_HPP_

#include <cstddef>

class Pipe
{
public:

	Pipe();
	~Pipe();

	int getFd() const { return mFds[PipeType::READ]; }

	void read();
	void write();

private:

	enum PipeType
	{
		READ = 0,
		WRITE = 1
	};

	int mFds[2];
};

#endif /* TEST_MOCKS_PIPE_HPP_ */
