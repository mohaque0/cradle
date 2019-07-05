/**
 * @file
 */

#pragma once

#include <cradle_builder.hpp>
#include <cradle_exec.hpp>
#include <cradle_main.hpp>
#include <cradle_types.hpp>
#include <cpp/cradle_cpp_toolchain.hpp>
#include <io/cradle_files.hpp>
#include <io/cradle_stat.hpp>

#include <time.h>

namespace cradle {
namespace cpp {

static const std::string OUTPUT_FILE = "OUTPUT_FILE";
static const std::string LIBRARY_NAME = "LIBRARY_NAME";
static const std::string LIBRARY_PATH = "LIBRARY_PATH";

namespace detail {

bool strendswith(const std::string& str, const std::string& end) {
	if (str.length() < end.length()) {
		return false;
	}

	for (unsigned int i = 0; i < end.length(); i++) {
		if (str.at(str.length() - end.length() + i) != end[i]) {
			return false;
		}
	}

	return true;
}

bool isTargetLessRecentThanHeaderFiles(const struct stat& targetFileStat, const std::string& path) {
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
			if (isTargetLessRecentThanHeaderFiles(targetFileStat, file.path)) {
				return true;
			}

		} else if (
			strendswith(filename, ".hpp") ||
			strendswith(filename, ".hh") ||
			strendswith(filename, ".h") ||
			strendswith(filename, ".tpp")
		) {
			const struct stat sourceFileStat = io::getStat(file.path);

			if (difftime(targetFileStat.st_mtime, sourceFileStat.st_mtime) < 0) {
				return true;
			}
		}

		tinydir_next(&dir);
	}

