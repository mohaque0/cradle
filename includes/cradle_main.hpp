/**
 * @file
 */

#pragma once

#include <platform/cradle_platform_util.hpp>

#include <algorithm>
#include <cstdlib>
#include <memory>
#include <queue>
#include <sstream>
#include <vector>
#include <unordered_map>

#define build_config                          \
	namespace cradle {                        \
		std::string getBuildConfigFile() {    \
			return __FILE__;                  \
		}                                     \
	}                                         \
	void configure();                         \
	int main(int argc, char** argv) {         \
	  log("Cradle Version v0.2-alpha");       \
	  cradle::platform::platform_chdir(cradle::io::path_parent(getBuildConfigFile())); \
	  parseCmdLineArgs(argc, argv);           \
	  configure();                            \
	  executor->execute();                    \
	  return 0;                               \
	}                                         \
	void configure()

namespace cradle {

static const std::string DEFAULT_BUILD_DIR = "build";

class Executor;
class SingleThreadedExecutor;
class Task;

typedef std::shared_ptr<Task> task_p;

void log(const std::string& msg);
void log_error(const std::string& msg);

/**
 * @return The absolute path to build.cpp.
 */
std::string getBuildConfigFile();

enum class ExecutionResult {
	SUCCESS,
	FAILURE
};


class Task {
	std::string name_;
	std::vector<task_p> dependencies_;
	std::vector<task_p> followingTasks_;
	std::unordered_map<std::string, std::string> properties;
	std::unordered_map<std::string, std::vector<std::string>> lists;

public:
	Task(std::string name) : name_(name) {}
	virtual ~Task() {}

	std::string name() const { return name_; }
	std::string addr() const {
		std::stringstream buffer;
		buffer << reinterpret_cast<const void*>(this);
		return buffer.str();
	}

	void dependsOn(task_p other) { dependencies_.push_back(other); }
	void dependsOn(std::vector<task_p>& others) { dependencies_.insert(dependencies_.end(), others.begin(), others.end()); }
	void dependsOn(std::initializer_list<task_p> others) { dependencies_.insert(dependencies_.end(), others.begin(), others.end()); }
	const std::vector<task_p> dependencies() const { return dependencies_; }
	void followedBy(task_p other) { followingTasks_.push_back(other); }
	void followedBy(std::vector<task_p>& others) { followingTasks_.insert(followingTasks_.end(), others.begin(), others.end()); }
	void followedBy(std::initializer_list<task_p> others) { followingTasks_.insert(followingTasks_.end(), others.begin(), others.end()); }
	const std::vector<task_p> followingTasks() const { return followingTasks_; }

	//
	// Single-valued properties.
	//

	const std::string get(const std::string& key) const {
		if (!has(key)) {
			std::runtime_error("Attempting to get unknown key: " + key);
		}
		return properties.at(key);
	}

	void set(const std::string& key, const std::string& value) {
		properties[key] = value;
	}

	bool has(const std::string& key) const {
		return properties.find(key) != properties.end();
	}

	std::vector<std::string> propKeys() {
		std::vector<std::string> keys;
		for (auto& it : properties) {
			keys.push_back(it.first);
		}
		return keys;
	}

	//
	// Multi-valued properties.
	//

	const std::vector<std::string>& getList(const std::string& key) const {
		if (!hasList(key)) {
			std::runtime_error("Attempting to get unknown list: " + key);
		}
		return lists.at(key);
	}

	void push(const std::string& key, const std::string& value) {
		if (!hasList(key)) {
			lists[key] = std::vector<std::string>();
		}
		lists[key].push_back(value);
	}

	void push(const std::string& key, const std::vector<std::string>& values) {
		ensureList(key);
		lists[key].insert(lists[key].end(), values.begin(), values.end());
	}

	void ensureList(const std::string& key) {
		if (!hasList(key)) {
			lists[key] = std::vector<std::string>();
		}
	}

	bool hasList(const std::string& key) const {
		return lists.find(key) != lists.end();
	}

	std::vector<std::string> listKeys() {
		std::vector<std::string> keys;
		for (auto& it : lists) {
			keys.push_back(it.first);
		}
		return keys;
	}

	// Interface.
	virtual ExecutionResult execute() = 0;
};

class Executor {
	std::unordered_map<std::string, task_p> tasks_;
	std::queue<std::string> taskNamesToExecute_;

	/**
	 * Check that `toCheck` doesn't have a cyclic dependency given that we've already seen `seen`.
	 * Assumes `toCheck` was not pushed onto `seen`.
	 */
	void checkForCycles(Task* toCheck, std::vector<Task*>& seen) {
		if (std::find(seen.begin(), seen.end(), toCheck) != seen.end()) {
			log("Cycle found:");
			for (Task* t : seen) {
				log(std::string() + (t == toCheck ? "*" : "") + "\t" + t->name());
			}
			log("*\t" + toCheck->name());
			throw std::runtime_error("Cycle found.");
		}

		seen.push_back(toCheck);

		for (task_p dep : toCheck->dependencies()) {
			checkForCycles(dep.get(), seen);
		}

		seen.pop_back();
	}

public:
	virtual ~Executor() {}
	virtual ExecutionResult execute() = 0;

