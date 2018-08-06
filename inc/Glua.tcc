#include "Glua.h"

namespace kdk::glua
{
template<typename T>
auto Glua::get(const ICallable* callable, int32_t stack_index) -> T
{
    auto* lua_ptr = static_cast<Glua*>(callable->GetImplementationData());
    if (lua_ptr)
    {
        return get<T>(lua_ptr->GetLuaState(), stack_index);
    }
    else
    {
        throw exceptions::LuaException("Tried to get Glua value for a non-Glua callable");
    }
}

enum class ManagedTypeStorage
{
    RAW_PTR,
    SHARED_PTR,
    STACK_ALLOCATED
};

class IManagedTypeStorage
{
public:
    virtual auto GetStorageType() const -> ManagedTypeStorage = 0;

    virtual ~IManagedTypeStorage() = default;
};

template<typename T>
class ManagedTypeSharedPtr : public IManagedTypeStorage
{
public:
    ManagedTypeSharedPtr(std::shared_ptr<T> value) : m_value(value) {}

    auto GetStorageType() const -> ManagedTypeStorage override { return ManagedTypeStorage::SHARED_PTR; }

    auto GetValue() const -> std::shared_ptr<const T> { return m_value; }
    auto GetValue() -> std::shared_ptr<T> { return m_value; }

    ~ManagedTypeSharedPtr() override = default;

private:
    std::shared_ptr<T> m_value;
};

template<typename T>
class ManagedTypeRawPtr : public IManagedTypeStorage
{
public:
    ManagedTypeRawPtr(T* value) : m_value(value) {}

    auto GetStorageType() const -> ManagedTypeStorage override { return ManagedTypeStorage::RAW_PTR; }

    auto GetValue() const -> const T* { return m_value; }
    auto GetValue() -> T* { return m_value; }

    ~ManagedTypeRawPtr() override = default; // we don't own the object, so we don't delete it

private:
    T* m_value;
};

template<typename T>
class ManagedTypeStackAllocated : public IManagedTypeStorage
{
public:
    ManagedTypeStackAllocated(T value) : m_value(std::move(value)) {}

    auto GetStorageType() const -> ManagedTypeStorage override { return ManagedTypeStorage::STACK_ALLOCATED; }

    auto GetValue() const -> const T& { return m_value; }
    auto GetValue() -> T& { return m_value; }

    ~ManagedTypeStackAllocated() override = default;

private:
    T m_value;
};

template<typename T, typename = void>
struct ManagedTypeHandler
{
    static auto get(lua_State*, size_t) -> T { throw exceptions::LuaException("Tried to get value of unmanaged type"); }
    static auto push(lua_State*, T) -> void { throw exceptions::LuaException("Tried to push value of unmanaged type"); }
};

template<typename T>
struct ManagedTypeHandler<T*, std::enable_if_t<!IsManualType<T>::value>>
{
    static auto get(lua_State* state, int32_t stack_index) -> T*
    {
        auto metatable_name_opt = Glua::GetInstanceFromState(state).GetMetatableName<T>();

        if (metatable_name_opt.has_value())
        {
            IManagedTypeStorage* managed_type_ptr =
                static_cast<IManagedTypeStorage*>(luaL_checkudata(state, stack_index, metatable_name_opt.value().data()));

            if (managed_type_ptr != nullptr)
            {
                switch (managed_type_ptr->GetStorageType())
                {
                    case ManagedTypeStorage::RAW_PTR:
                    {
                        return static_cast<ManagedTypeRawPtr<T>*>(managed_type_ptr)->GetValue();
                    }
                    break;
                    case ManagedTypeStorage::SHARED_PTR:
                    {
                        return static_cast<ManagedTypeSharedPtr<T>*>(managed_type_ptr)->GetValue().get();
                    }
                    break;
                    case ManagedTypeStorage::STACK_ALLOCATED:
                    {
                        return &static_cast<ManagedTypeStackAllocated<T>*>(managed_type_ptr)->GetValue();
                    }
                    break;
                }
            }
            else
            {
                throw kdk::exceptions::LuaException("Tried to get type but invalid value at index!");
            }
        }
        else
        {
            throw kdk::exceptions::LuaException("Tried to get type with no registered metatable!");
        }
    }

