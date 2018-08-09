#pragma once

namespace kdk::glua
{
enum class ManagedTypeStorage
{
    RAW_PTR,
    SHARED_PTR,
    STACK_ALLOCATED
};

class IManagedTypeStorage
{
public:
    IManagedTypeStorage()                               = default;
    IManagedTypeStorage(const IManagedTypeStorage&)     = default;
    IManagedTypeStorage(IManagedTypeStorage&&) noexcept = default;

    auto operator=(const IManagedTypeStorage&) -> IManagedTypeStorage& = default;
    auto operator=(IManagedTypeStorage&&) noexcept -> IManagedTypeStorage& = default;

    virtual auto GetStorageType() const -> ManagedTypeStorage = 0;

    virtual ~IManagedTypeStorage() = default;
};

template<typename T>
class ManagedTypeSharedPtr : public IManagedTypeStorage
{
public:
    explicit ManagedTypeSharedPtr(std::shared_ptr<T> value) : m_value(value) {}
    ManagedTypeSharedPtr(const ManagedTypeSharedPtr&)     = default;
    ManagedTypeSharedPtr(ManagedTypeSharedPtr&&) noexcept = default;

    auto operator=(const ManagedTypeSharedPtr&) -> ManagedTypeSharedPtr& = default;
    auto operator=(ManagedTypeSharedPtr&&) noexcept -> ManagedTypeSharedPtr& = default;

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
    explicit ManagedTypeRawPtr(T* value) : m_value(value) {}
    ManagedTypeRawPtr(const ManagedTypeRawPtr&)     = default;
    ManagedTypeRawPtr(ManagedTypeRawPtr&&) noexcept = default;

    auto operator=(const ManagedTypeRawPtr&) -> ManagedTypeRawPtr& = default;
    auto operator=(ManagedTypeRawPtr&&) noexcept -> ManagedTypeRawPtr& = default;

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
    explicit ManagedTypeStackAllocated(T value) : m_value(std::move(value)) {}
    ManagedTypeStackAllocated(const ManagedTypeStackAllocated&)     = default;
    ManagedTypeStackAllocated(ManagedTypeStackAllocated&&) noexcept = default;

    auto operator=(const ManagedTypeStackAllocated&) -> ManagedTypeStackAllocated& = default;
    auto operator=(ManagedTypeStackAllocated&&) noexcept -> ManagedTypeStackAllocated& = default;

    auto GetStorageType() const -> ManagedTypeStorage override { return ManagedTypeStorage::STACK_ALLOCATED; }

    auto GetValue() const -> const T& { return m_value; }
    auto GetValue() -> T& { return m_value; }

    ~ManagedTypeStackAllocated() override = default;

private:
    T m_value;
};

} // namespace kdk::glua
