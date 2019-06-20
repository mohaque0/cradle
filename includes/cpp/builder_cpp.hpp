#pragma once

#include <builder_exec.hpp>
#include <builder_main.hpp>
#include <builder_types.hpp>
#include <cpp/builder_cpp_toolchain.hpp>
#include <io/builder_files.hpp>
#include <io/builder_stat.hpp>

#include <time.h>

namespace cradle {
namespace cpp {

static const std::string OUTPUT_FILE = "OUTPUT_FILE";
static const std::string LIBRARY_NAME = "LIBRARY_NAME";

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

bool needsRebuildAgainstHeadersInDir(const struct stat& targetFileStat, const std::string& path) {
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
			if (needsRebuildAgainstHeadersInDir(targetFileStat, file.path)) {
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

bool needsRebuild(const std::string& targetFile, const std::string& sourceFile, const std::vector<std::string>& includeSearchDirs) {
	if (!io::exists(targetFile)) {
		return true;
	}

	const struct stat targetFileStat = io::getStat(targetFile);
	const struct stat sourceFileStat = io::getStat(sourceFile);

	if (difftime(targetFileStat.st_mtime, sourceFileStat.st_mtime) < 0) {
		return true;
	}

	for (auto& dir : includeSearchDirs) {
		if (needsRebuildAgainstHeadersInDir(targetFileStat, dir)) {
			return true;
		}
	}

	return false;
}

bool needsRebuild(const std::string& targetFile, const std::vector<std::string>& files) {
	if (!io::exists(targetFile)) {
		return true;
	}

	const struct stat targetFileStat = io::getStat(targetFile);

	for (auto& sourceFile : files) {
		const struct stat sourceFileStat = io::getStat(sourceFile);

		if (difftime(targetFileStat.st_mtime, sourceFileStat.st_mtime) < 0) {
			return true;
		}
	}

	return false;
}


task_p object(
	std::string rootTaskName,
	std::string filePath,
	std::vector<std::string> includeSearchDirs = std::vector<std::string>(),
	std::string outputDirectory = "build",
	std::shared_ptr<Toolchain> toolchain = Toolchain::platformDefault()
) {
	auto configure = task(rootTaskName + ':' + filePath + ":compile", [=] (Task* self) {
		std::string outputFile = io::path_concat(outputDirectory, toolchain->objectFileNameFromBase(filePath));
		self->set(OUTPUT_FILE, outputFile);

		if (needsRebuild(outputFile, filePath, includeSearchDirs)) {

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
	std::string outputDirectory = "build",
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

		if (needsRebuild(outputFile, objectFiles)) {
			std::string cmdline = toolchain->buildStaticLibCmd(outputFile, objectFiles);
			io::mkdirs(io::path_parent(outputFile));
			return exec(cmdline)->execute();
		} else {
			return ExecutionResult::SUCCESS;
		}
	});

	buildArchive->set(LIBRARY_NAME, name);
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
	std::vector<task_p> linkLibraryTasks = std::vector<task_p>(),
	std::vector<std::string> librarySearchPathList = std::vector<std::string>(),
	std::string outputDirectory = "build",
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

		std::vector<std::string> linkLibraries;
		std::vector<std::string> linkLibraryNames;
		std::vector<std::string> linkLibraryPaths;
		for (auto task : linkLibraryTasks) {
			linkLibraries.push_back(task->get(OUTPUT_FILE));
			linkLibraryNames.push_back(task->get(LIBRARY_NAME));
			linkLibraryPaths.push_back(io::path_parent(task->get(OUTPUT_FILE)));
		}

		std::vector<std::string> librarySearchPaths = std::vector<std::string>(librarySearchPathList);
		librarySearchPaths.insert(librarySearchPaths.end(), linkLibraryPaths.begin(), linkLibraryPaths.end());

		if (
			needsRebuild(outputFile, objectFiles) ||
			needsRebuild(outputFile, linkLibraries)
		) {
			std::string cmdline = toolchain->linkExeCmd(
				outputFile,
				objectFiles,
				includeSearchDirs,
				linkLibraryNames,
				librarySearchPaths
			);

			io::mkdirs(io::path_parent(outputFile));

			return exec(cmdline)->execute();
		} else {
			return ExecutionResult::SUCCESS;
		}
	});

	for (auto t : objectFileTasks) {
		link->dependsOn(t);
	}
	for (auto t : linkLibraryTasks) {
		link->dependsOn(t);
	}

	return link;
}

} // namespace detail

task_p static_lib(
	std::string name,
	std::vector<std::string> sourceFiles,
	std::vector<std::string> includeSearchDirs = std::vector<std::string>(),
	std::string outputDirectory = "build",
	std::shared_ptr<Toolchain> toolchain = Toolchain::platformDefault()
) {
	return detail::static_lib(name, name, sourceFiles, includeSearchDirs, outputDirectory, toolchain);
}

task_p static_lib(
	std::string name,
	task_p sourceFiles,
	task_p includeSearchDirs = emptyList(io::FILE_LIST),
	std::string outputDirectory = "build",
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
			self->set(OUTPUT_FILE, buildArchive->get(OUTPUT_FILE));
			return ExecutionResult::SUCCESS;
		}));

		return ExecutionResult::SUCCESS;
	});

	configure->dependsOn(sourceFiles);
	configure->dependsOn(includeSearchDirs);

	return configure;
}

task_p exe(
	std::string name,
	std::vector<std::string> sourceFiles,
	std::vector<std::string> includeSearchDirs = std::vector<std::string>(),
	std::vector<task_p> linkLibraryTasks = std::vector<task_p>(),
	std::vector<std::string> librarySearchPathList = std::vector<std::string>(),
	std::string outputDirectory = "build",
	std::shared_ptr<Toolchain> toolchain = Toolchain::platformDefault()
) {
	return detail::exe(name, name, sourceFiles, includeSearchDirs, linkLibraryTasks, librarySearchPathList, outputDirectory, toolchain);
}

task_p exe(
	std::string name,
	task_p sourceFiles,
	task_p includeSearchDirs = emptyList(io::FILE_LIST),
	std::vector<task_p> linkLibraryTasks = std::vector<task_p>(),
	task_p librarySearchPathList = emptyList(io::FILE_LIST),
	std::string outputDirectory = "build",
	std::shared_ptr<Toolchain> toolchain = Toolchain::platformDefault()
) {
	task_p configure = task(name, [=] (Task* self) {

		task_p compile = detail::exe(
			name + ":link",
			name,
			sourceFiles->getList(io::FILE_LIST),
			includeSearchDirs->getList(io::FILE_LIST),
			linkLibraryTasks,
			librarySearchPathList->getList(io::FILE_LIST),
			outputDirectory,
			toolchain
		);

		self->followedBy(compile);

		return ExecutionResult::SUCCESS;
	});

	configure->dependsOn(sourceFiles);
	configure->dependsOn(includeSearchDirs);
	for (auto t : linkLibraryTasks) {
		configure->dependsOn(t);
	}
	configure->dependsOn(librarySearchPathList);

	return configure;
}

} // namespace cpp
} // namespace cradle
