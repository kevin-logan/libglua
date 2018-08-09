#include "glua/GluaBase.h"
#include "glua/GluaBaseHelperTemplates.h"

namespace kdk::glua
{
/** DEFERRED ARGUMENT ArgumentStack CONTRACT IMPLEMENTATION **/
template<typename Type>
auto GluaBase::deferred_argument_get(const ICallable* callable, size_t index) -> Type
{
    auto* glua_ptr = static_cast<GluaBase*>(callable->GetImplementationData());
    return GluaBase::get<Type>(glua_ptr, glua_ptr->transformFunctionParameterIndex(index));
}
template<typename Type>
auto GluaBase::deferred_argument_push(const ICallable* callable, Type&& value) -> void
{
    auto* glua_ptr = static_cast<GluaBase*>(callable->GetImplementationData());
    GluaBase::push(glua_ptr, std::forward<Type>(value));
}
/*************************************************************/

template<typename Type>
auto GluaBase::get(GluaBase* glua, int index) -> Type
{
    if constexpr (HasCustomGetter<Type>::value)
    {
        std::optional<Type> result;
        glua_get(glua, index, result);

        if (!result.has_value())
        {
            throw exceptions::GluaBaseException("Custom glua_get did not populate optional");
        }

        return std::move(result.value());
    }

    if constexpr (GluaCanResolve<Type>::value)
    {
        return GluaResolver<Type>::get(glua, index);
    }

    return glua->getRegisteredType<Type>(index);
}

template<typename Type>
auto GluaBase::push(GluaBase* glua, Type&& value) -> void
{
    // first check for custom pusher of actual type, not base type
    if constexpr (HasCustomPusher<Type>::value)
    {
        glua_push(glua, std::forward<Type>(value));
    }
    else if constexpr (GluaBaseCanPush<Type>::value)
    {
        glua->push(std::forward<Type>(value));
    }
    else if constexpr (IsReferenceWrapper<Type>::value && GluaBaseCanPush<typename RegistryType<Type>::underlying_type>::value)
    {
        // reference_wrapper to supported type, just push it's value
        glua->push(value.get());
    }
    else
    {
        // some kind of complex type
        glua->pushRegisteredType(std::forward<Type>(value));
    }
}

template<typename T>
auto GluaBase::Pop() -> T
{
    auto val = GluaBase::get<T>(this, -1);
    popOffStack(1);

    return val;
}

template<typename T>
auto GluaBase::SetGlobal(const std::string& name, T value) -> void
{
    GluaBase::push(this, std::move(value));
    setGlobalFromStack(name, -1);
    popOffStack(1); // pop off the value we pushed
}

template<typename T>
auto GluaBase::GetGlobal(const std::string& name) -> T
{
    pushGlobal(name);
    auto val = GluaBase::get<T>(this, -1);
    popOffStack(1);

    return val;
}

template<typename Functor, typename... Params>
auto GluaBase::CreateGluaCallable(Functor&& f) -> Callable
{
    return Callable{std::make_unique<GluaCallable<Functor, Params...>>(this, std::forward<Functor>(f))};
}

template<typename ReturnType, typename... Params>
auto GluaBase::CreateGluaCallable(ReturnType (*callable)(Params...)) -> Callable
{
    return Callable{std::make_unique<GluaCallable<decltype(callable), Params...>>(this, callable)};
}

template<typename ClassType, typename ReturnType, typename... Params>
auto GluaBase::CreateGluaCallable(ReturnType (ClassType::*callable)(Params...) const) -> Callable
{
    auto method_call_lambda = [callable](const ClassType& object, Params... params) -> ReturnType {
        return (object.*callable)(std::forward<Params>(params)...);
    };

    return Callable{std::make_unique<GluaCallable<decltype(method_call_lambda), const ClassType&, Params...>>(
        this, std::move(method_call_lambda))};
}

template<typename ClassType, typename ReturnType, typename... Params>
auto GluaBase::CreateGluaCallable(ReturnType (ClassType::*callable)(Params...)) -> Callable
{
    auto method_call_lambda = [callable](ClassType& object, Params... params) -> ReturnType {
        return (object.*callable)(std::forward<Params>(params)...);
    };

    return Callable{std::make_unique<GluaCallable<decltype(method_call_lambda), ClassType&, Params...>>(
        this, std::move(method_call_lambda))};
}

template<typename Method, typename... Methods>
auto emplace_methods(GluaBase& lua, std::vector<std::unique_ptr<ICallable>>& v, Method m, Methods... methods) -> void
{
    v.emplace_back(lua.CreateGluaCallable(m).AcquireCallable());

    if constexpr (sizeof...(Methods) > 0)
    {
        emplace_methods(lua, v, methods...);
    }
}

template<typename ClassType, typename... Methods>
auto GluaBase::RegisterClassMultiString(std::string_view method_names, Methods... methods) -> void
{
    // split method names (as parameters) into vector, trim whitespace
    auto comma_separated = string_util::remove_all_whitespace(method_names);

    auto method_names_vector = string_util::split(comma_separated, std::string_view{","});

    std::vector<std::unique_ptr<ICallable>> methods_vector;
    methods_vector.reserve(sizeof...(Methods));
    emplace_methods(*this, methods_vector, methods...);

    RegisterClass<ClassType>(method_names_vector, std::move(methods_vector));
}

template<typename T>
auto default_construct_type() -> T
{
    return T{};
}

template<typename ClassType>
auto GluaBase::RegisterClass(std::vector<std::string_view> method_names, std::vector<std::unique_ptr<ICallable>> methods)
    -> void
{
    std::string                                                 class_name;
    auto                                                        name_pos = method_names.begin();
    std::unordered_map<std::string, std::unique_ptr<ICallable>> method_callables;

    for (auto& method : methods)
    {
        if (name_pos != method_names.end())
        {
            auto full_name = *name_pos;
            ++name_pos;

            auto last_scope = full_name.rfind("::");

            if (last_scope != std::string_view::npos)
            {
                auto method_class = full_name.substr(0, last_scope);
                auto method_name  = full_name.substr(last_scope + 2);

                // get rid of & from &ClassName::MethodName syntax
                if (!method_class.empty() && method_class.front() == '&')
                {
                    method_class.remove_prefix(1);
                }

                if (class_name.empty())
                {
                    class_name = std::string{method_class};
                }

                if (class_name == method_class)
                {
                    // add method to metatable
                    method_callables.emplace(std::string{method_name}, std::move(method));
                }
                else
                {
                    throw std::logic_error(
                        "RegisterClass had methods from various classes, only one class may be registered per call");
                }
            }
            else
            {
                throw std::logic_error("RegisterClass had method of invalid format, expecting ClassName::MethodName");
            }
        }
        else
        {
            throw std::logic_error("RegisterClass with fewer method names than methods!");
        }
    }

    // if we got here and have a class name then we had no expections, build Glua bindings
    if (!class_name.empty())
    {
        setUniqueClassName<ClassType>(class_name);
        registerClassImpl(class_name, std::move(method_callables));

        // now register the Create function if it exists
        if constexpr (HasCreate<ClassType>::value)
        {
            auto create_callable = CreateGluaCallable(&ClassType::Create);
            RegisterCallable("Create" + class_name, std::move(create_callable));
        }

        if constexpr (std::is_default_constructible<ClassType>::value)
        {
            auto construct_callable = CreateGluaCallable(&default_construct_type<ClassType>);
            RegisterCallable("Construct" + class_name, std::move(construct_callable));
        }
    }
}

template<typename ClassType>
auto GluaBase::RegisterMethod(const std::string& method_name, Callable method) -> void
{
    auto unique_name_opt = getUniqueClassName<ClassType>();

    if (unique_name_opt.has_value())
    {
        registerMethodImpl(unique_name_opt.value(), method_name, std::move(method));
    }
    else
    {
        throw exceptions::LuaException("Tried to register method to unregistered class [no metatable]");
    }
}

template<typename... Params>
auto GluaBase::CallScriptFunction(const std::string& function_name, Params&&... params) -> void
{
    // push all params onto the stack
    ((GluaBase::push<Params>(this, std::forward<Params>(params))), ...);

    callScriptFunctionImpl(function_name, sizeof...(params));
}

template<typename Type>
auto GluaBase::push(const std::vector<Type>& value) -> void
{
    auto size = value.size();
    pushArray(size);

    for (size_t i = 0; i < size; ++i)
    {
        GluaBase::push(this, transformObjectIndex(i));
        GluaBase::push(this, value[i]);
        arraySetFromStack();
    }
}
template<typename Type>
auto GluaBase::push(const std::unordered_map<std::string, Type>& value) -> void
{
    pushMap(value.size());

    for (const auto& item_pair : value)
    {
        GluaBase::push(this, item_pair.first);
        GluaBase::push(this, item_pair.second);
        mapSetFromStack();
    }
}
template<typename Type>
auto GluaBase::push(const std::optional<Type>& value) -> void
{
    if (value.has_value())
    {
        GluaBase::push(this, value.value());
    }
    else
    {
        push(std::nullopt);
    }
}

template<typename Type>
auto GluaBase::pushRegisteredType(Type&& custom_type) -> void
{
    if constexpr (std::is_enum<std::remove_reference_t<std::remove_const_t<Type>>>::value)
    {
        GluaBase::push(this, static_cast<uint64_t>(custom_type));
    }
    else
    {
        using UnderlyingType = typename RegistryType<Type>::underlying_type;

        auto unique_name_opt = getUniqueClassName<std::remove_reference_t<std::remove_const_t<UnderlyingType>>>();

        if (unique_name_opt.has_value())
        {
            // create storage, hand off to implementation
            if constexpr (std::is_pointer<Type>::value)
            {
                pushUserType(
                    unique_name_opt.value(),
                    std::make_unique<ManagedTypeRawPtr<UnderlyingType>>(std::forward<Type>(custom_type)));
            }
            else if constexpr (IsReferenceWrapper<Type>::value)
            {
                pushUserType(
                    unique_name_opt.value(),
                    std::make_unique<ManagedTypeRawPtr<UnderlyingType>>(static_cast<UnderlyingType*>(&custom_type.get())));
            }
            else if constexpr (IsSharedPointer<Type>::value)
            {
                pushUserType(
                    unique_name_opt.value(),
                    std::make_unique<ManagedTypeSharedPtr<UnderlyingType>>(std::forward<Type>(custom_type)));
            }
            else
            {
                // by value
                pushUserType(
                    unique_name_opt.value(),
                    std::make_unique<ManagedTypeStackAllocated<std::remove_reference_t<Type>>>(
                        std::forward<Type>(custom_type)));
            }
        }
        else
        {
            throw exceptions::GluaBaseException("Attempted to push unregistered type");
        }
    }
}

template<typename Type>
auto GluaBase::getArray(int stack_index) -> std::vector<Type>
{
    auto count = getArraySize(stack_index);

    std::vector<Type> result;
    result.reserve(count);

    for (size_t i = 0; i < count; ++i)
    {
        // pushes array value onto stack
        getArrayValue(transformObjectIndex(i), stack_index);
        result.push_back(GluaBase::get<Type>(this, -1)); // top of stack is now our value
        // pop the value back off thestack
        popOffStack(1);
    }

    return result;
}
template<typename Type>
auto GluaBase::getStringMap(int stack_index) -> std::unordered_map<std::string, Type>
{
    auto map_keys = getMapKeys(stack_index);

    std::unordered_map<std::string, Type> result;
    result.reserve(map_keys.size());

    for (auto& map_key : map_keys)
    {
        getMapValue(map_key, stack_index);
        result[std::move(map_key)] = GluaBase::get<Type>(this, -1); // after GetMapValue top of stack should be value
        // pop map value off stack
        popOffStack(1);
    }
}
template<typename Type>
auto GluaBase::getOptional(int stack_index) -> std::optional<Type>
{
    if (isNull(stack_index))
    {
        return std::nullopt;
    }

    return GluaBase::get<Type>(this, stack_index);
}

template<typename Type>
auto GluaBase::getRegisteredType(int index) -> Type
{
    if constexpr (std::is_enum<std::remove_reference_t<std::remove_const_t<Type>>>::value)
    {
        return static_cast<Type>(GluaBase::get<uint64_t>(this, index));
    }

    using UnderlyingType = typename RegistryType<Type>::underlying_type;

    auto unique_name_opt = getUniqueClassName<std::remove_reference_t<std::remove_const_t<UnderlyingType>>>();

    if (unique_name_opt.has_value())
    {
        auto storage_ptr = getUserType(unique_name_opt.value(), index);

        if (storage_ptr)
        {
            if constexpr (std::is_pointer<Type>::value)
            {
                switch (storage_ptr->GetStorageType())
                {
                    case ManagedTypeStorage::RAW_PTR:
                        return static_cast<ManagedTypeRawPtr<UnderlyingType>*>(storage_ptr)->GetValue();
                    case ManagedTypeStorage::SHARED_PTR:
                        return static_cast<ManagedTypeSharedPtr<UnderlyingType>*>(storage_ptr)->GetValue().get();
                    case ManagedTypeStorage::STACK_ALLOCATED:
                        return &static_cast<ManagedTypeStackAllocated<UnderlyingType>*>(storage_ptr)->GetValue();
                }
            }
            else if constexpr (IsReferenceWrapper<Type>::value)
            {
                switch (storage_ptr->GetStorageType())
                {
                    case ManagedTypeStorage::RAW_PTR:
                        return std::ref(*static_cast<ManagedTypeRawPtr<UnderlyingType>*>(storage_ptr)->GetValue());
                    case ManagedTypeStorage::SHARED_PTR:
                        return std::ref(*static_cast<ManagedTypeSharedPtr<UnderlyingType>*>(storage_ptr)->GetValue());
                    case ManagedTypeStorage::STACK_ALLOCATED:
                        return std::ref(static_cast<ManagedTypeStackAllocated<UnderlyingType>*>(storage_ptr)->GetValue());
                }
            }
            else if constexpr (IsSharedPointer<Type>::value)
            {
                if (storage_ptr->GetStorageType() == ManagedTypeStorage::SHARED_PTR)
                {
                    return static_cast<ManagedTypeSharedPtr<UnderlyingType>*>(storage_ptr)->GetValue();
                }

                throw exceptions::GluaBaseException(
                    ("Failed to get registered type as shared_ptr because it was not storaged as shared_ptr [" +
                     unique_name_opt.value().get())
                        .append("]"));
            }
            else
            {
                // by value
                switch (storage_ptr->GetStorageType())
                {
                    case ManagedTypeStorage::RAW_PTR:
                        return *static_cast<ManagedTypeRawPtr<UnderlyingType>*>(storage_ptr)->GetValue();
                    case ManagedTypeStorage::SHARED_PTR:
                        return *static_cast<ManagedTypeSharedPtr<UnderlyingType>*>(storage_ptr)->GetValue();
                    case ManagedTypeStorage::STACK_ALLOCATED:
                        return static_cast<ManagedTypeStackAllocated<UnderlyingType>*>(storage_ptr)->GetValue();
                }
            }
        }
        else
        {
            throw exceptions::GluaBaseException(
                ("Failed to get registered type [" + unique_name_opt.value().get()).append("]"));
        }
    }
    else
    {
        throw exceptions::GluaBaseException("Attempted to get registered type without valid class name");
    }
}

template<typename T>
auto GluaBase::getUniqueClassName() const -> std::optional<std::reference_wrapper<const std::string>>
{
    auto index = std::type_index{typeid(T)};

    auto pos = m_class_to_metatable_name.find(index);

    if (pos != m_class_to_metatable_name.end())
    {
        return std::cref(pos->second);
    }

    return std::nullopt;
}

template<typename T>
auto GluaBase::setUniqueClassName(std::string metatable_name) -> void
{
    auto index = std::type_index{typeid(T)};

    m_class_to_metatable_name[index] = std::move(metatable_name);
}

} // namespace kdk::glua
