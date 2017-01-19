/*
 *  Log implementation
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

#include "Log.hpp"

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <iostream>

using std::atomic;
using std::atomic_bool;
using std::chrono::duration_cast;
using std::chrono::milliseconds;
using std::chrono::system_clock;
using std::chrono::system_clock;
using std::cout;
using std::endl;
using std::localtime;
using std::lock_guard;
using std::mutex;
using std::ostringstream;
using std::put_time;
using std::setfill;
using std::setw;
using std::string;
using std::to_string;
using std::transform;

namespace XenBackend {

/*******************************************************************************
 * Static
 ******************************************************************************/

atomic<LogLevel> Log::sCurrentLevel(LogLevel::logINFO);
atomic_bool Log::sShowFileAndLine(false);

/// @cond HIDDEN_SYMBOLS

size_t LogLine::sAlignmentLength = 0;

/*******************************************************************************
 * Log
 ******************************************************************************/

bool Log::setLogLevel(const string& strLevel)
{
	static const char* strLevelArray[] =
	{
		"DISABLE",
		"ERROR",
		"WARNING",
		"INFO",
		"DEBUG"
	};

	string strLevelUp = strLevel;

	transform(strLevelUp.begin(), strLevelUp.end(), strLevelUp.begin(),
			  (int (*)(int))std::toupper);

	for(auto i = 0; i <= static_cast<int>(LogLevel::logDEBUG); i++)
	{
		if (strLevelUp == strLevelArray[i])
		{
			sCurrentLevel = static_cast<LogLevel>(i);

			return true;
		}
	}

	return false;
}

/*******************************************************************************
 * LogLine
 ******************************************************************************/

mutex LogLine::mMutex;

LogLine::LogLine()
{

}

LogLine::~LogLine()
{
	if (mCurrentLevel <= mSetLevel && mSetLevel > LogLevel::logDISABLE)
	{
		lock_guard<mutex> lock(mMutex);

		mStream << endl;

		cout << mStream.str();
		cout.flush();
	}
}

/*******************************************************************************
 * Public
 ******************************************************************************/

ostringstream& LogLine::get(const Log& log, const char* file,
							int line, LogLevel level)
{
	mCurrentLevel = level;
	mSetLevel = log.mLevel;

	if (log.mFileAndLine)
	{
		putHeader(string(file) + " " + to_string(line));
	}
	else
	{
		putHeader(log.mName);
	}

	return mStream;
}

ostringstream& LogLine::get(const char* name, const char* file,
							int line, LogLevel level)
{
	mCurrentLevel = level;
	mSetLevel = Log::sCurrentLevel;

	if (name)
	{
		putHeader(name);
	}
	else
	{
		putHeader(string(file) + " " + to_string(line));
	}

	return mStream;
}

/*******************************************************************************
 * Private
 ******************************************************************************/

void LogLine::putHeader(const string& header)
{
	if (mCurrentLevel <= mSetLevel && mSetLevel > LogLevel::logDISABLE)
	{
		if (header.length() > sAlignmentLength)
		{
			sAlignmentLength = header.length();
		}

		mStream << nowTime()
				<< " | " << header << " "
				<< string(sAlignmentLength - header.length(), ' ') << "| "
				<< levelToString(mCurrentLevel) << " - ";
	}
}

string LogLine::levelToString(LogLevel level)
{
	static const char* buffer[] = {"", "ERR", "WRN", "INF", "DBG"};

	return buffer[static_cast<int>(level)];
}

string LogLine::nowTime()
{
	auto now = system_clock::now();
	auto time = system_clock::to_time_t(now);
	auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;

	std::stringstream ss;

	ss << put_time(localtime(&time), "%d.%m.%y %X.")
	   << setfill('0') << setw(3) << ms.count();

	return ss.str();
}

/// @endcond

}