    static auto push(lua_State* state, T* value) -> void
    {
        auto metatable_name_opt = Glua::GetInstanceFromState(state).GetMetatableName<T>();

        if (metatable_name_opt.has_value())
        {
            ManagedTypeRawPtr<T>* managed_type_ptr =
                static_cast<ManagedTypeRawPtr<T>*>(lua_newuserdata(state, sizeof(ManagedTypeRawPtr<T>)));

            managed_type_ptr = new (managed_type_ptr) ManagedTypeRawPtr<T>{value};

            auto metatable_name = metatable_name_opt.value();

            // set the metatable for this object
            luaL_getmetatable(state, metatable_name.data());

            if (lua_isnil(state, -1))
            {
                lua_pop(state, 1);
                managed_type_ptr->~ManagedTypeRawPtr<T>();

                throw std::runtime_error(
                    "Pushing class type which has not been registered [null metatable]! " + metatable_name);
            }
            lua_setmetatable(state, -2);
        }
        else
        {
            throw std::runtime_error("Pushing class type which has not been registered [no metatable name]!");
        }
    }
};

/*template<typename T>
struct ManagedTypeHandler<
    T&,
    std::enable_if_t<std::is_copy_constructible<T>::value && !std::is_enum<T>::value && !HasTypeHandler<T>::value>>
{
    static auto get(lua_State* state, int32_t stack_index) -> T&
    {
        auto metatable_name_opt = Glua::GetInstanceFromState(state).GetMetatableName<T>();

        if (metatable_name_opt.has_value())
        {
            IManagedTypeStorage* managed_type_ptr = static_cast<IManagedTypeStorage*>(
                luaL_checkudata(state, static_cast<int>(stack_index), metatable_name_opt.value().data()));

            if (managed_type_ptr != nullptr)
            {
                switch (managed_type_ptr->GetStorageType())
                {
                    case ManagedTypeStorage::RAW_PTR:
                    {
                        return *static_cast<ManagedTypeRawPtr<T>*>(managed_type_ptr)->GetValue();
                    }
                    break;
                    case ManagedTypeStorage::SHARED_PTR:
                    {
                        return *static_cast<ManagedTypeSharedPtr<T>*>(managed_type_ptr)->GetValue();
                    }
                    break;
                    case ManagedTypeStorage::STACK_ALLOCATED:
                    {
                        return static_cast<ManagedTypeStackAllocated<T>*>(managed_type_ptr)->GetValue();
                    }
                    break;
                }
            }
            else
            {
                throw kdk::exceptions::LuaException("Tried to get type but invalid value at index!");
            }
        }
        else
        {
            throw kdk::exceptions::LuaException("Tried to get type with no registered metatable!");
        }
    }

    static auto push(lua_State* state, T& value) -> void
    {
        auto metatable_name_opt = Glua::GetInstanceFromState(state).GetMetatableName<T>();

        if (metatable_name_opt.has_value())
        {
            ManagedTypeRawPtr<T>* managed_type_ptr =
                static_cast<ManagedTypeRawPtr<T>*>(lua_newuserdata(state, sizeof(ManagedTypeRawPtr<T>)));

            managed_type_ptr = new (managed_type_ptr) ManagedTypeRawPtr<T>{&value};

            auto metatable_name = metatable_name_opt.value();

            // set the metatable for this object
            luaL_getmetatable(state, metatable_name.data());

            if (lua_isnil(state, -1))
            {
                lua_pop(state, 1);
                managed_type_ptr->~ManagedTypeRawPtr<T>();

                throw std::runtime_error(
                    "Pushing class type which has not been registered [null metatable]! " + metatable_name);
            }
            lua_setmetatable(state, -2);
        }
        else
        {
            throw std::runtime_error("Pushing class type which has not been registered [no metatable name]!");
        }
    }
};*/

template<typename T>
struct ManagedTypeHandler<std::vector<T>>
{
    static auto get(lua_State* state, int32_t stack_index) -> std::vector<T>
    {
        auto& lua_object = Glua::GetInstanceFromState(state);

        if (lua_istable(state, stack_index))
        {
            std::vector<T> result;

            size_t length = lua_objlen(state, stack_index);

            result.reserve(length);

            for (size_t i = 0; i < length; ++i)
            {
                // get our value onto the stack...
                lua_pushinteger(state, static_cast<lua_Integer>(i + 1)); // lua arrays 1 based

                if (stack_index >= 0)
                {
                    lua_gettable(state, stack_index);
                }
                else
                {
                    // we have a negative (from the end) index, which we've changed by pushing the index
                    lua_gettable(state, stack_index - 1);
                }

                result.emplace_back(lua_object.Pop<T>());
            }

            return result;
        }
        else
        {
            throw exceptions::LuaException("Attempted to pop array off stack but top of stack wasn't a table");
        }
    }

