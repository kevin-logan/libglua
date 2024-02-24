#pragma once

#include "glua/glua.hpp"

#include <format>

#include <jsapi.h>
#include <jsfriendapi.h>

#include <js/Array.h>
#include <js/CallArgs.h>
#include <js/CompilationAndEvaluation.h>
#include <js/Conversions.h>
#include <js/ErrorInterceptor.h>
#include <js/Initialization.h>
#include <js/Object.h>
#include <js/RootingAPI.h>
#include <js/SourceText.h>
#include <js/Value.h>

// get declarations first
#include "spidermonkey_impl/converter_declarations.hpp"

// any implementations depend on converter declarations
#include "spidermonkey_impl/any.hpp"

// class_registration_impl depends on converters and any
#include "spidermonkey_impl/class_registration_impl.hpp"

// some definitions depend on converter declarations, but some depend on class_registration_impl
#include "spidermonkey_impl/converter_definitions.hpp"

// depends on all conversion utilities
#include "spidermonkey_impl/generic_functor.hpp"

namespace glua::spidermonkey {
class backend {
public:
    static result<std::unique_ptr<backend>> create()
    {
        return do_global_init().and_then([&]() {
            return create_context().and_then([&](auto context) {
                return create_global_scope(context.value_).transform([&](auto* global_scope) {
                    return std::unique_ptr<backend> { new backend { std::move(context), global_scope } };
                });
            });
        });
    }

    struct sandbox {
        sandbox(JSContext* cx, JSObject* obj)
            : scope_(cx, obj)
            , owner_(true)
        {
        }

        sandbox(const sandbox&) = delete;
        sandbox(sandbox&& move)
            : scope_(std::move(move.scope_))
            , owner_(std::exchange(move.owner_, false))
        {
        }

        ~sandbox()
        {
            if (owner_)
                scope_.reset();
        }

        JS::PersistentRootedObject scope_;
        bool owner_;
    };

    result<sandbox> create_sandbox()
    {
        return create_global_scope(cx_.value_).transform([&](auto* obj) {
            return sandbox { cx_.value_, obj };
        });
    }

    void set_active_sandbox(sandbox* sandbox)
    {
        if (sandbox == nullptr) {
            current_scope_ = global_scope_.get();
        } else {
            current_scope_ = sandbox->scope_.get();
        }
    }

    template <typename ReturnType>
    result<ReturnType> execute_script(const std::string& code)
    {
        JSAutoRealm auto_realm { cx_.value_, current_scope_ };

        JS::SourceText<mozilla::Utf8Unit> source;
        if (!source.init(cx_.value_, code.data(), code.size(), JS::SourceOwnership::Borrowed)) {
            return unexpected("Spidermonkey failed to init source\n");
        }

        JS::CompileOptions compile_options { cx_.value_ };
        compile_options.setFileAndLine("inline", 1);

        JS::RootedScript compiled_script { cx_.value_, JS::Compile(cx_.value_, compile_options, source) };
        if (compiled_script == nullptr) {
            return unexpected("Spidermonkey failed to compile script\n");
        }

        JS::RootedValue return_value { cx_.value_ };
        if (!JS_ExecuteScript(cx_.value_, compiled_script, &return_value)) {
            return unexpected("Spidermonkey failed to execute script\n");
        }

        if constexpr (!std::same_as<ReturnType, void>) {
            return from_js<ReturnType>(cx_.value_, return_value);
        }
    }

    template <typename ReturnType, typename... ArgTypes>
    result<void> register_functor(const std::string& name, generic_functor<ReturnType, ArgTypes...>& functor)
    {
        JSAutoRealm auto_realm { cx_.value_, current_scope_ };

        JS::RootedObject scope { cx_.value_, current_scope_ };
        JS::RootedFunction js_func {
            cx_.value_,
            js::DefineFunctionWithReserved(
                cx_.value_, scope, name.data(), callback_for(functor), functor.num_args, 0)
        };
        if (js_func == nullptr) {
            return unexpected(std::format("Spidermonkey failed to define function"));
        }

        auto* obj = JS_GetFunctionObject(js_func);
        js::SetFunctionNativeReserved(obj, 0, JS::PrivateValue(&functor));

        return {};
    }

    template <typename T>
    result<T> get_global(const std::string& name)
    {
        JSAutoRealm auto_realm { cx_.value_, current_scope_ };

        JS::RootedValue prop { cx_.value_ };
        bool prop_found { false };
        JS::RootedObject scope { cx_.value_, current_scope_ };
        if (JS_HasProperty(cx_.value_, scope, name.data(), &prop_found) && prop_found && JS_GetProperty(cx_.value_, scope, name.data(), &prop)) {
            return from_js<T>(cx_.value_, prop);
        }

        return unexpected(std::format("No global property with the name {} was found", name));
    }

