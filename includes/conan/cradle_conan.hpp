#pragma once

#include <cradle_builder.hpp>
#include <cradle_exec.hpp>

namespace cradle {
namespace conan {

task_p conan_install(std::string name, std::string buildDir, std::string pathToConanfile);

class ConanInstallBuilder {
public:
	builder::Str<ConanInstallBuilder> name{this};
	builder::Str<ConanInstallBuilder> buildDir{this};
	builder::Str<ConanInstallBuilder> pathToConanfile{this};

	task_p build() {
		return conan_install(name, buildDir, pathToConanfile);
	}
};

task_p conan_install(std::string name, std::string buildDir, std::string pathToConanfile) {
	return task(name, [=] (Task* self) {
		self->followedBy(exec("conan "));
		return ExecutionResult::SUCCESS;
	});
}

ConanInstallBuilder conan_install() {
	return ConanInstallBuilder();
}

} // namespace conan
} // namespace cradle
