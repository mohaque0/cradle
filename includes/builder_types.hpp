#pragma once

#include <builder_main.hpp>

//
// Helper for creating builders.
//

#define task_arg_d(type, name)    \
	public:                       \
	  type name##_;               \
	  auto& name(type value) { this->name##_ = (value); return *this; }

#define task_arg_v(type, name, initial) \
	public:                             \
	  type name##_ = (initial);         \
	  auto& name(type value) { this->name##_ = (value); return *this; }

namespace cradle {

task_p listOf(const std::string& key, std::initializer_list<std::string> items) {
	std::vector<std::string> itemVector(items);
	return task([key, items{move(itemVector)}] (Task* self) { self->push(key, items); return ExecutionResult::SUCCESS; });
}

task_p emptyList(const std::string& key) {
	return task([key] (Task* self) { self->ensureList(key); return ExecutionResult::SUCCESS; });
}

} // namespace cradle
