#pragma once

#include <optional>

namespace kdk::glua
{
class GluaBase;

/**
 * Class abstracting the concept of a position on the glua stack. An RAII object where
 * the value on the stack will be popped when it goes out of scope.
 *
 * Unfortunately this creates a complexity where StackPositions must never be used in
 * a way where they destruct in an order different than their construction order, so
 * this class is only intended to be used as l-values on the stack, rather than using
 * something like unique/shared_ptrs to extend or modify their lifetimes.
 */
class StackPosition
{
public:
    /**
     * Constructs a new stack position to represent the (already existing) value at
     * the given index
     *
     * @param glua the glua instance this stack position belongs to
     * @param position the position on the glua stack for this value
     */
    StackPosition(GluaBase* glua, int position);

    StackPosition(const StackPosition&) = delete;
    auto operator=(const StackPosition&) -> StackPosition& = delete;

    /**
     * Move constructor/assignment, move the stack position out of the rhs
     * and into this one. rhs is left without a valid index and should not
     * be used again
     *
     * @{
     */
    StackPosition(StackPosition&& rhs) noexcept;
    auto operator=(StackPosition&& rhs) noexcept -> StackPosition&;
    /** @} */

    /**
     * @return the stack index for this value
     */
    auto GetStackIndex() const -> int;
    /**
     * @brief Releases the stack index from this position without popping it
     * off the stack. This object must not be used again and calling code now
     * must manage the stack index and make sure it gets popped appropriately
     */
    auto Release() -> void;

    /**
     * Pushes the child of this object with the given index onto the stack
     * and returns a StackPosition referring to it. If the child doesn't
     * exist a null value will be pushed onto the stack.
     *
     * @param child_index the index of the child to push onto the stack
     * @return the stack position of the child.
     *
     * @throws std::bad_optional_access if this StackPosition no longer
     * holds an index (either it's been moved or Release was called).
     */
    auto PushChild(size_t child_index) -> StackPosition;
    /**
     * Pushes the child of this object with the given key onto the stack
     * and returns a StackPosition referring to it. If the child doesn't
     * exist a null value will be pushed onto the stack.
     *
     * @param child_key the key of the child to push onto the stack
     * @return the stack position of the child.
     *
     * @throws std::bad_optional_access if this StackPosition no longer
     * holds an index (either it's been moved or Release was called).
     */
    auto PushChild(const std::string& child_key) -> StackPosition;

    /**
     * Pushes the child of this object with the given index onto the stack
     * and returns a StackPosition referring to it. If the child doesn't
     * exist an exception is thrown.
     *
     * @param child_index the index of the child to push onto the stack
     * @return the stack position of the child.
     *
     * @throws std::bad_optional_access if this StackPosition no longer
     * holds an index (either it's been moved or Release was called).
     *
     * @throws std::out_of_range if there is no child with the given
     * index
     */
    auto SafePushChild(size_t child_index) -> StackPosition;
    /**
     * Pushes the child of this object with the given key onto the stack
     * and returns a StackPosition referring to it.If the child doesn't
     * exist an exception is thrown.
     *
     * @param child_key the key of the child to push onto the stack
     * @return the stack position of the child.
     *
     * @throws std::bad_optional_access if this StackPosition no longer
     * holds an index (either it's been moved or Release was called).
     *
     * @throws std::out_of_range if there is no child with the given
     * key
     */
    auto SafePushChild(const std::string& child_key) -> StackPosition;

    /**
     * @tparam Type the type to convert this item to
     * @return The value of the item this position refers to converted to
     * the requested type
     */
    template<typename Type>
    auto As() -> Type;

    /**
     * @tparam Type the Type this value against
     * @return true if the item this position refers to is of the given type
     */
    template<typename Type>
    auto Is() -> bool;

    /**
     * @tparam Type the expected type of this value
     * @return the value of this item with the given type
     *
     * @throws std::runtime_error if the item was not of the requested type
     */
    template<typename Type>
    auto Get() -> Type;

    /**
     * @brief Destructor which pops the item off the top of the stack
     */
    ~StackPosition();

private:
    GluaBase*          m_glua;     ///< The glua instance this stack position belongs to
    std::optional<int> m_position; ///< The stack index of this item
};
} // namespace kdk::glua

// .tcc implementation file is included by GluaBase.h instead to avoid circular dependency