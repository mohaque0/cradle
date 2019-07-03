/**
 * @file cradle_platform.hpp
 *
 * @brief Macros and functions for getting information about the platform that cradle is running on.
 *
 * Currently only supports detection of Linux, Mac, and Windows.
 */

#pragma once

#ifdef __APPLE__
#define PLATFORM_MAC
#endif

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
#define PLATFORM_WINDOWS
#elif defined(__linux__)
#define PLATFORM_LINUX
#else
#error Unsupported platform.
#endif


namespace cradle {
namespace platform {
namespace os {

bool is_windows() {
#ifdef PLATFORM_WINDOWS
	return true;
#else
	return false;
#endif
}

bool is_linux() {
#ifdef PLATFORM_LINUX
	return true;
#else
	return false;
#endif
}

bool is_mac() {
#ifdef PLATFORM_MAC
	return true;
#else
	return false;
#endif
}

}
}
}
