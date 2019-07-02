/**
 * @file
 */

#pragma once

//
// Code modified from: https://stackoverflow.com/questions/40504281/c-how-to-check-the-last-modified-time-of-a-file
//

#include <cradle_platform.hpp>
#include <cstring>
#include <stdexcept>

#include <sys/types.h>
#include <sys/stat.h>
#ifndef PLATFORM_WINDOWS
#include <unistd.h>
#endif

#ifdef PLATFORM_WINDOWS
#define stat _stat
#endif

namespace cradle {
namespace io {

bool exists(const std::string& filepath) {
	struct stat result;
	return stat(filepath.c_str(), &result) == 0;
}

struct stat getStat(const std::string& filepath) {
	struct stat result;
	if (stat(filepath.c_str(), &result) == 0) {
		return result;
	} else {
		throw std::runtime_error("Error getting stats for " + filepath + ": " + std::strerror(errno));
	}
}

} // namespace io
} // namespace cradle
