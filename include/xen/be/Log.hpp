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

#ifndef SRC_XEN_LOG_HPP_
#define SRC_XEN_LOG_HPP_

#include <atomic>
#include <cstring>
#include <mutex>
#include <sstream>

/***************************************************************************//**
 * @defgroup Log
 * Backend log.
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
 * @ingroup Log
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
 * @ingroup Log
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
 * @ingroup Log
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
 * @ingroup Log
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
	Log(const std::string& name, LogLevel level = sCurrentLevel,
		bool fileAndLine = sShowFileAndLine) :
		mName(name),  mLevel(level), mFileAndLine(fileAndLine) {}

	/**
	 * Sets log level
	 * @param[in] level log level
	 */
	static void setLogLevel(LogLevel level) { sCurrentLevel = level; }

	/**
	 * Sets log level
	 * @param[in] strLevel string log level (<i>"disable", "error",
	 * "warning", "info", "debug</i>)
	 * @return <i>true</i> if log level is set successfully
	 */
	static bool setLogLevel(const std::string& strLevel);

	/**
	 * Gets current log level
	 * @return current log level
	 */
	static LogLevel getLogLevel() { return sCurrentLevel; }

	/**
	 * Sets weather to display the source file name and line or the module name
	 * @param[in] showFileAndLine sets <i>true</i> if the source file name and
	 * line shall be displayed
	 */
	static void setShowFileAndLine(bool showFileAndLine)
	{
		sShowFileAndLine = showFileAndLine;
	}

	/**
	 * Gets weather to display the source file name and line or the module name
	 * @return <i>true</i> if the source file name and line
	 * is be displayed
	 */
	static bool getShowFileAndLine() { return sShowFileAndLine; }

private:

	friend class LogLine;

	static std::atomic<LogLevel> sCurrentLevel;
	static std::atomic_bool sShowFileAndLine;

	std::string mName;
	LogLevel mLevel;
	bool mFileAndLine;
};

/// @cond HIDDEN_SYMBOLS
class LogLine
{
public:
	LogLine();
	virtual ~LogLine();

	std::ostringstream& get(const Log& log, const char* file, int line,
							LogLevel level = LogLevel::logDEBUG);
	std::ostringstream& get(const char* name, const char* file, int line,
							LogLevel level = LogLevel::logDEBUG);

private:
	static size_t sAlignmentLength;

	std::ostringstream mStream;
	LogLevel mCurrentLevel;
	LogLevel mSetLevel;

	std::mutex mMutex;

	void putHeader(const std::string& header);
	std::string nowTime();
	std::string levelToString(LogLevel level);
};
/// @endcond

}

#endif /* SRC_XEN_LOG_HPP_ */
