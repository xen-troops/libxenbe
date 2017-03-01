#include "test.hpp"

#define CATCH_CONFIG_MAIN
#define CATCH_CONFIG_COLOUR_NONE

#include <catch.hpp>

using std::unique_ptr;

using XenBackend::Log;
using XenBackend::LogLevel;

TEST_CASE("Backend creation", "[backend]")
{
//	Log::setLogLevel(LogLevel::logDISABLE);
	Log::setLogLevel(LogLevel::logDEBUG);

	TestBackend testBackend;
}