	std::unordered_map<std::string, task_p> tasks() {
		return tasks_;
	}

	std::queue<std::string>& taskNamesToExecute() {
		return taskNamesToExecute_;
	}

	void add(task_p t) {
		if (t->name().empty()) {
			return;
		}

		if (tasks_.find(t->name()) != tasks_.end()) {
			throw std::runtime_error("Duplicate tasks with name: " + t->name());
		}
		tasks_[t->name()] = t;
	}

	void queue(std::string name) {
		taskNamesToExecute_.push(name);
	}

	void checkForCycles() {
		for (auto& t : tasks()) {
			auto seen = std::vector<Task*>();
			checkForCycles(t.second.get(), seen);
		}
	}
};

class SingleThreadedExecutor : public Executor {

	std::unordered_map<task_p, ExecutionResult> results;

	bool wasExecuted(task_p task) const {
		return results.find(task) != results.end();
	}

	ExecutionResult getResult(task_p task) const {
		return results.at(task);
	}

	ExecutionResult setResult(task_p task, ExecutionResult result) {
		results[task] = result;
		return result;
	}

	ExecutionResult execute(task_p t) {
		checkForCycles();

		// Don't repeat a task twice.
		if (wasExecuted(t)) {
			return results[t];
		}

		// Recursively execute dependencies.
		for (auto dep : t->dependencies()) {
			if (execute(dep) == ExecutionResult::FAILURE) {
				return setResult(t, ExecutionResult::FAILURE);
			}
		}

		// Execute task.

		if (!t->name().empty()) {
			log("Executing: " + t->name());
		}

		auto result = t->execute();

		if (result == ExecutionResult::FAILURE) {
			return ExecutionResult::FAILURE;
		}

		// Recursively execute followers.
		for (auto f : t->followingTasks()) {
			if (execute(f) == ExecutionResult::FAILURE) {
				return setResult(t, ExecutionResult::FAILURE);
			}
		}

		return setResult(t, ExecutionResult::SUCCESS);
	}

public:
	ExecutionResult execute() override {
		while (!taskNamesToExecute().empty()) {
			auto name = taskNamesToExecute().front();
			taskNamesToExecute().pop();

			bool executed = false;
			for (auto& t : tasks()) {
				if (t.first == name) {
					auto result = execute(t.second);
					executed = true;

					if (result == ExecutionResult::FAILURE) {
						return ExecutionResult::FAILURE;
					}
				}
			}

			if (!executed) {
				throw std::runtime_error("Unknown task: " + name);
			}
		}
		return ExecutionResult::SUCCESS;
	}
};

static std::unique_ptr<Executor> executor = std::make_unique<SingleThreadedExecutor>();

void parseCmdLineArgs(int argc, char** argv) {
	for (int i = 1; i < argc; ++i) {
		executor->queue(std::string(argv[i]));
	}
}

template <typename F>
class FunctionTask : public Task {
	F f;

public:
	FunctionTask(std::string name, F&& f) : Task(name), f(std::move(f)) {}
	ExecutionResult execute() override {
		return f(this);
	}
};

/**
 * Create a task and add it to the executor with the given name.
 *
 * @tparam F This is always a function type equivalent to (Task* Self) -> ExecutionResult.
 *
 * @param f This is a function of type (Task* self) -> ExecutionResult.
 *          The function represents the behavior of the task and should return the appropriate
 *          result representing the results of execution.
 *          The `self` argument passed to `f` is always a pointer to the created task object.
 */
template <typename F>
task_p task(std::string name, F&& f) {
	task_p t = std::make_shared<FunctionTask<F>>(name, std::move(f));
	executor->add(t);
	return t;
}

/**
 * Create a task with no name. It will not be added to the executor.
 *
 * @tparam F This is always a function type equivalent to (Task* Self) -> ExecutionResult.
 *
 * @param f This is a function of type (Task* self) -> ExecutionResult.
 *          The function represents the behavior of the task and should return the appropriate
 *          result representing the results of execution.
 *          The `self` argument passed to `f` is always a pointer to the created task object.
 */
template <typename F>
task_p task(F f) {
	return std::make_shared<FunctionTask<F>>("", std::move(f));
}


void log(const std::string& msg) {
	printf("%s\n", msg.c_str());
}

void log_error(const std::string& msg) {
	printf("ERROR: %s\n", msg.c_str());
}

} // namespace cradle


