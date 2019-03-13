#pragma once

#include "glua/GluaBaseHelperTemplates.h"

namespace kdk::glua {
template <typename T>
auto GluaResolver<T>::as(GluaBase* glua, int stack_index) -> T
{
    using RawT = std::decay_t<T>;

    if constexpr (std::is_enum<RawT>::value) {
        return static_cast<T>(GluaResolver<uint64_t>::as(glua, stack_index));
    } else {
        auto unique_name_opt = glua->getUniqueClassName<RawT>();

        if (unique_name_opt.has_value()) {
            auto storage_ptr = static_cast<ManagedTypeStorage<RawT>*>(
                glua->getUserType(unique_name_opt.value(), stack_index));

            if (storage_ptr) {
                return storage_ptr->GetStoredValue();
            }

            // all above cases return, so type must be unregistered
            throw exceptions::GluaBaseException(
                ("Failed to get registered type [" + unique_name_opt.value().get())
                    .append("]"));
        }

        // if we got here we have an invalid class name
        throw exceptions::GluaBaseException(
            "Attempted to get registered type without valid class name");
    }
}

template <typename T>
auto GluaResolver<T>::is(GluaBase* glua, int stack_index) -> bool
{
    using RawT = std::decay_t<T>;

    if constexpr (std::is_enum<RawT>::value) {
        return GluaResolver<uint64_t>::is(glua, stack_index);
    }

    auto unique_name_opt = glua->getUniqueClassName<RawT>();

    if (unique_name_opt.has_value()) {
        return glua->isUserType(unique_name_opt.value(), stack_index);
    }

    throw exceptions::GluaBaseException(
        "Attempted to check against a registered type without valid class name");
}

template <typename T>
auto GluaResolver<T>::push(GluaBase* glua, T value) -> void
{
    using RawT = std::decay_t<T>;

    if constexpr (std::is_enum<RawT>::value) {
        GluaResolver<uint64_t>::push(glua, static_cast<uint64_t>(value));
    } else {
        auto unique_name_opt = glua->getUniqueClassName<RawT>();

        if (unique_name_opt.has_value()) {
            // by value
            glua->pushUserType(
                unique_name_opt.value(),
                std::make_unique<ManagedTypeStackAllocated<RawT>>(std::move(value)));
        } else {
            throw exceptions::GluaBaseException(
                "Attempted to push unregistered type");
        }
    }
}

template <typename T>
auto GluaResolver<T*>::as(GluaBase* glua, int stack_index) -> T*
{
    using RawT = std::decay_t<T>;

    if constexpr (std::is_enum<RawT>::value) {
        return static_cast<T>(GluaResolver<uint64_t>::as(glua, stack_index));
    }

    auto unique_name_opt = glua->getUniqueClassName<RawT>();

    if (unique_name_opt.has_value()) {
        auto storage_ptr = static_cast<ManagedTypeStorage<RawT>*>(
            glua->getUserType(unique_name_opt.value(), stack_index));

        if (storage_ptr) {
            return &storage_ptr->GetStoredValue();
        }

        // all above cases return, so type must be unregistered
        throw exceptions::GluaBaseException(
            ("Failed to get registered type [" + unique_name_opt.value().get())
                .append("]"));
    }

    // if we got here we have an invalid class name
    throw exceptions::GluaBaseException(
        "Attempted to get registered type without valid class name");
}
template <typename T>
auto GluaResolver<T*>::is(GluaBase* glua, int stack_index) -> bool
{
    using RawT = std::decay_t<T>;

    if constexpr (std::is_enum<RawT>::value) {
        return GluaResolver<uint64_t>::is(glua, stack_index);
    }

    auto unique_name_opt = glua->getUniqueClassName<RawT>();

    if (unique_name_opt.has_value()) {
        return glua->isUserType(unique_name_opt.value(), stack_index);
    }

    throw exceptions::GluaBaseException(
        "Attempted to check against a registered type without valid class name");
}
template <typename T>
auto GluaResolver<T*>::push(GluaBase* glua, T* value) -> void
{
    using RawT = std::decay_t<T>;

    if constexpr (std::is_enum<RawT>::value) {
        GluaResolver<uint64_t>::push(glua, static_cast<uint64_t>(value));
    } else {
        auto unique_name_opt = glua->getUniqueClassName<RawT>();

        if (unique_name_opt.has_value()) {
            // create storage, hand off to implementation
            glua->pushUserType(unique_name_opt.value(),
                std::make_unique<ManagedTypeRawPtr<RawT>>(value));
        } else {
            throw exceptions::GluaBaseException(
                "Attempted to push unregistered type");
        }
    }
}

template <typename T>
auto GluaResolver<std::reference_wrapper<T>>::as(GluaBase* glua,
    int stack_index)
    -> std::reference_wrapper<T>
{
    using RawT = std::decay_t<T>;

    if constexpr (std::is_enum<RawT>::value) {
        return static_cast<T>(GluaResolver<uint64_t>::as(glua, stack_index));
    }

    auto unique_name_opt = glua->getUniqueClassName<RawT>();

    if (unique_name_opt.has_value()) {
        auto storage_ptr = static_cast<ManagedTypeStorage<RawT>*>(
            glua->getUserType(unique_name_opt.value(), stack_index));

        if (storage_ptr) {
            return std::ref(storage_ptr->GetStoredValue());
        }

        // all above cases return, so type must be unregistered
        throw exceptions::GluaBaseException(
            ("Failed to get registered type [" + unique_name_opt.value().get())
                .append("]"));
    }

    // if we got here we have an invalid class name
    throw exceptions::GluaBaseException(
        "Attempted to get registered type without valid class name");
}
template <typename T>
auto GluaResolver<std::reference_wrapper<T>>::is(GluaBase* glua,
    int stack_index) -> bool
{
    using RawT = std::decay_t<T>;

    if constexpr (std::is_enum<RawT>::value) {
        return GluaResolver<uint64_t>::is(glua, stack_index);
    }

    auto unique_name_opt = glua->getUniqueClassName<RawT>();

    if (unique_name_opt.has_value()) {
        return glua->isUserType(unique_name_opt.value(), stack_index);
    }

    throw exceptions::GluaBaseException(
        "Attempted to check against a registered type without valid class name");
}
template <typename T>
auto GluaResolver<std::reference_wrapper<T>>::push(
    GluaBase* glua, std::reference_wrapper<T> value) -> void
{
    using RawT = std::decay_t<T>;

    if constexpr (std::is_const<T>::value) {
        // scripting languages often have no concept of const
        // push by value
        GluaResolver<RawT>::push(glua, value.get());
    } else {
        if constexpr (std::is_enum<RawT>::value) {
            GluaResolver<uint64_t>::push(glua, static_cast<uint64_t>(value));
        } else {
            auto unique_name_opt = glua->getUniqueClassName<RawT>();

            if (unique_name_opt.has_value()) {
                // create storage, hand off to implementation
                glua->pushUserType(unique_name_opt.value(),
                    std::make_unique<ManagedTypeRawPtr<RawT>>(
                        static_cast<RawT*>(&value.get())));

            } else {
                throw exceptions::GluaBaseException(
                    "Attempted to push unregistered type");
            }
        }
    }
}

template <typename T>
auto GluaResolver<std::shared_ptr<T>>::as(GluaBase* glua, int stack_index)
    -> std::shared_ptr<T>
{
    using RawT = std::decay_t<T>;

    if constexpr (std::is_enum<RawT>::value) {
        return static_cast<T>(GluaResolver<uint64_t>::as(glua, stack_index));
    }

    auto unique_name_opt = glua->getUniqueClassName<RawT>();

    if (unique_name_opt.has_value()) {
        auto storage_ptr = static_cast<ManagedTypeStorage<RawT>*>(
            glua->getUserType(unique_name_opt.value(), stack_index));

        if (storage_ptr) {
            // shared_ptr is a special case where it must specifically have
            // shared_ptr storage
            if (storage_ptr->GetStorageType() == ManagedTypeStorageType::SHARED_PTR) {
                return static_cast<ManagedTypeSharedPtr<RawT>*>(storage_ptr)
                    ->GetValue();
            }

            throw exceptions::GluaBaseException(
                ("Failed to get registered type as shared_ptr because it was not "
                 "storaged as shared_ptr ["
                    + unique_name_opt.value().get())
                    .append("]"));
        }

        // all above cases return, so type must be unregistered
        throw exceptions::GluaBaseException(
            ("Failed to get registered type [" + unique_name_opt.value().get())
                .append("]"));
    }

    // if we got here we have an invalid class name
    throw exceptions::GluaBaseException(
        "Attempted to get registered type without valid class name");
}
template <typename T>
auto GluaResolver<std::shared_ptr<T>>::is(GluaBase* glua, int stack_index)
    -> bool
{
    using RawT = std::decay_t<T>;

    if constexpr (std::is_enum<RawT>::value) {
        return GluaResolver<uint64_t>::is(glua, stack_index);
    }

    auto unique_name_opt = glua->getUniqueClassName<RawT>();

    if (unique_name_opt.has_value()) {
        return glua->isUserType(unique_name_opt.value(), stack_index);
    }

    throw exceptions::GluaBaseException(
        "Attempted to check against a registered type without valid class name");
}
template <typename T>
auto GluaResolver<std::shared_ptr<T>>::push(GluaBase* glua,
    std::shared_ptr<T> value) -> void
{
    using RawT = std::decay_t<T>;

    if constexpr (std::is_enum<RawT>::value) {
        GluaResolver<uint64_t>::push(glua, static_cast<uint64_t>(value));
    } else {
        auto unique_name_opt = glua->getUniqueClassName<RawT>();

        if (unique_name_opt.has_value()) {
            // create storage, hand off to implementation
            glua->pushUserType(unique_name_opt.value(),
                std::make_unique<ManagedTypeSharedPtr<RawT>>(value));
        } else {
            throw exceptions::GluaBaseException(
                "Attempted to push unregistered type");
        }
    }
}

template <typename T>
auto GluaResolver<std::vector<T>>::as(GluaBase* glua, int stack_index)
    -> std::vector<T>
{
    auto count = glua->getArraySize(stack_index);

    std::vector<T> result;
    result.reserve(count);

    for (size_t i = 0; i < count; ++i) {
        // pushes array value onto stack
        glua->getArrayValue(glua->transformObjectIndex(i), stack_index);
        result.push_back(
            GluaResolver<T>::as(glua, -1)); // top of stack is now our value
        // pop the value back off thestack
        glua->popOffStack(1);
    }

    return result;
}
template <typename T>
auto GluaResolver<std::vector<T>>::is(GluaBase* glua, int stack_index) -> bool
{
    return glua->isArray(stack_index);
}
template <typename T>
auto GluaResolver<std::vector<T>>::push(GluaBase* glua,
    const std::vector<T>& value) -> void
{
    auto size = value.size();
    glua->pushArray(size);

    for (size_t i = 0; i < size; ++i) {
        GluaResolver<size_t>::push(glua, glua->transformObjectIndex(i));
        GluaResolver<T>::push(glua, value[i]);
        glua->arraySetFromStack();
    }
}

template <typename T>
auto GluaResolver<std::unordered_map<std::string, T>>::as(GluaBase* glua,
    int stack_index)
    -> std::unordered_map<std::string, T>
{
    auto map_keys = glua->getMapKeys(stack_index);

    std::unordered_map<std::string, T> result;
    result.reserve(map_keys.size());

    for (auto& map_key : map_keys) {
        glua->getMapValue(map_key, stack_index);
        result[std::move(map_key)] = GluaResolver<T>::as(
            glua, -1); // after GetMapValue top of stack should be value
        // pop map value off stack
        glua->popOffStack(1);
    }

    return result;
}
template <typename T>
auto GluaResolver<std::unordered_map<std::string, T>>::is(GluaBase* glua,
    int stack_index)
    -> bool
{
    return glua->isMap(stack_index);
}
template <typename T>
auto GluaResolver<std::unordered_map<std::string, T>>::push(
    GluaBase* glua, const std::unordered_map<std::string, T>& value) -> void
{
    glua->pushStartMap(value.size());

    for (const auto& item_pair : value) {
        GluaResolver<std::string>::push(glua, item_pair.first);
        GluaResolver<T>::push(glua, item_pair.second);
        glua->mapSetFromStack();
    }
}

template <typename T>
auto GluaResolver<std::optional<T>>::as(GluaBase* glua, int stack_index)
    -> std::optional<T>
{
    if (glua->isNull(stack_index)) {
        return std::nullopt;
    }

    return GluaResolver<T>::as(glua, stack_index);
}
template <typename T>
auto GluaResolver<std::optional<T>>::is(GluaBase* glua, int stack_index)
    -> bool
{
    return GluaResolver<T>::is(glua, stack_index) || glua->isNull(stack_index);
}
template <typename T>
auto GluaResolver<std::optional<T>>::push(GluaBase* glua,
    const std::optional<T>& value)
    -> void
{
    if (value.has_value()) {
        GluaResolver<T>::push(glua, value.value());
    } else {
        glua->push(std::nullopt);
    }
}

} // namespace kdk::glua
