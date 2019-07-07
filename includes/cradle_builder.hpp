/**
 * @file
 *
 * The Builder concept in general is used to simplify the creation of objects that have multiple
 * constructors or constructors of optional arguments. This file contains a number of abstractions
 * to simplify the creation of builders for @ref cradle::Task "Task"s.
 *
 * To use these classes, add public fields of the appropriate type. The classes implement
 * `operator()` so they may be called as methods (similar to typical builders) and will
 * implicitly convert to the `ValueType` specified in the template argument. Additionaly,
 * most of these classes will accept @ref cradle::task_p "task_p" objects as arguments and extract
 * appropriate string values. This reduces the boilerplate necessary to create simple builders.
 *
 * For example:
 * ```cpp
 *		class MyBuilder {
 *		public:
 *			BuilderValue<MyBuilder, std::string> name{this}
 *
 *			task_p build() {
 *				return task(name, [] { return ExecutionStatus::SUCCESS; });
 *			}
 *		}
 *
 *
 *		int myFunctionThatUsesBuilder() {
 *			auto b = MyBuilder()
 *				.name("myName")
 *				.build()
 *		}
 * ```
 */

#pragma once

#include <cradle_types.hpp>

namespace cradle {
namespace builder {

template<typename Builder, typename ValueType>
class Value {
	Builder& builder;
	ValueType value;
	bool isSet;
public:
	Value() = delete;
	Value(Builder* builder) : builder(*builder), isSet(false) {}
	Value(Builder* builder, const ValueType& defaultValue) :
		builder(*builder),
		value(defaultValue),
		isSet(true)
	{}
	Builder& operator()(const ValueType& value) {
		this->value = value;
		this->isSet = true;
		return builder;
	}
	operator const ValueType&() const {
		if (!isSet) throw new std::runtime_error("Attempting to access unset value.");
		return value;
	}
};

template<typename Builder>
class Str : public Value<Builder, std::string> {
public:
	Str() = delete;
	Str(Builder* builder) : Value<Builder, std::string>(builder) {}
	Str(Builder* builder, const std::string& defaultValue) : Value<Builder, std::string>(builder, defaultValue) {}
};

template<typename Builder, typename ValueType>
class List {
	Builder& builder;
	std::vector<ValueType> value;
	bool isSet;
public:
	List() = delete;
	List(Builder* builder) : builder(*builder), isSet(false) {}
	List(Builder* builder, std::initializer_list<ValueType> defaultValues) :
		builder(*builder),
		value(defaultValues),
		isSet(true)
	{}
	Builder& operator()(const ValueType& value) {
		this->value.push_back(value);
		this->isSet = true;
		return builder;
	}
	operator const std::vector<ValueType>&() const {
		if (!isSet) throw new std::runtime_error("Attempting to access unset value.");
		return value;
	}
};

template<typename Builder>
class StrList : public List<Builder, std::string> {
public:
	StrList() = delete;
	StrList(Builder* builder) : List<Builder, std::string>(builder) {}
	StrList(Builder* builder, std::initializer_list<std::string> defaultValues) :
		List<Builder, std::string>(builder, defaultValues)
	{}
};

template <typename Builder>
class StrFromTask {
	Builder& builder;
	std::string key;
	task_p t;
	bool isSet;

public:
	StrFromTask(Builder* builder, const std::string& key) :
		builder(*builder),
		key(key),
		isSet(false)
	{}

	StrFromTask(Builder* builder, const std::string& key, task_p defaultTask) :
		builder(*builder),
		key(key),
		t(defaultTask),
		isSet(true)
	{}

	StrFromTask(Builder* builder, std::string key, std::string defaultValue) :
		StrFromTask(
			*builder,
			key,
			[key, defaultValue] (Task* self) { self->set(key, defaultValue); return ExecutionResult::SUCCESS; }
		)
	{}

	Builder& operator() (const std::string& key, task_p newTask) {
		std::string newTaskKey = key;
		std::string origTaskKey = this->key;

		this->t = task([newTaskKey, origTaskKey, newTask] (Task* self) {
			self->set(origTaskKey, newTask->get(origTaskKey));
			return ExecutionResult::SUCCESS;
		});

		this->t->dependsOn(newTask);
	}

	operator task_p() const {
		if (!isSet) throw new std::runtime_error("Attempting to access unset value.");
		return t;
	}
};

template <typename Builder>
class StrListFromTask {
	Builder& builder;
	std::string key;
	task_p t;
	bool isSet;

	static void pushValuesToList(Task& dst, const std::string& dstKey, task_p src, const std::string& srcKey) {
		bool foundValue = false;
		if (src->has(srcKey)) {
			dst.push(dstKey, src->get(srcKey));
			foundValue = true;
		}
		if (src->hasList(srcKey)) {
			dst.push(dstKey, src->getList(srcKey));
			foundValue = true;
		}

		if (!foundValue) {
			throw std::runtime_error(
						"Attempting to get value " + srcKey +
						" from task " + (src->name().empty() ? "" : "with name \"" + src->name() + "\" ")  +
						"but it is not defined."
			);
		}
	}

public:
	StrListFromTask(Builder* builder, const std::string& key) :
		builder(*builder),
		key(key),
		isSet(false)
	{}

	StrListFromTask(Builder* builder, const std::string& key, task_p defaultTask) :
		builder(*builder),
		key(key),
		t(defaultTask),
		isSet(true)
	{}

	Builder& operator() (std::initializer_list<std::string> values) {
		return (*this)(key, listOf(key, values));
	}

	Builder& operator() (const std::string& value) {
		return (*this)(key, listOf(key, {value}));
	}

	/**
	 * Adds all the values in {@code newTask->getList(key)} if any and the single value in
	 * {@code newTask->get(key)} if any to this parameter.
	 * @brief operator ()
	 * @param key
	 * @param newTask
	 * @return
	 */
	Builder& operator() (const std::string& key, task_p newTask) {
		if (!isSet) {

			std::string newTaskKey = key;
			std::string origTaskKey = this->key;

			this->t = task([newTaskKey, origTaskKey, newTask] (Task* self) {
				StrListFromTask::pushValuesToList(*self, origTaskKey, newTask, newTaskKey);
				return ExecutionResult::SUCCESS;
			});

			this->t->dependsOn(newTask);

		} else {

			std::string newTaskKey = key;
			std::string origTaskKey = this->key;
			task_p origTask = this->t;

			this->t = task([origTaskKey, newTaskKey, origTask, newTask] (Task* self) {
				StrListFromTask::pushValuesToList(*self, origTaskKey, origTask, origTaskKey);
				StrListFromTask::pushValuesToList(*self, origTaskKey, newTask, newTaskKey);
				return ExecutionResult::SUCCESS;
			});
			this->t->dependsOn(origTask);
			this->t->dependsOn(newTask);

		}
		this->isSet = true;
		return builder;
	}

	Builder& operator() (const std::string& key, std::initializer_list<task_p> tasks) {
		for (auto newTask : tasks) {
			(*this)(key, newTask);
		}
		return builder;
	}

	operator task_p() const {
		if (!isSet) throw new std::runtime_error("Attempting to access unset value.");
		return t;
	}
};

} // namespace builder
} // namespace cradle
