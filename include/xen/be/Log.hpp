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

#ifndef XENBE_LOG_HPP_
#define XENBE_LOG_HPP_

#include <algorithm>
#include <chrono>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>

/***************************************************************************//**
 * @defgroup log Backend log
 *
 * Special macros are designed to show logs: LOG() and DLOG().
 * DLOG is compiled to void in release build (NDEBUG is defined) and can be used
 * in time critical path. These macros returns a string stream object thus basic
 * c++ iostream operators can be used.
 * The macros take two parameters: first one could be
 * either instance of XenBackend::Log or string, second one is
 * XenBackend::LogLevel (for macro use DISABLE, ERROR, WARNING, INFO, DEBUG).
 * In case of XenBackend::Log instance all log parameters are kept inside
 * the instance. If the string is passed then it will be displayed as module
 * name. If <i>nullptr</i> is passed instead of string then the source file
 * name and line number will be displayed in the log.
 *
 * Log with string module name:
 * @code{.cpp}
 * LOG("ModuleName", DEBUG) << "This is debug log";
 *
 * output:
 *
 * 07.11.16 16:42:08.210 | ModuleName | DBG - This is debug log
 * @endcode
 *
 * Log with <i>nullptr</i>:
 * @code{.cpp}
 * LOG(nullptr, DEBUG) << "This is debug log";
 *
 * output:
 *
 * 07.11.16 16:43:56.256 | main.cpp 23 | DBG - This is debug log
 * @endcode
 *
 * Log with XenBackend::Log instance:
 * @code
 * XenBackend::Log myLog("MyModule");
 *
 * LOG(myLog, DEBUG) << "This is debug log";
 *
 * output:
 *
 * 07.11.16 16:46:54.029 | MyModule | DBG - This is debug log
 * @endcode
 *
 ******************************************************************************/

#define __FILENAME__ (strrchr(__FILE__, '/') ? \
					  strrchr(__FILE__, '/') + 1 : __FILE__)

/**
 * @def LOG(instance, level)
 * Displays log with defined level.
 * @param[in] instance log instance (XenBackend::Log) or <i>const char*</i> or
 *                         <i>nullptr</i>
 * @param[in] level    log level
 * @ingroup log
 */
