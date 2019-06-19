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

	std::string listToArgs(const std::vector<std::string>& items, const std::string& prefix = "") {
		std::string cmdline = "";
		for (auto i : items) {
			cmdline += " " + prefix + i;
		}
		return cmdline + " ";
	}

} // detail

class Toolchain {
public:
	virtual ~Toolchain() {}

	virtual std::string objectFileNameFromBase(const std::string& base) = 0;
	virtual std::string staticLibNameFromBase(const std::string& base) = 0;

	virtual std::string compileObjectCmd(
		std::string outputFilePath,
		std::string inputFileName,
		std::vector<std::string> includeSearchDirs
	) = 0;

	virtual std::string compileExeCmd(
		std::string outputFilePath,
		std::vector<std::string> objectFiles,
        std::vector<std::string> includeSearchDirs,
		std::vector<std::string> linkLibraryNames,
		std::vector<std::string> librarySearchPaths
	) = 0;

	virtual std::string compileStaticLibCmd(
		std::string outputFilePath,
		std::vector<std::string> objectFiles
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
		std::vector<std::string> includeSearchDirs
	) override {
		std::string cmdline = compiler;
		cmdline += " -c ";
		cmdline += inputFileName;

		for (auto name : includeSearchDirs) {
			cmdline += " -I" +  name;
		}

		cmdline += " -o " + outputFileName;

		return cmdline;
	}

	std::string compileExeCmd(
		std::string outputFileName,
		std::vector<std::string> objectFiles,
        std::vector<std::string> includeSearchDirs,
		std::vector<std::string> linkLibraryNames,
		std::vector<std::string> librarySearchPaths
	) override {
		std::string cmdline = compiler;

		for (auto name : includeSearchDirs) {
			cmdline += " -I" + name;
		}

		for (auto name : librarySearchPaths) {
			cmdline += " -L" + name;
		}

		for (auto name : objectFiles) {
			cmdline += " " + name;
		}

		for (auto name : linkLibraryNames) {
			cmdline += " -l" + name;
		}

		cmdline += " -o " + outputFileName;

		return cmdline;
	}

	std::string compileStaticLibCmd(
		std::string outputFileName,
		std::vector<std::string> objectFiles
	) override {
		std::string cmdline = archiver;

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
		std::vector<std::string> includeSearchDirs
	) override {
		std::string cmdline = compiler;
		cmdline += " /c ";
		cmdline += inputFileName;

		for (auto name : includeSearchDirs) {
			cmdline += " /I" +  name;
		}

		cmdline += " /Fo" + outputFileName;

		return cmdline;
	}

	std::string compileExeCmd(
		std::string outputFileName,
		std::vector<std::string> objectFiles,
        std::vector<std::string> includeSearchDirs,
		std::vector<std::string> linkLibraryNames,
		std::vector<std::string> librarySearchPaths
	) override {
        std::string cmdline = linker;

		for (auto name : librarySearchPaths) {
			cmdline += " /LIBPATH:" + name;
		}

		for (auto name : objectFiles) {
			cmdline += " " + name;
		}

		for (auto name : linkLibraryNames) {
            cmdline += " " + name + ".lib";
		}

        cmdline += " /OUT:" + outputFileName + ".exe";

		return cmdline;
	}

	std::string compileStaticLibCmd(
		std::string outputFileName,
		std::vector<std::string> objectFiles
	) override {
		std::string cmdline = archiver;

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