    static auto push(lua_State* state, std::vector<T> value) -> void
    {
        auto& lua_object = Glua::GetInstanceFromState(state);

        auto size = value.size();
        lua_createtable(state, size, 0);
        for (size_t i = 0; i < size; ++i)
        {
            lua_pushinteger(state, static_cast<lua_Integer>(i + 1)); // lua arrays 1 based
            lua_object.Push(std::move(value[i]));

            lua_settable(state, -3);
        }
    }
};

template<typename T>
struct ManagedTypeHandler<std::optional<T>>
{
    static auto get(lua_State* state, int32_t stack_index) -> std::optional<T>
    {
        auto& lua_object = Glua::GetInstanceFromState(state);

        if (lua_isnil(state, stack_index))
        {
            return {};
        }
        else
        {
            return lua_object.Pop<T>();
        }
    }

    static auto push(lua_State* state, std::optional<T> value) -> void
    {
        auto& lua_object = Glua::GetInstanceFromState(state);

        if (value.has_value())
        {
            lua_object.Push(std::move(value.value()));
        }
        else
        {
            lua_pushnil(state);
        }
    }
}; // namespace kdk::glua

template<typename T>
struct ManagedTypeHandler<std::shared_ptr<T>, std::enable_if_t<!IsManualType<T>::value>>
{
    static auto get(lua_State* state, int32_t stack_index) -> std::shared_ptr<T>
    {
        auto metatable_name_opt = Glua::GetInstanceFromState(state).GetMetatableName<T>();

        if (metatable_name_opt.has_value())
        {
            IManagedTypeStorage* managed_type_ptr =
                static_cast<IManagedTypeStorage*>(luaL_checkudata(state, stack_index, metatable_name_opt.value().data()));

            if (managed_type_ptr != nullptr)
            {
                if (managed_type_ptr->GetStorageType() == ManagedTypeStorage::SHARED_PTR)
                {
                    return static_cast<ManagedTypeSharedPtr<T>*>(managed_type_ptr)->GetValue();
                }
                else
                {
                    throw kdk::exceptions::LuaException("Tried to get shared pointer of type but invalid value at index!");
                }
            }
            else
            {
                throw kdk::exceptions::LuaException("Tried to get type but invalid value at index!");
            }
        }
        else
        {
            throw kdk::exceptions::LuaException("Tried to get type with no registered metatable!");
        }
    }

    static auto push(lua_State* state, std::shared_ptr<T> value) -> void
    {
        auto metatable_name_opt = Glua::GetInstanceFromState(state).GetMetatableName<T>();

        if (metatable_name_opt.has_value())
        {
            ManagedTypeSharedPtr<T>* managed_type_ptr =
                static_cast<ManagedTypeSharedPtr<T>*>(lua_newuserdata(state, sizeof(ManagedTypeSharedPtr<T>)));

            managed_type_ptr = new (managed_type_ptr) ManagedTypeSharedPtr<T>{value};

            auto metatable_name = metatable_name_opt.value();

            // set the metatable for this object
            luaL_getmetatable(state, metatable_name.data());

            if (lua_isnil(state, -1))
            {
                lua_pop(state, 1);
                managed_type_ptr->~ManagedTypeSharedPtr<T>();

                throw std::runtime_error(
                    "Pushing class type which has not been registered [null metatable]! " + metatable_name);
            }
            lua_setmetatable(state, -2);
        }
        else
        {
            throw std::runtime_error("Pushing class type which has not been registered [no metatable name]!");
        }
    }
};

template<typename T>
struct ManagedTypeHandler<std::reference_wrapper<T>, std::enable_if_t<!IsManualType<T>::value && IsBasicType<T>::value>>
{
    static auto get(lua_State* state, int32_t stack_index) -> T { return Glua::get<T>(state, stack_index); }

