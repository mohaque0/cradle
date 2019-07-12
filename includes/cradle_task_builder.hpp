#pragma once

#include <cradle_main.hpp>

#include <functional>
#include <vector>

namespace cradle {
namespace detail {

class SubsequentTask {
	std::shared_ptr<std::function<ExecutionResult(Task*,Task*)>> f;

public:
	SubsequentTask(std::function<ExecutionResult(Task*,Task*)> f) :
		f(std::make_shared<std::function<ExecutionResult(Task*,Task*)>>(f))
	{}

	std::function<ExecutionResult(Task*,Task*)>& get() const {
		return *f;
	}
};

} // detail

class TaskBuilder {
	task_p _first;
	std::string _name;
	std::vector<detail::SubsequentTask> followers;

	static void copyNonconflictingKeys(Task* dst, Task* src) {
		for (auto& k : src->propKeys()) {
			if (!dst->has(k)) {
				dst->set(k, src->get(k));
			}
		}
		for (auto& k : src->listKeys()) {
			if (!dst->has(k)) {
				dst->push(k, src->getList(k));
			}
		}
	}

public:
	TaskBuilder() :
		_first(task([] (Task *) { return ExecutionResult::SUCCESS; }))
	{}

	TaskBuilder& name(std::string _name) {
		this->_name = _name;
		return *this;
	}

	TaskBuilder& first(std::function<ExecutionResult(Task*)> f) {
		_first = task(std::move(f));
		return *this;
	}

	TaskBuilder& first(task_p t) {
		_first = t;
		return *this;
	}

	/**
	 * Takes a function of type (Task* prev, Task* self) -> ExecutionResult.
	 * This will be passed the preceding task as `prev` and itself as `self`.
	 */
	TaskBuilder& then(std::function<ExecutionResult(Task*,Task*)> t) {
		followers.push_back(t);
		return *this;
	}

	TaskBuilder& then(std::function<ExecutionResult(Task*)> f) {
		followers.push_back(detail::SubsequentTask([f] (Task*, Task* self) { return f(self); }));
		return *this;
	}

	TaskBuilder& then(task_p t) {
		followers.push_back(detail::SubsequentTask([t] (Task*, Task* self) {
			auto retVal = t->execute();
			if (retVal == ExecutionResult::SUCCESS) {
				TaskBuilder::copyNonconflictingKeys(self, t.get());
			}
			return retVal;
		}));
		return *this;
	}

	task_p build() {
		task_p current = _first;
		std::vector<detail::SubsequentTask> subsequentTasks = followers;

		for (auto nextFunction : subsequentTasks) {
			task_p prev = current;
			current = task([prev, nextFunction] (Task *self) {
				ExecutionResult result = nextFunction.get()(prev.get(), self);
				TaskBuilder::copyNonconflictingKeys(self, prev.get());
				return result;
			});
			current->dependsOn(prev);
		}

		// Create final task with name.
		task_p retVal = task(_name, [current] (Task* self) {
			TaskBuilder::copyNonconflictingKeys(self, current.get());
			return ExecutionResult::SUCCESS;
		});
		retVal->dependsOn(current);

		return retVal;
	}
};

TaskBuilder task() {
	return TaskBuilder();
}

}
