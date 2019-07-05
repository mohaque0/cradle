#pragma once

#include <platform/cradle_platform.hpp>

#include <memory>
#include <string>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef PLATFORM_LINUX
	#include <unistd.h>
#endif

#ifdef PLATFORM_WINDOWS
	#include <direct.h>
	#define stat _stat
#endif

namespace cradle {
namespace platform {

#ifdef PLATFORM_WINDOWS

#define PATH_SEP ('\\')

int platform_mkdir(const char * const str) {
	return _mkdir(str);
}

#else

#define PATH_SEP ('/')

int platform_mkdir(const char * const str) {
	return mkdir(str, 0744);
}

#endif

int platform_chdir(const std::string& str) {
	return chdir(str.c_str());
}

}
}
