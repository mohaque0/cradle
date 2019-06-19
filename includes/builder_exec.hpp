#pragma once

#include <builder_main.hpp>

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
