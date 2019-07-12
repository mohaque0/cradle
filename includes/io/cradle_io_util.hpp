#pragma once

#include <io/cradle_stat.hpp>
#include <time.h>
#include <vector>

namespace cradle {
namespace io {

bool isTargetLessRecentThanFiles(const std::string& targetFile, const std::vector<std::string>& files) {
	if (!io::exists(targetFile)) {
		return true;
	}

	const struct stat targetFileStat = io::getStat(targetFile);

	for (auto& sourceFile : files) {
		if (!io::exists(sourceFile)) {
			return true;
		}

		const struct stat sourceFileStat = io::getStat(sourceFile);

		if (difftime(targetFileStat.st_mtime, sourceFileStat.st_mtime) < 0) {
			return true;
		}
	}

	return false;
}

}
}