#define LOG(instance, level) \
	XenBackend::LogLine().get(instance, __FILENAME__, __LINE__, \
							  XenBackend::LogLevel::log ## level)

/**
 * @def DLOG(instance, level)
 * Displays log with defined level for debug build. For release build it is
 * compiled to void.
 * @param[in] instance log instance (XenBackend::Log) or <i>const char*</i> or
 *                         <i>nullptr</i>
 * @param[in] level    log level
 * @ingroup log
 */
#ifndef NDEBUG

#define DLOG(instance, level) LOG(instance, level)

#else

#define DLOG(instance, level) \
	true ? (void) 0 : XenBackend::LogVoid() & LOG(instance, level)

#endif

namespace XenBackend {

/**
 * Log levels.
 * @ingroup log
 */
enum class LogLevel
{
	logDISABLE, logERROR, logWARNING, logINFO, logDEBUG
};

/// @cond HIDDEN_SYMBOLS
class LogVoid
{
public:

	LogVoid() { }
	void operator&(std::ostream&) { }
};
/// @endcond


/***************************************************************************//**
 * Log instance.
 * @ingroup log
 ******************************************************************************/
class Log
{
public:

	/**
	 * @param[in] name        module name which will be displayed in the log
	 * @param[in] level       log level for this module
	 * @param[in] fileAndLine displays source file name and line number instead
	 *                        of module name
	 */
	Log(const std::string& name, LogLevel level = Log::getLogLevel(),
		bool fileAndLine = Log::getShowFileAndLine()) :
		mName(name),  mLevel(level), mFileAndLine(fileAndLine)
	{
		setLogLevelByMask();
	}

	/**
	 * Gets current log level
	 * @return current log level
	 */
	static LogLevel& getLogLevel()
	{
		static LogLevel sCurrentLevel = LogLevel::logINFO;

		return sCurrentLevel;
	}

	/**
	 * Sets log level
	 * @param[in] level log level
	 */
	static void setLogLevel(LogLevel level) { getLogLevel() = level; }

	/**
	 * Sets log level
	 * @param[in] strLevel string log level (<i>"disable", "error",
	 * "warning", "info", "debug</i>)
	 * @return <i>true</i> if log level is set successfully
	 */
	static bool setLogLevel(const std::string& strLevel)
	{
		return getLogLevelByString(strLevel, getLogLevel());
	}

	/**
	 * Gets weather to display the source file name and line or the module name
	 * @return <i>true</i> if the source file name and line
	 * is be displayed
	 */
	static bool& getShowFileAndLine()
	{
		static bool sShowFileAndLine = false;

		return sShowFileAndLine;
	}

	/**
	 * Sets weather to display the source file name and line or the module name
	 * @param[in] showFileAndLine sets <i>true</i> if the source file name and
	 * line shall be displayed
	 */
	static void setShowFileAndLine(bool showFileAndLine)
	{
		getShowFileAndLine() = showFileAndLine;
	}

	/**
	 * Gets current log mask
	 * @return current log mask
	 */
	static std::string& getLogMask()
	{
		static std::string sLogMask;

		return sLogMask;
	}

	/**
	 * Sets log mask
	 * @param[in] mask log mask
	 * @return <i>true</i> if log mask is set successfully
	 */
	static bool setLogMask(const std::string& mask)
	{
		getLogMask() = mask;

		std::vector<std::string> items;

		splitMask(items, ',');

		std::vector<std::pair<std::string,
							  LogLevel>>& logMaskItems = getLogMaskItems();

		for(auto item : items)
		{
			size_t sepPos = 0;
			LogLevel logLevel = LogLevel::logDEBUG;

			sepPos = item.find(':');

			if (sepPos == std::string::npos)
			{
				sepPos = item.length();
			}
			else
			{
				if (!getLogLevelByString(item.substr(sepPos + 1), logLevel))
				{
					logMaskItems.clear();

					return false;
				}
			}

			logMaskItems.push_back(make_pair(item.substr(0, sepPos),
								   logLevel));
		}

		return true;
	}

	/**
	 * Sets file to write log
	 * @param[in] fileName file name
	 */
	static void setStreamBuffer(std::streambuf* streamBuffer)
	{
		getOutputStream().rdbuf(streamBuffer);
	}

private:

	friend class LogLine;

	std::string mName;
	LogLevel mLevel;
	bool mFileAndLine;

	static std::vector<std::pair<std::string, LogLevel>>& getLogMaskItems()
	{
		static std::vector<std::pair<std::string, LogLevel>> sMaskItems;

		return sMaskItems;
	}

	static std::ostream& getOutputStream()
	{
		static std::ostream sOutput(std::cout.rdbuf());

		return sOutput;
	}

	void setLogLevelByMask()
	{
		for(auto item : getLogMaskItems())
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

	static void splitMask(std::vector<std::string>& splitVector, char delim)
	{
		size_t curPos = 0;
		size_t delimPos = 0;

		splitVector.clear();

		auto logMask = getLogMask();

		if (logMask.empty())
		{
			return;
		}

		do
		{
			delimPos = logMask.find(delim, curPos);

			if (delimPos != std::string::npos)
			{
				splitVector.push_back(logMask.substr(curPos,
													 delimPos - curPos));

				curPos = ++delimPos;

				if (delimPos == logMask.length())
				{
					delimPos = std::string::npos;
				}
			}
			else
			{
				splitVector.push_back(logMask.substr(curPos,
													 logMask.length() -
													 curPos));
			}
		}
		while (delimPos != std::string::npos);
	}

	static bool getLogLevelByString(const std::string& strLevel,
									LogLevel& level)
	{
		static const char* strLevelArray[] =
		{
			"DISABLE",
			"ERROR",
			"WARNING",
			"INFO",
			"DEBUG"
		};

		std::string strLevelUp = strLevel;

		std::transform(strLevelUp.begin(), strLevelUp.end(), strLevelUp.begin(),
					   (int (*)(int))std::toupper);

		for(auto i = 0; i <= static_cast<int>(LogLevel::logDEBUG); i++)
		{
			if (std::string(strLevelArray[i]).compare(0, strLevelUp.length(),
												 	  strLevelUp) == 0)
			{
				level = static_cast<LogLevel>(i);

				return true;
			}
		}

		return false;
	}
};

/// @cond HIDDEN_SYMBOLS
class LogLine
{
public:

	virtual ~LogLine()
	{
		static std::mutex sMutex;

		if (mCurrentLevel <= mSetLevel && mSetLevel > LogLevel::logDISABLE)
		{
			std::lock_guard<std::mutex> lock(sMutex);

			Log::getOutputStream() << mStream.str() << std::endl;
		}
	}

	std::ostringstream& get(const Log& log, const char* file, int line,
							LogLevel level = LogLevel::logDEBUG)
	{
		mCurrentLevel = level;
		mSetLevel = log.mLevel;

		if (log.mFileAndLine)
		{
			putHeader(std::string(file) + " " + std::to_string(line));
		}
		else
		{
			putHeader(log.mName);
		}

		return mStream;
	}

	std::ostringstream& get(const char* name, const char* file, int line,
							LogLevel level = LogLevel::logDEBUG)
	{
		mCurrentLevel = level;
		mSetLevel = Log::getLogLevel();

		if (name)
		{
			putHeader(name);
		}
		else
		{
			putHeader(std::string(file) + " " + std::to_string(line));
		}

		return mStream;
	}

private:

	std::ostringstream mStream;
	LogLevel mCurrentLevel;
	LogLevel mSetLevel;

	void putHeader(const std::string& header)
	{
		static size_t sAlignmentLength;

		if (mCurrentLevel <= mSetLevel && mSetLevel > LogLevel::logDISABLE)
		{
			if (header.length() > sAlignmentLength)
			{
				sAlignmentLength = header.length();
			}

			mStream << nowTime()
					<< " | " << header << " "
					<< std::string(sAlignmentLength - header.length(), ' ')
					<< "| " << levelToString(mCurrentLevel) << " - ";
		}
	}

	std::string nowTime()
	{
		auto now = std::chrono::system_clock::now();
		auto time = std::chrono::system_clock::to_time_t(now);
		auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
					  now.time_since_epoch()) % 1000;

		std::stringstream ss;

		ss << std::put_time(localtime(&time), "%d.%m.%y %X.")
		   << std::setfill('0') << std::setw(3) << ms.count();

		return ss.str();
	}

	std::string levelToString(LogLevel level)
	{
		static const char* buffer[] = {"", "ERR", "WRN", "INF", "DBG"};

		return buffer[static_cast<int>(level)];
	}
};
/// @endcond

}

#endif /* XENBE_LOG_HPP_ */
