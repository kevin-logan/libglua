#pragma once

#include <memory>
#include <vector>

#include "any.hpp"
#include "generic_functor.hpp"
#include "helpers.hpp"
#include "registration.hpp"
#include "result.hpp"

namespace glua {

template <typename Backend>
class instance {
public:
    template <typename... CreateArgs>
    static result<instance> create(CreateArgs&&... create_args)
    {
        return Backend::create(std::forward<CreateArgs>(create_args)...).transform([&](auto backend_ptr) { return instance { std::move(backend_ptr) }; });
    }

    result<typename Backend::sandbox> create_sandbox() { return backend_ptr_->create_sandbox(); }
    auto set_active_sandbox(typename Backend::sandbox* s) { backend_ptr_->set_active_sandbox(s); }

    template <typename ReturnType>
    result<ReturnType> execute_script(const std::string& code)
    {
        if constexpr (std::same_as<ReturnType, void>)
            backend_ptr_->template execute_script<ReturnType>(code);
        else
            return backend_ptr_->template execute_script<ReturnType>(code);
    }

    template <typename F>
    result<void> register_functor(const std::string& name, F functor)
    {
        auto generic_functor_ptr = create_generic_functor(std::move(functor));
        auto result = register_functor(name, *generic_functor_ptr);
        registered_functors_.push_back(std::move(generic_functor_ptr));
        return result;
    }

    template <typename ReturnType, typename... ArgTypes>
    result<void> register_functor(const std::string& name, generic_functor<ReturnType, ArgTypes...>& functor)
    {
        return backend_ptr_->register_functor(name, functor);
    }

    template <typename T>
    result<T> get_global(const std::string& name)
    {
        return backend_ptr_->template get_global<T>(name);
    }

    template <typename T>
    result<void> set_global(const std::string& name, T value)
    {
        return backend_ptr_->set_global(name, std::move(value));
    }

    template <typename ReturnType, typename... Args>
    result<ReturnType> call_function(const std::string& name, Args&&... args)
    {
        return backend_ptr_->template call_function<ReturnType>(name, std::forward<Args>(args)...);
    }

    template <registered_class T>
    void register_class()
    {
        return backend_ptr_->template register_class<T>();
    }

private:
    instance(std::unique_ptr<Backend> backend_ptr)
        : backend_ptr_(std::move(backend_ptr))
    {
    }

    std::unique_ptr<Backend> backend_ptr_;
    std::vector<std::unique_ptr<functor>> registered_functors_;
};
} // namespace glua