	return false;
}

bool isTargetLessRecentThanFiles(const std::string& targetFile, const std::string& sourceFile, const std::vector<std::string>& includeSearchDirs) {
	if (!io::exists(targetFile)) {
		return true;
	}

	const struct stat targetFileStat = io::getStat(targetFile);
	const struct stat sourceFileStat = io::getStat(sourceFile);

	if (difftime(targetFileStat.st_mtime, sourceFileStat.st_mtime) < 0) {
		return true;
	}

	for (auto& dir : includeSearchDirs) {
		if (isTargetLessRecentThanHeaderFiles(targetFileStat, dir)) {
			return true;
		}
	}

	return false;
}

bool isTargetLessRecentThanFiles(const std::string& targetFile, const std::vector<std::string>& files) {
	if (!io::exists(targetFile)) {
		return true;
	}

	const struct stat targetFileStat = io::getStat(targetFile);

	for (auto& sourceFile : files) {
		// TODO: This conditional is here to prevent attempting to get stats of system libraries like pthread.
		// There should perhaps be a better solution than skipping it.
		if (!io::exists(sourceFile)) {
			continue;
		}

		const struct stat sourceFileStat = io::getStat(sourceFile);

		if (difftime(targetFileStat.st_mtime, sourceFileStat.st_mtime) < 0) {
			return true;
		}
	}

	return false;
}

std::string resolveFile(const std::string& name, const std::vector<std::string>& paths) {
	for (auto& path : paths) {
		std::string fileCandidate = io::path_concat(path, name);
		if (io::exists(fileCandidate)) {
			return fileCandidate;
		}
	}

	return name;
}


task_p object(
	std::string rootTaskName,
	std::string filePath,
	std::vector<std::string> includeSearchDirs = std::vector<std::string>(),
	std::string outputDirectory = DEFAULT_BUILD_DIR,
	std::shared_ptr<Toolchain> toolchain = Toolchain::platformDefault()
) {
	auto configure = task(rootTaskName + ':' + filePath + ":compile", [=] (Task* self) {
		std::string outputFile = io::path_concat(outputDirectory, toolchain->objectFileNameFromBase(filePath));
		self->set(OUTPUT_FILE, outputFile);

		if (isTargetLessRecentThanFiles(outputFile, filePath, includeSearchDirs)) {

			std::string cmdline = toolchain->compileObjectCmd(outputFile, filePath, includeSearchDirs);
			io::mkdirs(io::path_parent(outputFile));
			return exec(cmdline)->execute();

		} else {
			return ExecutionResult::SUCCESS;
		}
	});

	return configure;
}

task_p static_lib(
	std::string taskName,
	std::string name,
	std::vector<std::string> sourceFiles,
	std::vector<std::string> includeSearchDirs = std::vector<std::string>(),
	std::string outputDirectory = DEFAULT_BUILD_DIR,
	std::shared_ptr<Toolchain> toolchain = Toolchain::platformDefault()
) {
	std::string outputFile(io::path_concat(outputDirectory, toolchain->staticLibNameFromBase(name)));

	std::vector<task_p> objectFileTasks;
	for (auto file : sourceFiles) {
		objectFileTasks.push_back(detail::object(name, file, includeSearchDirs, outputDirectory, toolchain));
	}

	auto buildArchive = task(taskName, [=] (Task* self) {
		std::vector<std::string> objectFiles;
		for (auto task : objectFileTasks) {
			objectFiles.push_back(task->get(OUTPUT_FILE));
		}

		if (isTargetLessRecentThanFiles(outputFile, objectFiles)) {
			std::string cmdline = toolchain->buildStaticLibCmd(outputFile, objectFiles);
			io::mkdirs(io::path_parent(outputFile));
			return exec(cmdline)->execute();
		} else {
			return ExecutionResult::SUCCESS;
		}
	});

	buildArchive->set(LIBRARY_NAME, name);
	buildArchive->set(LIBRARY_PATH, io::path_parent(outputFile));
	buildArchive->set(OUTPUT_FILE, outputFile);

	for (auto t : objectFileTasks) {
		buildArchive->dependsOn(t);
	}

	return buildArchive;
}

task_p exe(
	std::string taskName,
	std::string name,
	std::vector<std::string> sourceFiles,
	std::vector<std::string> includeSearchDirs = std::vector<std::string>(),
	std::vector<std::string> libraryNames = std::vector<std::string>(),
	std::vector<std::string> librarySearchPaths = std::vector<std::string>(),
	std::string outputDirectory = DEFAULT_BUILD_DIR,
	std::shared_ptr<Toolchain> toolchain = Toolchain::platformDefault()
) {
	std::string outputFile(io::path_concat(outputDirectory, name));

	std::vector<task_p> objectFileTasks;
	for (auto file : sourceFiles) {
		objectFileTasks.push_back(detail::object(name, file, includeSearchDirs, outputDirectory, toolchain));
	}

	task_p link = task(taskName, [=] (Task* _) {

		std::vector<std::string> objectFiles;
		for (auto task : objectFileTasks) {
			objectFiles.push_back(task->get(OUTPUT_FILE));
		}

		std::vector<std::string> libraryFiles;
		for (const auto& lib : libraryNames) {
			libraryFiles.push_back(detail::resolveFile(toolchain->staticLibNameFromBase(lib), librarySearchPaths));
		}

		if (
			isTargetLessRecentThanFiles(outputFile, objectFiles) ||
			isTargetLessRecentThanFiles(outputFile, libraryFiles)
		) {
			std::string cmdline = toolchain->linkExeCmd(
				outputFile,
				objectFiles,
				includeSearchDirs,
				libraryNames,
				librarySearchPaths
			);

			io::mkdirs(io::path_parent(outputFile));

			return exec(cmdline)->execute();
		} else {
			return ExecutionResult::SUCCESS;
		}
	});

	link->dependsOn(objectFileTasks);

	return link;
}

} // namespace detail

task_p static_lib(
	std::string name,
	std::vector<std::string> sourceFiles,
	std::vector<std::string> includeSearchDirs = std::vector<std::string>(),
	std::string outputDirectory = DEFAULT_BUILD_DIR,
	std::shared_ptr<Toolchain> toolchain = Toolchain::platformDefault()
) {
	return detail::static_lib(name, name, sourceFiles, includeSearchDirs, outputDirectory, toolchain);
}

