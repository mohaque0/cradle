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
 * @code{.cpp}
 *	class MyBuilder {
 *	public:
 *		BuilderValue<MyBuilder, std::string> name{this}  //
 *
 *		task_p build() {
 *			return task(name, [] { return ExecutionStatus::SUCCESS; });
 *		}
 *	}
 *
 *
 *	int myFunctionThatUsesBuilder() {
 *		auto b = MyBuilder()
 *			.name("myName")
 *			.build()
 *	}
 * @endcode
 */

#pragma once

#include <cradle_types.hpp>

namespace cradle {


template<typename Builder, typename ValueType>
class BuilderValue {
	Builder& builder;
	ValueType value;
	bool isSet;
public:
	BuilderValue() = delete;
	BuilderValue(Builder* builder) : builder(*builder), isSet(false) {}
	BuilderValue(Builder* builder, const ValueType& defaultValue) :
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

template<typename Builder, typename ValueType>
class BuilderList {
	Builder& builder;
	std::vector<ValueType> value;
	bool isSet;
public:
	BuilderList() = delete;
	BuilderList(Builder* builder) : builder(*builder), isSet(false) {}
	BuilderList(Builder* builder, std::initializer_list<ValueType> defaultValues) :
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
class BuilderStr : public BuilderValue<Builder, task_p> {
public:
	BuilderStr(Builder* builder, std::string key, std::string defaultValue) :
		BuilderValue<Builder, task_p>(
			*builder,
			[key, defaultValue] (Task* self) { self->set(key, defaultValue); return ExecutionResult::SUCCESS; }
		)
	{}
};

template <typename Builder>
class BuilderStrList {
	Builder& builder;
	std::string key;
	task_p t;
	bool isSet;
public:
	BuilderStrList(Builder* builder, const std::string& key) :
		builder(*builder),
		key(key),
		isSet(false)
	{}
	BuilderStrList(Builder* builder, const std::string& key, task_p defaultTask) :
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

} // namespace cradle