    static auto push(lua_State* state, std::reference_wrapper<T> value) -> void
    {
        Glua::GetInstanceFromState(state).Push(value.get());
    }
};

template<typename T>
struct ManagedTypeHandler<std::reference_wrapper<T>, std::enable_if_t<!IsManualType<T>::value && !IsBasicType<T>::value>>
{
    static auto get(lua_State* state, int32_t stack_index) -> std::reference_wrapper<T>
    {
        auto metatable_name_opt = Glua::GetInstanceFromState(state).GetMetatableName<T>();

        if (metatable_name_opt.has_value())
        {
            IManagedTypeStorage* managed_type_ptr =
                static_cast<IManagedTypeStorage*>(luaL_checkudata(state, stack_index, metatable_name_opt.value().data()));

            if (managed_type_ptr != nullptr)
            {
                switch (managed_type_ptr->GetStorageType())
                {
                    case ManagedTypeStorage::RAW_PTR:
                    {
                        return std::ref(*static_cast<ManagedTypeRawPtr<T>*>(managed_type_ptr)->GetValue());
                    }
                    break;
                    case ManagedTypeStorage::SHARED_PTR:
                    {
                        return std::ref(*static_cast<ManagedTypeSharedPtr<T>*>(managed_type_ptr)->GetValue());
                    }
                    break;
                    case ManagedTypeStorage::STACK_ALLOCATED:
                    {
                        return std::ref(static_cast<ManagedTypeStackAllocated<T>*>(managed_type_ptr)->GetValue());
                    }
                    break;
                }
            }
            else
            {
                throw kdk::exceptions::LuaException("Tried to get type but invalid value at index!");
            }
        }
        else
        {
            throw kdk::exceptions::LuaException("Tried to get type with no registered metatable!");
        }
    }

    static auto push(lua_State* state, std::reference_wrapper<T> value) -> void
    {
        auto metatable_name_opt = Glua::GetInstanceFromState(state).GetMetatableName<T>();

        if (metatable_name_opt.has_value())
        {
            ManagedTypeRawPtr<T>* managed_type_ptr =
                static_cast<ManagedTypeRawPtr<T>*>(lua_newuserdata(state, sizeof(ManagedTypeRawPtr<T>)));

            managed_type_ptr = new (managed_type_ptr) ManagedTypeRawPtr<T>{&value.get()};

            auto metatable_name = metatable_name_opt.value();

            // set the metatable for this object
            luaL_getmetatable(state, metatable_name.data());

            if (lua_isnil(state, -1))
            {
                lua_pop(state, 1);
                managed_type_ptr->~ManagedTypeRawPtr<T>();

                throw std::runtime_error(
                    "Pushing class type which has not been registered [null metatable]! " + metatable_name);
            }
            lua_setmetatable(state, -2);
        }
        else
        {
            throw std::runtime_error("Pushing class type which has not been registered [no metatable name]!");
        }
    }
};

template<typename T>
struct ManagedTypeHandler<std::reference_wrapper<T>, std::enable_if_t<IsManualType<T>::value>>
{
    static auto get(lua_State* state, int32_t stack_index) -> T { return Glua::get<T>(state, stack_index); }

