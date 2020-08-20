/**
 * @file
 */

#pragma once

#include <io/cradle_files.hpp>
#include <cradle_main.hpp>

namespace cradle {

task_p exec(std::string name, std::string wd, std::string cmd) {
	return task(name, [wd,cmd] (Task* self) -> ExecutionResult {
		log(cmd.c_str());
		cradle::platform::platform_chdir(wd);
		auto ret = system(cmd.c_str()) > 0 ? ExecutionResult::FAILURE : ExecutionResult::SUCCESS;
		cradle::platform::platform_chdir(cradle::io::path_parent(getBuildConfigFile()));
		return ret;
	});
}

task_p exec(std::string name, std::string cmd) {
	return task(name, [cmd] (Task* self) -> ExecutionResult {
		log(cmd.c_str());
		return system(cmd.c_str()) > 0 ? ExecutionResult::FAILURE : ExecutionResult::SUCCESS;
	});
}

task_p exec(std::string cmd) {
	return task([cmd] (Task* self) -> ExecutionResult {
		log(cmd.c_str());
		return system(cmd.c_str()) > 0 ? ExecutionResult::FAILURE : ExecutionResult::SUCCESS;
	});
}

} // namespace cradle
