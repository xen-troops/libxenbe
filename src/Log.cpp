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

using std::chrono::duration_cast;
using std::chrono::milliseconds;
using std::chrono::system_clock;
using std::chrono::system_clock;
using std::cout;
using std::endl;
using std::localtime;
using std::lock_guard;
using std::make_pair;
using std::mutex;
using std::ostringstream;
using std::pair;
using std::put_time;
using std::setfill;
using std::setw;
using std::string;
using std::to_string;
using std::transform;
using std::vector;

namespace XenBackend {

/*******************************************************************************
 * Static
 ******************************************************************************/

LogLevel Log::sCurrentLevel(LogLevel::logINFO);
bool Log::sShowFileAndLine(false);
string Log::sLogMask;
vector<pair<string, LogLevel>> Log::sLogMaskItems;

std::ostream Log::mOutput(cout.rdbuf());

/// @cond HIDDEN_SYMBOLS

size_t LogLine::sAlignmentLength = 0;

/*******************************************************************************
 * Log
 ******************************************************************************/

bool Log::setLogLevel(const string& strLevel)
{
	return getLogLevelByString(strLevel, sCurrentLevel);
}

/*******************************************************************************
 * Public
 ******************************************************************************/

bool Log::setLogMask(const string& mask)
{
	sLogMask = mask;

	vector<string> items;

	splitMask(items, ';');

	sLogMaskItems.clear();

	for(auto item : items)
	{
		size_t sepPos = 0;
		LogLevel logLevel = LogLevel::logDEBUG;

		sepPos = item.find(':');

		if (sepPos == string::npos)
		{
			sepPos = item.length();
		}
		else
		{
			if (!getLogLevelByString(item.substr(sepPos + 1), logLevel))
			{
				sLogMaskItems.clear();

				return false;
			}
		}

		sLogMaskItems.push_back(make_pair(item.substr(0, sepPos), logLevel));
	}

	return true;
}

/*******************************************************************************
 * Private
 ******************************************************************************/

void Log::setLogLevelByMask()
{
	for(auto item : sLogMaskItems)
	{
		if (item.first.back() == '*')
		{
			item.first.pop_back();

			if (mName.compare(0, item.first.length(), item.first) == 0)
			{
				mLevel = item.second;
			}
		}
		else
		{
			if (item.first == mName)
			{
				mLevel = item.second;
			}
		}
	}
}

bool Log::getLogLevelByString(const string& strLevel, LogLevel& level)
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
		if (string(strLevelArray[i]).compare(0, strLevelUp.length(),
											 strLevelUp) == 0)
		{
			level = static_cast<LogLevel>(i);

			return true;
		}
	}

	return false;
}

void Log::splitMask(vector<string>& splitVector, char delim)
{
	size_t curPos = 0;
	size_t delimPos = 0;

	splitVector.clear();

	if (sLogMask.empty())
	{
		return;
	}

	do
	{
		delimPos = sLogMask.find(delim, curPos);

		if (delimPos != string::npos)
		{
			splitVector.push_back(sLogMask.substr(curPos, delimPos - curPos));

			curPos = ++delimPos;

			if (delimPos == sLogMask.length())
			{
				delimPos = string::npos;
			}
		}
		else
		{
			splitVector.push_back(sLogMask.substr(curPos, sLogMask.length() -
												  curPos));
		}
	}
	while (delimPos != string::npos);
}

/*******************************************************************************
 * LogLine
 ******************************************************************************/

mutex LogLine::mMutex;

LogLine::~LogLine()
{
	if (mCurrentLevel <= mSetLevel && mSetLevel > LogLevel::logDISABLE)
	{
		mStream << endl;

		lock_guard<mutex> lock(mMutex);

		Log::mOutput << mStream.str();
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