    static auto push(lua_State* state, std::reference_wrapper<T> value) -> void
    {
        Glua::GetInstanceFromState(state).Push(value.get());
    }
};

template<typename T>
struct ManagedTypeHandler<T, std::enable_if_t<std::is_enum<T>::value && !HasTypeHandler<T>::value>>
{
    static auto get(lua_State* state, int32_t stack_index) -> T
    {
        // lua treats as number
        return static_cast<T>(lua_tonumber(state, stack_index));
    }
    static auto push(lua_State* state, T value) -> void
    {
        // lua treats as number
        lua_pushnumber(state, static_cast<lua_Number>(value));
    }
};

template<typename T>
struct ManagedTypeHandler<T, std::enable_if_t<HasCustomHandler<T>::value>>
{
    static auto get(lua_State* state, int32_t stack_index) -> T
    {
        return CustomTypeHandler<std::remove_const_t<T>>::get(state, stack_index);
    }
    static auto push(lua_State* state, T value) -> void
    {
        CustomTypeHandler<std::remove_const_t<T>>::push(state, std::move(value));
    }
};

template<typename T>
struct ManagedTypeHandler<
    T,
    std::enable_if_t<
        std::is_copy_constructible<T>::value && !std::is_enum<T>::value && !HasTypeHandler<T>::value && !IsManualType<T>::value>>
{
    static auto get(lua_State* state, int32_t stack_index) -> T
    {
        auto metatable_name_opt = Glua::GetInstanceFromState(state).GetMetatableName<std::remove_reference_t<T>>();

        if (metatable_name_opt.has_value())
        {
            IManagedTypeStorage* managed_type_ptr =
                static_cast<IManagedTypeStorage*>(luaL_checkudata(state, stack_index, metatable_name_opt.value().data()));

            if (managed_type_ptr != nullptr)
            {
                switch (managed_type_ptr->GetStorageType())
                {
                    case ManagedTypeStorage::RAW_PTR:
                    {
                        return *static_cast<ManagedTypeRawPtr<std::remove_reference_t<T>>*>(managed_type_ptr)->GetValue();
                    }
                    break;
                    case ManagedTypeStorage::SHARED_PTR:
                    {
                        return *static_cast<ManagedTypeSharedPtr<std::remove_reference_t<T>>*>(managed_type_ptr)->GetValue();
                    }
                    break;
                    case ManagedTypeStorage::STACK_ALLOCATED:
                    {
                        return static_cast<ManagedTypeStackAllocated<std::remove_reference_t<T>>*>(managed_type_ptr)
                            ->GetValue();
                    }
                    break;
                }
            }
            else
            {
                throw kdk::exceptions::LuaException("Tried to get type but invalid value at index!");
            }
        }
        else
        {
            throw kdk::exceptions::LuaException("Tried to get type with no registered metatable!");
        }
    } // namespace kdk::glua
    static auto push(lua_State* state, T value) -> void
    {
        auto metatable_name_opt = Glua::GetInstanceFromState(state).GetMetatableName<std::remove_reference_t<T>>();

        if (metatable_name_opt.has_value())
        {
            ManagedTypeStackAllocated<T>* managed_type_ptr =
                static_cast<ManagedTypeStackAllocated<T>*>(lua_newuserdata(state, sizeof(ManagedTypeStackAllocated<T>)));

            managed_type_ptr =
                new (managed_type_ptr) ManagedTypeStackAllocated<std::remove_reference_t<T>>{std::move(value)};

            auto metatable_name = metatable_name_opt.value();

            // set the metatable for this object
            luaL_getmetatable(state, metatable_name.data());

            if (lua_isnil(state, -1))
            {
                lua_pop(state, 1);
                managed_type_ptr->~ManagedTypeStackAllocated<std::remove_reference_t<T>>();

                throw std::runtime_error(
                    "Pushing class type which has not been registered [null metatable]! " + metatable_name);
            }
            lua_setmetatable(state, -2);
        }
        else
        {
            throw std::runtime_error("Pushing class type which has not been registered [no metatable name]!");
        }
    }
};

template<typename T>
auto Glua::get(lua_State* state, int32_t stack_index) -> T
{
    return ManagedTypeHandler<T>::get(state, stack_index);
}

template<typename T>
auto Glua::push(const ICallable* callable, T value) -> void
{
    auto* lua_ptr = static_cast<Glua*>(callable->GetImplementationData());
    if (lua_ptr)
    {
        lua_ptr->Push(std::move(value));
    }
    else
    {
        throw exceptions::LuaException("Tried to push Glua value for a non-Glua callable");
    }
}

template<>
auto Glua::get(lua_State* lua, int32_t stack_index) -> bool;

template<>
auto Glua::get(lua_State* lua, int32_t stack_index) -> int8_t;

template<>
auto Glua::get(lua_State* lua, int32_t stack_index) -> int16_t;

template<>
auto Glua::get(lua_State* lua, int32_t stack_index) -> int32_t;

template<>
auto Glua::get(lua_State* lua, int32_t stack_index) -> int64_t;

template<>
auto Glua::get(lua_State* lua, int32_t stack_index) -> uint8_t;

template<>
auto Glua::get(lua_State* lua, int32_t stack_index) -> uint16_t;

template<>
auto Glua::get(lua_State* lua, int32_t stack_index) -> uint32_t;

template<>
auto Glua::get(lua_State* lua, int32_t stack_index) -> uint64_t;

template<>
auto Glua::get(lua_State* lua, int32_t stack_index) -> float;

template<>
auto Glua::get(lua_State* lua, int32_t stack_index) -> double;

template<>
auto Glua::get(lua_State* lua, int32_t stack_index) -> std::string;

template<>
auto Glua::get(lua_State* lua, int32_t stack_index) -> std::string_view;

template<typename T>
auto Glua::Push(T value) -> void
{
    return ManagedTypeHandler<T>::push(m_lua, std::move(value));
}

template<typename T>
auto Glua::Pop() -> T
{
    auto val = get<T>(m_lua, -1);
    lua_pop(m_lua, 1);

    return val;
}

template<typename T>
auto Glua::SetGlobal(const std::string& name, T value) -> void
{
    Push(std::move(value));
    set_global_from_stack(name.data());
}

template<typename T>
auto Glua::GetGlobal(const std::string& name) -> T
{
    get_global_onto_stack(name.data());
    return Pop<T>();
}

template<typename Functor, typename... Params>
auto Glua::CreateLuaCallable(Functor&& f) -> Callable
{
    return Callable{std::make_unique<GluaCallable<Functor, Params...>>(shared_from_this(), std::forward<Functor>(f))};
}

template<typename ReturnType, typename... Params>
auto Glua::CreateLuaCallable(ReturnType (*callable)(Params...)) -> Callable
{
    return Callable{std::make_unique<GluaCallable<decltype(callable), Params...>>(shared_from_this(), callable)};
}

template<typename ClassType, typename ReturnType, typename... Params>
auto Glua::CreateLuaCallable(ReturnType (ClassType::*callable)(Params...) const) -> Callable
{
    auto method_call_lambda = [callable](const ClassType& object, Params... params) -> ReturnType {
        return (object.*callable)(std::forward<Params>(params)...);
    };

    return Callable{std::make_unique<GluaCallable<decltype(method_call_lambda), const ClassType&, Params...>>(
        shared_from_this(), std::move(method_call_lambda))};
}

template<typename ClassType, typename ReturnType, typename... Params>
auto Glua::CreateLuaCallable(ReturnType (ClassType::*callable)(Params...)) -> Callable
{
    auto method_call_lambda = [callable](ClassType& object, Params... params) -> ReturnType {
        return (object.*callable)(std::forward<Params>(params)...);
    };

    return Callable{std::make_unique<GluaCallable<decltype(method_call_lambda), ClassType&, Params...>>(
        shared_from_this(), std::move(method_call_lambda))};
}

template<typename... Params>
auto Glua::CallScriptFunction(const std::string& function_name, Params&&... params) -> void
{
    get_global_onto_stack(function_name.data());

    if (!lua_isfunction(m_lua, -1))
    {
        throw exceptions::LuaException(
            "Attempted to call lua script function " + function_name + " which was not a function");
    }

    lua_getglobal(m_lua, "__libglua__env__");
    lua_setfenv(m_lua, -2);

    // push all params onto the lua stack
    ((Push(std::forward<Params>(params))), ...);

    if (lua_pcall(m_lua, sizeof...(Params), LUA_MULTRET, 0))
    {
        throw exceptions::LuaException(
            "Failed to call lua script function [" + function_name + "]: " + lua_tostring(m_lua, -1));
    }
}

template<typename Method, typename... Methods>
auto emplace_methods(Glua& lua, std::vector<std::unique_ptr<ICallable>>& v, Method m, Methods... methods) -> void
{
    v.emplace_back(lua.CreateLuaCallable(m).AcquireCallable());

    if constexpr (sizeof...(Methods) > 0)
    {
        emplace_methods(lua, v, methods...);
    }
}

template<typename ClassType, typename... Methods>
auto Glua::RegisterClassMultiString(std::string_view method_names, Methods... methods) -> void
{
    // split method names (as parameters) into vector, trim whitespace
    auto comma_separated = remove_all_whitespace(method_names);

    auto method_names_vector = split(comma_separated, std::string_view{","});

    std::vector<std::unique_ptr<ICallable>> methods_vector;
    methods_vector.reserve(sizeof...(Methods));
    emplace_methods(*this, methods_vector, methods...);

    RegisterClass<ClassType>(method_names_vector, std::move(methods_vector));
}

template<typename T, typename = void>
struct HasCreate : std::false_type
{
};

template<typename T>
struct HasCreate<T, std::void_t<decltype(T::Create)>> : std::true_type
{
};

template<typename T>
auto default_construct_type() -> T
{
    return T{};
}

template<typename ClassType, typename... Methods>
auto Glua::RegisterClass(std::vector<std::string_view> method_names, std::vector<std::unique_ptr<ICallable>> methods)
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
                if (method_class.size() > 0 && method_class.front() == '&')
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
        SetMetatableName<ClassType>(class_name);

