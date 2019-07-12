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

public:
	TaskBuilder() :
		_first(task([] (Task *) { return ExecutionResult::SUCCESS; })),
		_name(_first->name())
	{}

	TaskBuilder(std::string _name) :
		_first(task(_name, [] (Task *) { return ExecutionResult::SUCCESS; })),
		_name(_name)
	{}

	TaskBuilder& name(std::string _name) {
		this->_name = _name;
		return *this;
	}

	TaskBuilder& first(std::function<ExecutionResult(Task*)> f) {
		_first = task(_name, std::move(f));
		return *this;
	}

	TaskBuilder& first(task_p t) {
		_first->dependsOn(t);
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
				for (auto& k : t->propKeys()) {
					self->set(k, t->get(k));
				}
				for (auto& k : t->listKeys()) {
					self->push(k, t->getList(k));
				}
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
			current = task([current, nextFunction] (Task *self) {
				return nextFunction.get()(current.get(), self);
			});
			current->dependsOn(prev);
		}

		return current;
	}
};

TaskBuilder task() {
	return TaskBuilder();
}

}