    template <typename T>
    result<void> set_global(const std::string& name, T value)
    {
        JSAutoRealm auto_realm { cx_.value_, current_scope_ };

        return to_js(cx_.value_, std::move(value)).and_then([&](auto v) -> result<void> {
            JS::RootedValue value_handle { cx_.value_, v };
            JS::RootedObject scope { cx_.value_, current_scope_ };
            if (!JS_SetProperty(cx_.value_, scope, name.data(), value_handle)) {
                return unexpected(std::format("Spidermonkey failed to set {} global", name));
            }
            return {};
        });
    }

    template <typename ReturnType, typename... Args>
    result<ReturnType> call_function(const std::string& name, Args&&... args)
    {
        JSAutoRealm auto_realm { cx_.value_, current_scope_ };

        return many_to_js(cx_.value_, std::forward<Args>(args)...).and_then([&](auto actual_args) -> result<ReturnType> {
            JS::RootedValue call_return { cx_.value_ };

            JS::RootedValueArray<sizeof...(args)> rooted_args {
                cx_.value_,
                std::apply(
                    [&](auto&&... unwrapped_args) {
                        return JS::ValueArray<sizeof...(unwrapped_args)> {
                            std::forward<decltype(unwrapped_args)>(unwrapped_args)...
                        };
                    },
                    std::move(actual_args))
            };
            JS::HandleValueArray call_args { rooted_args };
            JS::RootedObject scope { cx_.value_, current_scope_ };
            if (!JS_CallFunctionName(cx_.value_, scope, name.data(), call_args, &call_return)) {
                return unexpected(std::format("Spidermonkey failed to call function with name {}", name));
            }

            if constexpr (!std::same_as<ReturnType, void>) {
                return from_js<ReturnType>(cx_.value_, call_return);
            } else {
                return {};
            }
        });
    }

    template <registered_class T>
    void register_class()
    {
        JSAutoRealm auto_realm { cx_.value_, current_scope_ };

        JS::RootedObject scope { cx_.value_, current_scope_ };
        class_registration_impl<T>::do_registration(cx_.value_, scope);
        class_deregistrations_.push_back(class_registration_impl<T>::do_deregistration);
    }

    ~backend()
    {
        for (auto dereg : class_deregistrations_) {
            dereg();
        }
    }

private:
    struct global_init {
        global_init(const global_init&) = delete;
        global_init& operator=(const global_init&) = delete;
        global_init(global_init&& move)
            : responsible_(std::exchange(move.responsible_, false))
        {
        }
        global_init& operator=(global_init&& move)
        {
            if (this != &move) {
                if (responsible_) {
                    JS_ShutDown();
                }
                responsible_ = std::exchange(move.responsible_, false);
            }
            return *this;
        }

        global_init(bool success)
            : responsible_(success)
        {
        }
        ~global_init()
        {
            if (responsible_)
                JS_ShutDown();
        }

        bool responsible_;
    };

    struct context {
        context(const context&) = delete;
        context& operator=(const context&) = delete;
        context(context&& move)
            : value_(std::exchange(move.value_, nullptr))
        {
        }
        context& operator=(context&& move)
        {
            if (this != &move) {
                if (value_)
                    JS_DestroyContext(value_);
                value_ = std::exchange(move.value_, nullptr);
            }
            return *this;
        }

        context(JSContext* value)
            : value_(value)
        {
        }
        ~context()
        {
            if (value_)
                JS_DestroyContext(value_);
        }

        JSContext* value_;
    };

    static result<void> do_global_init()
    {
        static bool result { JS_Init() };
        static global_init init { result };
        if (!result) {
            return unexpected("Spidermonkey failed to init (JS_Init)");
        }

        return {};
    }

    static result<context> create_context()
    {
        JSContext* value = JS_NewContext(JS::DefaultHeapMaxBytes);
        if (value) {
            if (!JS::InitSelfHostedCode(value)) {
                return unexpected("Spidermonkey error initializing self hosted code");
            }
            return context { value };
        } else {
            return unexpected("Spidermonkey failed to create context (JS_NewContext)");
        }
    }

    static result<JSObject*> create_global_scope(JSContext* cx)
    {
        static JSClass global_object { "GlobalObject", JSCLASS_GLOBAL_FLAGS, &JS::DefaultGlobalClassOps, nullptr, nullptr, nullptr };
        JS::RealmOptions options;
        JSObject* result = JS_NewGlobalObject(cx, &global_object, nullptr, JS::FireOnNewGlobalHook, options);
        if (result == nullptr) {
            return unexpected("Spidermonkey error creating global scope");
        }

        return result;
    }

    backend(context cx, JSObject* global_scope)
        : cx_(std::move(cx))
        , global_scope_(cx_.value_, global_scope)
        , current_scope_(global_scope_.get())
    {
    }

    context cx_;
    JS::RootedObject global_scope_;
    JSObject* current_scope_;
    std::vector<void (*)()> class_deregistrations_;
};

} // namespace spidermonkey