        luaL_newmetatable(m_lua, class_name.data());

        lua_pushstring(m_lua, "__gc");
        lua_pushcfunction(m_lua, &destruct_managed_type);
        lua_settable(m_lua, -3);

        auto  pos_pair     = m_method_registry.emplace(class_name, std::move(method_callables));
        auto& our_registry = pos_pair.first->second;

        lua_pushstring(m_lua, "__index");
        lua_pushvalue(m_lua, -2);
        lua_settable(m_lua, -3);

        for (auto& method_pair : our_registry)
        {
            lua_pushstring(m_lua, method_pair.first.data());
            auto* callable_ptr = method_pair.second.get();
            lua_pushlightuserdata(m_lua, callable_ptr);
            lua_pushcclosure(m_lua, call_callable_from_lua, 1);

            lua_settable(m_lua, -3);
        }

        // pop the new metatable off the stack
        lua_pop(m_lua, 1);

        // now register the Create function if it exists
        if constexpr (HasCreate<ClassType>::value)
        {
            auto create_callable = CreateLuaCallable(&ClassType::Create);
            RegisterCallable("Create" + class_name, std::move(create_callable));
        }

        if constexpr (std::is_default_constructible<ClassType>::value)
        {
            auto construct_callable = CreateLuaCallable(&default_construct_type<ClassType>);
            RegisterCallable("Construct" + class_name, std::move(construct_callable));
        }
    }
}

