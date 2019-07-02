#pragma once

#include <cradle_main.hpp>

namespace cradle {

task_p listOf(const std::string& key, std::initializer_list<std::string> items) {
    std::vector<std::string> itemVector(items);
    return task([key, items{move(itemVector)}] (Task* self) { self->push(key, items); return ExecutionResult::SUCCESS; });
}

task_p emptyList(const std::string& key) {
    return task([key] (Task* self) { self->ensureList(key); return ExecutionResult::SUCCESS; });
}

} // namespace cradle
