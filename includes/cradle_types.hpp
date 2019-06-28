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


template<typename Builder, typename ValueType>
class builder_val {
    Builder& builder;
    ValueType value;
    bool isSet;
public:
    builder_val() = delete;
    builder_val(Builder* builder) : builder(*builder), isSet(false) {}
    builder_val(Builder* builder, const ValueType& defaultValue) :
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
class builder_list {
    Builder& builder;
    std::vector<ValueType> value;
    bool isSet;
public:
    builder_list() = delete;
    builder_list(Builder* builder) : builder(*builder), isSet(false) {}
    builder_list(Builder* builder, std::initializer_list<ValueType> defaultValues) :
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
class builder_str : public builder_val<Builder, task_p> {
public:
    builder_str(Builder* builder, std::string key, std::string defaultValue) :
        builder_val<Builder, task_p>(
            *builder,
            [key, defaultValue] (Task* self) { self->set(key, defaultValue); return ExecutionResult::SUCCESS; }
        )
    {}
};

template <typename Builder>
class builder_str_list {
    Builder& builder;
    std::string key;
    task_p t;
    bool isSet;
public:
    builder_str_list(Builder* builder, const std::string& key) :
        builder(*builder),
        key(key),
        isSet(false)
    {}
    builder_str_list(Builder* builder, const std::string& key, task_p defaultTask) :
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