task_p static_lib(
	std::string name,
	task_p sourceFiles,
	task_p includeSearchDirs = emptyList(io::FILE_LIST),
	std::string outputDirectory = DEFAULT_BUILD_DIR,
	std::shared_ptr<Toolchain> toolchain = Toolchain::platformDefault()
) {

	task_p configure = task(name, [=] (Task* self){

		task_p buildArchive = detail::static_lib(
			name + ":archive",
			name,
			sourceFiles->getList(io::FILE_LIST),
			includeSearchDirs->getList(io::FILE_LIST),
			outputDirectory,
			toolchain
		);

		self->followedBy(buildArchive);
		self->followedBy(task([self, buildArchive] (Task* _) {
			self->set(LIBRARY_NAME, buildArchive->get(LIBRARY_NAME));
			self->set(LIBRARY_PATH, buildArchive->get(LIBRARY_PATH));
			self->set(OUTPUT_FILE, buildArchive->get(OUTPUT_FILE));
			return ExecutionResult::SUCCESS;
		}));

		return ExecutionResult::SUCCESS;
	});

	configure->dependsOn(sourceFiles);
	configure->dependsOn(includeSearchDirs);

	return configure;
}

class StaticLibBuilder {
public:
	builder::Value<StaticLibBuilder, std::string> name{this};
	builder::StrListFromTask<StaticLibBuilder> sourceFiles{this, io::FILE_LIST};
	builder::StrListFromTask<StaticLibBuilder> includeSearchDirs{this, io::FILE_LIST};
	builder::Value<StaticLibBuilder, std::string> outputDirectory{this, DEFAULT_BUILD_DIR};
	builder::Value<StaticLibBuilder, std::shared_ptr<Toolchain>> toolchain{this, Toolchain::platformDefault()};

    task_p build() {
        return static_lib(name, sourceFiles, includeSearchDirs, outputDirectory, toolchain);
    }
};

StaticLibBuilder static_lib() {
	return StaticLibBuilder();
}

task_p exe(
	std::string name,
	task_p sourceFiles,
	task_p includeSearchDirs = emptyList(io::FILE_LIST),
	task_p linkLibraries = emptyList(LIBRARY_NAME),
	task_p linkLibraryPaths = emptyList(LIBRARY_PATH),
	std::string outputDirectory = DEFAULT_BUILD_DIR,
	std::shared_ptr<Toolchain> toolchain = Toolchain::platformDefault()
) {
	task_p configure = task(name, [=] (Task* self) {

		task_p compile = detail::exe(
			name + ":link",
			name,
			sourceFiles->getList(io::FILE_LIST),
			includeSearchDirs->getList(io::FILE_LIST),
			linkLibraries->getList(LIBRARY_NAME),
			linkLibraryPaths->getList(LIBRARY_PATH),
			outputDirectory,
			toolchain
		);

		self->followedBy(compile);

		return ExecutionResult::SUCCESS;
	});

	configure->dependsOn(sourceFiles);
	configure->dependsOn(includeSearchDirs);
	configure->dependsOn(linkLibraries);
	configure->dependsOn(linkLibraryPaths);

	return configure;
}


class ExeBuilder {
public:
	builder::Str<ExeBuilder> name{this};
	builder::StrListFromTask<ExeBuilder> sourceFiles{this, io::FILE_LIST};
	builder::StrListFromTask<ExeBuilder> includeSearchDirs{this, io::FILE_LIST};
	builder::StrListFromTask<ExeBuilder> linkLibrary{this, LIBRARY_NAME, emptyList(LIBRARY_NAME)};
	builder::StrListFromTask<ExeBuilder> linklibrarySearchPath{this, LIBRARY_PATH, emptyList(LIBRARY_PATH)};
	builder::Str<ExeBuilder> outputDirectory{this, DEFAULT_BUILD_DIR};
	builder::Value<ExeBuilder, std::shared_ptr<Toolchain>> toolchain{this, Toolchain::platformDefault()};

	task_p build() {
		return exe(
			name,
			sourceFiles,
			includeSearchDirs,
			linkLibrary,
			linklibrarySearchPath,
			outputDirectory,
			toolchain
		);
	}
};

ExeBuilder exe() {
	return ExeBuilder();
}

} // namespace cpp
} // namespace cradle
