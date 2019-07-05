#pragma once

#include <cradle_builder.hpp>
#include <cradle_exec.hpp>
#include <cradle_main.hpp>
#include <io/cradle_files.hpp>

#include <fstream>
#include <sstream>
#include <vector>


namespace cradle {
namespace conan {

static const std::string INCLUDEDIRS = "includedirs";
static const std::string LIBDIRS = "libdirs";
static const std::string LIBS = "libs";

task_p conan_install(
		std::string name,
		std::string installFolder,
		std::string pathToConanfile,
		std::string buildOption,
		std::vector<std::string> options,
		std::vector<std::string> settings
);

class ConanInstallBuilder {
public:
	builder::Str<ConanInstallBuilder> name{this};
	builder::Str<ConanInstallBuilder> pathToConanfile{this};
	builder::Str<ConanInstallBuilder> installFolder{this, DEFAULT_BUILD_DIR};

	/**
	 * Set the value of `--build` passed to Conan. The default is `--build=missing`.
	 */
	builder::Str<ConanInstallBuilder> buildOption{this, "missing"};

	/**
	 * Set an option passed to Conan via `-o`.
	 */
	builder::StrList<ConanInstallBuilder> option{this, {}};

	/**
	 * Set a setting passed to Conan via `-s`.
	 */
	builder::StrList<ConanInstallBuilder> setting{this, {}};

	task_p build() {
		return conan_install(name, installFolder, pathToConanfile, buildOption, option, setting);
	}
};

task_p conan_install(
		std::string name,
		std::string installFolder,
		std::string pathToConanfile,
		std::string buildOption,
		std::vector<std::string> options,
		std::vector<std::string> settings
) {
	return task(name, [=] (Task* self) {
		std::string cmd = "conan install";

		// Always use the text generator since we depend on its output.
		cmd += " -g txt ";
		cmd += " --install-folder " + installFolder;
		cmd += " --build=" + buildOption;
		cmd += " " + pathToConanfile;

		for (auto& opt : options) {
			if (!opt.empty()) {
				cmd += " -o " + opt;
			}
		}
		for (auto& setting : settings) {
			if (!setting.empty()) {
				cmd += " -s " + setting;
			}
		}

		if (exec(cmd)->execute() == ExecutionResult::FAILURE) {
			return ExecutionResult::FAILURE;
		}

		std::fstream conanbuildinfo(io::path_concat(installFolder, "conanbuildinfo.txt"));
		std::string line;
		std::string section = "";
		while (std::getline(conanbuildinfo, line)) {
			if (line.length() >= 2 && line[0] == '[' && line[line.length()-1] == ']') {
				section = line.substr(1, line.length()-2);
				continue;
			} else if (line.empty()) {
				continue;
			}

			self->push(section, line);
		}

		return ExecutionResult::SUCCESS;
	});
}

ConanInstallBuilder conan_install() {
	return ConanInstallBuilder();
}

} // namespace conan
} // namespace cradle
