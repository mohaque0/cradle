#pragma once

#include <builder_main.hpp>
#include <builder_platform.hpp>
#include <memory>
#include <string>

namespace cpp {

class EnvToolchain;

namespace detail {

	static const std::string AR_ENV_VAR = "AR";
	static const std::string CXX_ENV_VAR = "CXX";

#ifdef PLATFORM_LINUX
	static const std::string DEFAULT_AR = "ar";
	static const std::string DEFAULT_CXX = "g++";
#endif

	std::string getEnvOrDefault(const std::string& envVar, const std::string& defaultValue) {
		char* value = std::getenv(envVar.c_str());
		if (value == NULL) {
			return defaultValue;
		}
		return value;
	}

	std::string listToArgs(const std::string& prefix, const std::vector<std::string>& items) {
		std::string cmdline = "";
		for (auto i : items) {
			cmdline += " " + prefix + i;
		}
		return cmdline + " ";
	}

	std::string listToArgs(const std::vector<std::string>& items) {
		return listToArgs("", items);
	}

} // detail

class Toolchain {
protected:
	std::vector<std::string> compileFlags;
	std::vector<std::string> linkFlags;
	std::vector<std::string> staticLibFlags;

public:
	virtual ~Toolchain() {}

	void addCompileFlags(const std::vector<std::string>& flags) {
		compileFlags.insert(compileFlags.end(), flags.begin(), flags.end());
	}

	void addLinkFlags(const std::vector<std::string>& flags) {
		linkFlags.insert(compileFlags.end(), flags.begin(), flags.end());
	}

	void addStaticLibFlags(const std::vector<std::string>& flags) {
		staticLibFlags.insert(compileFlags.end(), flags.begin(), flags.end());
	}

	virtual std::string objectFileNameFromBase(const std::string& base) = 0;
	virtual std::string staticLibNameFromBase(const std::string& base) = 0;

	virtual std::string compileObjectCmd(
		std::string outputFilePath,
		std::string inputFileName,
		std::vector<std::string> includeSearchDirs,
		std::vector<std::string> flags = std::vector<std::string>()
	) = 0;

	virtual std::string linkExeCmd(
		std::string outputFilePath,
		std::vector<std::string> objectFiles,
        std::vector<std::string> includeSearchDirs,
		std::vector<std::string> linkLibraryNames,
		std::vector<std::string> librarySearchPaths,
		std::vector<std::string> flags = std::vector<std::string>()
	) = 0;

	virtual std::string buildStaticLibCmd(
		std::string outputFilePath,
		std::vector<std::string> objectFiles,
		std::vector<std::string> flags = std::vector<std::string>()
	) = 0;

	static std::shared_ptr<Toolchain> platformDefault();
};

class GccClangCompatibleToolchain : public Toolchain {
	std::string archiver;
	std::string compiler;

public:
	GccClangCompatibleToolchain(std::string archiver, std::string compiler) :
		archiver(archiver),
		compiler(compiler)
	{}

	std::string objectFileNameFromBase(const std::string& base) override {
		return base + ".o";
	}

	std::string staticLibNameFromBase(const std::string& base) override {
		return "lib" + base + ".a";
	}

	std::string compileObjectCmd(
		std::string outputFileName,
		std::string inputFileName,
		std::vector<std::string> includeSearchDirs,
		std::vector<std::string> flags
	) override {
		std::string cmdline = compiler;
		cmdline += detail::listToArgs(compileFlags);
		cmdline += detail::listToArgs(flags);
		cmdline += " -c ";
		cmdline += inputFileName;
		cmdline += detail::listToArgs("-I", includeSearchDirs);
		cmdline += " -o " + outputFileName;
		return cmdline;
	}

	std::string linkExeCmd(
		std::string outputFileName,
		std::vector<std::string> objectFiles,
        std::vector<std::string> includeSearchDirs,
		std::vector<std::string> linkLibraryNames,
		std::vector<std::string> librarySearchPaths,
		std::vector<std::string> flags
	) override {
		std::string cmdline = compiler;
		cmdline += detail::listToArgs("-I", includeSearchDirs);
		cmdline += detail::listToArgs("-L", librarySearchPaths);
		cmdline += detail::listToArgs(objectFiles);
		cmdline += detail::listToArgs("-l", linkLibraryNames);
		cmdline += detail::listToArgs(linkFlags);
		cmdline += detail::listToArgs(flags);
		cmdline += " -o " + outputFileName;
		return cmdline;
	}

	std::string buildStaticLibCmd(
		std::string outputFileName,
		std::vector<std::string> objectFiles,
		std::vector<std::string> flags
	) override {
		std::string cmdline = archiver;
		cmdline += detail::listToArgs(staticLibFlags);
		cmdline += " rcs ";
		cmdline += outputFileName;
		cmdline += detail::listToArgs(objectFiles);
		return cmdline;
	}
};


class MSVCToolchain : public Toolchain {
	std::string archiver;
	std::string compiler;
    std::string linker;

public:
    MSVCToolchain() :
        archiver("lib"),
        compiler("cl"),
        linker("link")
	{}

	std::string objectFileNameFromBase(const std::string& base) override {
		return base + ".obj";
	}

	std::string staticLibNameFromBase(const std::string& base) override {
		return base + ".lib";
	}

	std::string compileObjectCmd(
		std::string outputFileName,
		std::string inputFileName,
		std::vector<std::string> includeSearchDirs,
		std::vector<std::string> flags
	) override {
		std::string cmdline = compiler;
		cmdline += detail::listToArgs(compileFlags);
		cmdline += detail::listToArgs(flags);
		cmdline += " /c ";
		cmdline += inputFileName;
		cmdline += detail::listToArgs("/I", includeSearchDirs);
		cmdline += " /Fo" + outputFileName;
		return cmdline;
	}

	std::string linkExeCmd(
		std::string outputFileName,
		std::vector<std::string> objectFiles,
        std::vector<std::string> includeSearchDirs,
		std::vector<std::string> linkLibraryNames,
		std::vector<std::string> librarySearchPaths,
		std::vector<std::string> flags
	) override {
		std::string cmdline = linker;
		cmdline += detail::listToArgs("/LIBPATH:", librarySearchPaths);
		cmdline += detail::listToArgs(objectFiles);

		for (auto name : linkLibraryNames) {
			cmdline += " " + staticLibNameFromBase(name);
		}

		cmdline += detail::listToArgs(linkFlags);
		cmdline += detail::listToArgs(flags);
        cmdline += " /OUT:" + outputFileName + ".exe";

		return cmdline;
	}

	std::string buildStaticLibCmd(
		std::string outputFileName,
		std::vector<std::string> objectFiles,
		std::vector<std::string> flags
	) override {
		std::string cmdline = archiver;
		cmdline += detail::listToArgs(staticLibFlags);
		cmdline += detail::listToArgs(flags);
        cmdline += " ";
        cmdline += "/OUT:" + outputFileName;
		cmdline += detail::listToArgs(objectFiles);
		return cmdline;
	}
};

std::shared_ptr<Toolchain> Toolchain::platformDefault() {
	#ifdef PLATFORM_WINDOWS
        return std::make_shared<MSVCToolchain>();
	#else
		return std::make_shared<GccClangCompatibleToolchain>(
			detail::getEnvOrDefault(detail::AR_ENV_VAR, detail::DEFAULT_AR),
			detail::getEnvOrDefault(detail::CXX_ENV_VAR, detail::DEFAULT_CXX)
		);
	#endif
}

} // namespace cpp
