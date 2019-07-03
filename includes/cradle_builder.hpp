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

template <typename Builder>
class StrFromTask : public Value<Builder, task_p> {
public:
	StrFromTask(Builder* builder, std::string key, std::string defaultValue) :
		Value<Builder, task_p>(
			*builder,
			[key, defaultValue] (Task* self) { self->set(key, defaultValue); return ExecutionResult::SUCCESS; }
		)
	{}
};

template <typename Builder>
class StrListFromTask {
	Builder& builder;
	std::string key;
	task_p t;
	bool isSet;
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
		return (*this)(listOf(key, values));
	}
	Builder& operator() (const std::string& value) {
		return (*this)(listOf(key, {value}));
	}

	/**
	 * Adds all the values in {@code t->getList(key)} to this parameter.
	 * @brief operator ()
	 * @param t
	 * @return
	 */
	Builder& operator() (task_p t) {
		if (!isSet) {
			this->t = t;
		} else {
			std::string k = key;
			task_p t0 = this->t;
			this->t = task([k, t0, t] (Task* self) {
				self->push(k, t0->getList(k));
				self->push(k, t->getList(k));
				return ExecutionResult::SUCCESS;
			});
			this->t->dependsOn(t0);
			this->t->dependsOn(t);
		}
		this->isSet = true;
		return builder;
	}
	operator task_p() const {
		if (!isSet) throw new std::runtime_error("Attempting to access unset value.");
		return t;
	}
};

} // namespace builder
} // namespace cradle