template<typename ClassType>
auto Glua::RegisterMethod(const std::string& method_name, Callable method) -> void
{
    auto metatable_opt = GetMetatableName<ClassType>();

    if (metatable_opt.has_value())
    {
        luaL_getmetatable(m_lua, metatable_opt.value().data());

        auto pos_pair =
            m_method_registry[metatable_opt.value()].emplace(method_name, std::move(method).AcquireCallable());

        if (pos_pair.second)
        {
            lua_pushlstring(m_lua, method_name.data(), method_name.size());
            auto* callable_ptr = pos_pair.first->second.get();
            lua_pushlightuserdata(m_lua, callable_ptr);
            lua_pushcclosure(m_lua, call_callable_from_lua, 1);
        }
        else
        {
            throw exceptions::LuaException("Tried to register method with already registered name [" + method_name + "]");
        }

        lua_settable(m_lua, -3);
    }
    else
    {
        throw exceptions::LuaException("Tried to register method to unregistered class [no metatable]");
    }
}

template<typename T>
auto Glua::GetMetatableName() -> std::optional<std::string>
{
    auto index = std::type_index{typeid(T)};

    auto pos = m_class_to_metatable_name.find(index);

    if (pos != m_class_to_metatable_name.end())
    {
        return pos->second;
    }
    else
    {
        return std::nullopt;
    }
}

template<typename T>
auto Glua::SetMetatableName(std::string metatable_name) -> void
{
    auto index = std::type_index{typeid(T)};

    m_class_to_metatable_name[index] = std::move(metatable_name);
}

} // namespace kdk::glua
