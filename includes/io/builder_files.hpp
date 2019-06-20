#pragma once

#include <builder_main.hpp>
#include <builder_platform.hpp>
#include <builder_types.hpp>
#include <io/builder_tinydir.hpp>

#include <algorithm>
#include <string.h>
#include <regex>

namespace cradle {
namespace io {

static const std::string FILE_LIST = "FILE_LIST";

//
// Platform dependent section.
//

#ifdef PLATFORM_WINDOWS
#include <direct.h>

#define PATH_SEP ('\\')

int platform_mkdir(const char * const str) {
	return _mkdir(str);
}

#else
#include <sys/types.h>
#include <sys/stat.h>

#define PATH_SEP ('/')

int platform_mkdir(const char * const str) {
	return mkdir(str, 0744);
}

#endif

//
// End platform dependent section.
//


// TODO: Do something smarter to handle volume names and other things.
std::string path_concat(std::string a, std::string b) {
	return a + PATH_SEP + b;
}

// TODO: Do something smarter to handle volume names and other things.
std::string path_parent(std::string p) {
    std::size_t posA = p.find_last_of(PATH_SEP);
    std::size_t posB = p.find_last_of('/');

    std::size_t pos =
            (posA != std::string::npos && posB != std::string::npos) ?
				std::max(posA, posB) : // If they're both valid return the max.
                (posA == std::string::npos ? posB : posA); // Otherwise return the valid one (technically, could still be invalid.)

    // Check pos isn't invalid.
	if (pos == std::string::npos) {
		return std::string(1, PATH_SEP);
	}
	return p.substr(0, pos);
}

/**
 * @brief path_filename
 * @param p
 * @return The last name in this path.
 */
std::string path_filename(std::string p) {
	std::size_t pos = p.find_last_of(PATH_SEP);
	if (pos == std::string::npos) {
		return p;
	}
	return p.substr(pos+1);
}

/**
 * @brief path_basename
 * @param p
 * @return The path with the extension removed.
 */
std::string path_basename(std::string p) {
	std::size_t pos = p.find_last_of('.');
	if (pos == std::string::npos) {
		return p;
	}
	return p.substr(0, pos);
}

/**
 * @brief path_ext
 * @param p
 * @return The extension of the path. (i.e. the part after and not including the last '.')
 */
std::string path_ext(std::string p) {
	std::size_t pos = p.find_last_of('.');
	if (pos == std::string::npos) {
		return p;
	}
	return p.substr(pos+1);
}

void mkdir_if_necessary(std::string d) {
    tinydir_dir dir;
    if (tinydir_open(&dir, d.c_str()) != 0) {
        if (platform_mkdir(d.c_str()) != 0) {
            printf("Error making directory %s: %s", d.c_str(), strerror(errno));
        }
    }
}

void mkdirs(std::string d) {
	if (d == std::string(1, PATH_SEP)) {
		return;
	}

	mkdirs(path_parent(d));
	mkdir_if_necessary(d);
}

void recursiveAddFilesInDir(
		std::vector<std::string>& aggregator,
		const std::string& path,
		const std::regex& include,
		const std::regex& exclude
) {
	tinydir_dir dir;
	tinydir_open(&dir, path.c_str());

	while (dir.has_next) {
		tinydir_file file;
		tinydir_readfile(&dir, &file);

		std::string filename(file.name);
		std::string filepath(file.path);

		if (filename == "." || filename == "..") {
			tinydir_next(&dir);
			continue;
		}

		if (file.is_dir) {
			recursiveAddFilesInDir(aggregator, file.path, include, exclude);

		} else {
			std::smatch includeMatch;
			std::smatch excludeMatch;

			if (
				std::regex_match(filepath, includeMatch, include) &&
				!std::regex_match(filepath, excludeMatch, exclude)
			) {
				aggregator.push_back(filepath);
			}
		}

		tinydir_next(&dir);
	}
}

/**
 * @brief files    A task to recursively find all files in a directory.
 * @param dir      The path of the directory to search. Can be relative or absolute.
 * @param include  A regex (using standard C++ regex library syntax) that will be run on the entire path to the file to determine if it is to be included.
 * @param exclude  A regex (using standard C++ regex library syntax) that will be run on the entire path to the file after it has been added to the inclusion list
 *                 to determine if it is actually to be excluded.
 * @return
 */
task_p files(std::string dir, std::string include = std::string(".*"), std::string exclude = std::string("a^")) {
	return task(
		[=] (Task* self) {
			std::vector<std::string> aggregator;
			std::regex includeRegex(include);
			std::regex excludeRegex(exclude);

			recursiveAddFilesInDir(aggregator, dir, includeRegex, excludeRegex);
			self->push(FILE_LIST, aggregator);
			return ExecutionResult::SUCCESS;
		}
	);
}

} // namespace io
} // namespace cradle
