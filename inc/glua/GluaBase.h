#pragma once

#include "glua/Exceptions.h"
#include "glua/GluaCallable.h"
#include "glua/GluaManagedTypeStorage.h"
#include "glua/ICallable.h"
#include "glua/StackPosition.h"
#include "glua/StringUtil.h"

#include <optional>
#include <string>
#include <type_traits>
#include <typeindex>
#include <unordered_map>
#include <vector>

#define REGISTER_TO_GLUA(glua, functor)                                        \
  glua.RegisterCallable(#functor, (glua).CreateGluaCallable(functor))

#define REGISTER_CLASS_TO_GLUA(glua, ClassType, ...)                           \
  glua.RegisterClassMultiString<ClassType>(#__VA_ARGS__, __VA_ARGS__)

namespace kdk::glua {
/**
 * Base for a Glua instance that can serve as an argument stack for a
 * DeferredArgumentCaller, while providing the shared logic for type deduction
 * used for the various language binding implementation
 *
 */
class GluaBase {
public:
  GluaBase() = default;
  GluaBase(const GluaBase &) = default;
  GluaBase(GluaBase &&) noexcept = default;

  auto operator=(const GluaBase &) -> GluaBase & = default;
  auto operator=(GluaBase &&) noexcept -> GluaBase & = default;
  /** DEFERRED ARGUMENT ArgumentStack CONTRACT IMPLEMENTATION **/

  /**
   * @brief pulls an argument off the stack for the given
   * DeferredArgumentCallable, which must be a GluaCallable
   *
   * @tparam Type the type  of the argument to pull off the argument stack
   * @param callable the callable that is requesting the data
   * @param index the index of the argument, 0 based
   * @return Type the value pulled off the argument stack
   */
  template <typename Type>
  static auto deferred_argument_get(const ICallable *callable, size_t index)
      -> Type;
  /**
   * @brief pushes an argument onto the stack, the argument being the return
   * value from a DeferredArgumentCallable's call, where the callable must be a
   * GluaCallable
   *
   * @tparam Type the type of the parameter pushed onto the stack
   * @param callable the GluaCallable that is pushing a return value
   * @param value the value to push on the stack
   */
  template <typename Type>
  static auto deferred_argument_push(const ICallable *callable, Type &&value)
      -> void;
  /*************************************************************/

  /** GluaBase public interface, implemented by language specific derivations
   * **/

  /**
   * @brief Resets the environment of this instance, effectively removing any
   * globals or changes that have occurred from running scripts. Registered
   * functions and methods are NOT affected, and will still be callable.
   * Effectively preps this instance to run a script from a fresh state
   *
   * @param sandboxed true if the new environment should be a sandbox
   * environment that prevents some security issues like writing to files, etc
   */
  virtual auto ResetEnvironment(bool sandboxed = true) -> void = 0;
  /**
   * @brief registers a callable so that it is available to be called from the
   * scripting environment using the given name
   *
   * @param name the name the callable should be called with from the scripting
   * environment
   * @param callable the callable to execute when called from the scripting
   * environment
   */
  virtual auto RegisterCallable(const std::string &name, Callable callable)
      -> void = 0;
  /**
   * @brief Runs a script, executing the global code
   *
   * @param script_data the script code
   */
  virtual auto RunScript(std::string_view script_data) -> void = 0;
  /**
   * @brief Runs a file, by reading the data in from the file and calling
   * RunScript
   *
   * @param file_name the file to run
   */
  virtual auto RunFile(std::string_view file_name) -> void = 0;
  /*****************************************************************************/

  /**
   * @brief pushes a given value onto the stack and returns the index of the new
   * value
   *
   * @tparam Type the type  of the value to push
   * @param value the value to push onto the stack
   *
   * @return the index of the new value on the stack
   */
  template <typename Type> auto Push(Type &&value) -> int;

  /**
   * @brief pushes a child object of the given object onto the stack. The object
   * at `parent_index` must have a child with `child_index`, which will be
   * pushed on the stack, otherwise a null value is pushed onto the stack.
   *
   * @param parent_index the stack index of the parent object
   * @param child_index the index of the child within the object at
   * `parent_index` to push onto the stack
   * @return the stack position of the retrieved child
   */
  auto PushChild(int parent_index, size_t child_index) -> StackPosition;

  /**
   * @brief pushes a child object of the given object onto the stack. The object
   * at `parent_index` must have a child with `child_key`, which will be pushed
   * on the stack, otherwise a null value is pushed onto the stack.
   *
   * @param parent_index the stack index of the parent object
   * @param child_key the key of the child within the object at `parent_index`
   * to push onto the stack
   * @return the stack position of the retrieved child
   */
  auto PushChild(int parent_index, const std::string &child_key)
      -> StackPosition;

  /**
   * @brief pushes a child object of the given object onto the stack. The object
   * at `parent_index` must have a child with `child_index`, which will be
   * pushed on the stack, otherwise an std::out_of_range is thrown.
   *
   * @param parent_index the stack index of the parent object
   * @param child_index the index of the child within the object at
   * `parent_index` to push onto the stack
   * @return the stack position of the retrieved child
   *
   * @throws std::out_of_range if no such child exists
   */
  auto SafePushChild(int parent_index, size_t child_index) -> StackPosition;

  /**
   * @brief pushes a child object of the given object onto the stack. The object
   * at `parent_index` must have a child with `child_key`, which will be pushed
   * on the stack, otherwise an std::out_of_range is thrown.
   *
   * @param parent_index the stack index of the parent object
   * @param child_key the key of the child within the object at `parent_index`
   * to push onto the stack
   * @return the stack position of the retrieved child
   *
   * @throws std::out_of_range if no such child exists
   */
  auto SafePushChild(int parent_index, const std::string &child_key)
      -> StackPosition;

  /**
   * @brief Gets the value of a child object of the given object as `Type`. The
   * object at `parent_index` must have a child with `child_index`, otherwise
   * the value will be converted from null.
   *
   * @tparam Type the type to convert the value of the child to
   * @param parent_index the stack index of the parent object
   * @param child_index the index of the child within the object at
   * `parent_index` to get the value for
   * @return the converted value
   */
  template <typename Type>
  auto GetChild(int parent_index, size_t child_index) -> Type;
  /**
   * @brief Gets the value of a child object of the given object as `Type`. The
   * object at `parent_index` must have a child with `child_key`, otherwise the
   * value will be converted from null.
   *
   * @tparam Type the type to convert the value of the child to
   * @param parent_index the stack index of the parent object
   * @param child_key the key of the child within the object at `parent_index`
   * to get the value for
   * @return the converted value
   */
  template <typename Type>
  auto GetChild(int parent_index, const std::string &child_key) -> Type;
  /**
   * @brief Gets the value of a child object of the given object as `Type`. The
   * object at `parent_index` must have a child with `child_index`, otherwise
   * std::out_of_range is thrown. If the child at the given index is not already
   * of type `Type` then a std::runtime_error is thrown.
   *
   * @tparam Type the type to get the value of the child as
   * @param parent_index the stack index of the parent object
   * @param child_index the index of the child within the object at
   * `parent_index` to get the value for
   * @return the child's value
   */
  template <typename Type>
  auto SafeGetChild(int parent_index, size_t child_index) -> Type;
  /**
   * @brief Gets the value of a child object of the given object as `Type`. The
   * object at `parent_index` must have a child with `child_key`, otherwise
   * std::out_of_range is thrown. If the child at the given key is not already
   * of type `Type` then a std::runtime_error is thrown.
   *
   * @tparam Type the type to convert the value of the child to
   * @param parent_index the stack index of the parent object
   * @param child_key the key of the child within the object at `parent_index`
   * to get the value for
   * @return the child's value
   */
  template <typename Type>
  auto SafeGetChild(int parent_index, const std::string &child_key) -> Type;

  /**
   * @brief checks if a value of the requested Type is on the stack at the given
   * index
   *
   * @tparam Type the type of the value that should be checked for
   * @param index the index of the element to check (negative indices work from
   *              the top of the stack, e.g. -1  is the top of the stack, 0 is
   * the bottom, and 1 is 1 from the bottom)
   * @return true if the value at that index is of the given Type
   */
  template <typename Type> auto Is(int stack_index) -> bool;

  /**
   * @brief Gets the value of the item at the given position in the stack and
   * converts it to `Type`
   *
   * @tparam Type the type of the value that it should be converted to
   * @param index the index of the element to retrieve (negative indices work
   * from the top of the stack, e.g. -1  is the top of the stack, 0 is the
   *              bottom, and 1 is 1 from the bottom)
   * @return the value at the given index converted to `Type`
   */
  template <typename Type> auto As(int stack_index) -> Type;

  /**
   * @brief Gets the value of the item at the given position in the stack and
   * checks to make sure it is of type `Type`. If so it is returned, otherwise
   * an error is thrown
   *
   *
   * @tparam Type the type of the value that it should be retrieved as
   * @param index the index of the element to retrieve (negative indices work
   * from the top of the stack, e.g. -1  is the top of the stack, 0 is the
   *              bottom, and 1 is 1 from the bottom)
   * @return the value at the given index of type `Type`
   * @throws std::runtime_error if the value at the given index is not of type
   * `Type`
   */
  template <typename Type> auto Get(int stack_index) -> Type;

  /**
   * @brief Gets the value at the top of the stack and pops it off the stack
   *
   * @tparam T the type of value to retrieve
   * @return T the value that was previously at the top of the stack
   */
  template <typename T> auto Pop() -> T;

  /**
   * @brief Sets a global value with the given name to the given value
   *
   * @tparam T the type of value to set the global to
   * @param name the name to give the global value in the scripting environment
   * @param value the value to give the global value
   */
  template <typename T>
  auto SetGlobal(const std::string &name, T value) -> void;

  /**
   * @brief Retrieves the value of the requested global from the scripting
   * environment as a value of the requested type
   *
   * @tparam T the type to receive the global value as
   * @param name the name of the global in the scripting environment to retrieve
   * @return T the value of the global
   */
  template <typename T> auto GetGlobal(const std::string &name) -> T;

  /**
   * @brief Retrieves the value of the requested global from the scripting
   * environment and pushes it on the variable stack. The stack position of the
   * global is returned.
   *
   * @param name the name of the global in the scripting environment to retrieve
   * @return the stack position of the retrieved global on the variable stack
   */
  auto PushGlobal(const std::string &name) -> StackPosition;

  /**
   * @brief Creates a callable object as a GluaCallable from the given functor,
   * so it is capable of retrieving arguments from the Glua stack, and putting
   * return values back on the glua stack
   *
   * @tparam Functor the type of the actual functor for the callable
   * @tparam Params the parameters the functor receives when called
   * @param f the actual functor for the callable
   * @return Callable a wrapped GluaCallable as a Callable
   */
  template <typename Functor, typename... Params>
  auto CreateGluaCallable(Functor &&f) -> Callable;

  /**
   * @brief Creates a callable object as a GluaCallable from the given function
   * pointer, so it is capable of retrieving arguments from the Glua stack, and
   * putting return values back on the glua stack
   *
   * @tparam ReturnType the return type of the function pointer
   * @tparam Params thet parameter types of the function pointer
   * @param callable the actual function pointer for the callable
   * @return Callable a wrapped GluaCallable as a Callable
   */
  template <typename ReturnType, typename... Params>
  auto CreateGluaCallable(ReturnType (*callable)(Params...)) -> Callable;

  /**
   * @brief Creates a callable object as a GluaCallable from the given const
   * method pointer, so it is capable of retrieving arguments from the Glua
   * stack, and putting return values back on the glua stack
   *
   * @tparam ClassType the type of the Class this is a method of
   * @tparam ReturnType the return type of the method
   * @tparam Params the method parameter types
   * @param callable the actual method pointer for the callable
   * @return Callable a wrapped GluaCallable as a Callable
   */
  template <typename ClassType, typename ReturnType, typename... Params>
  auto CreateGluaCallable(ReturnType (ClassType::*callable)(Params...) const)
      -> Callable;

  /**
   * @brief Creates a callable object as a GluaCallable from the given method
   * pointer, so it is capable of retrieving arguments from the Glua stack, and
   * putting return values back on the glua stack
   *
   * @tparam ClassType the type of the Class this is a method of
   * @tparam ReturnType the return type of the method
   * @tparam Params the method parameter types
   * @param callable the actual method pointer for the callable
   * @return Callable a wrapped GluaCallable as a Callable
   */
  template <typename ClassType, typename ReturnType, typename... Params>
  auto CreateGluaCallable(ReturnType (ClassType::*callable)(Params...))
      -> Callable;

  /**
   * @brief Registers a class from a multi string. This string is generally
   * generated from REGISTER_CLASS_TO_GLUA, and it's not recommended to call
   * this method directly
   *
   * @tparam ClassType the class type to register to this instance of glua
   * @tparam Methods the types of the method pointers to register
   * @param method_names a comma separated and fully qualified method pointers,
   * e.g.:
   *                     "&ClassName::Foo, &ClassName::Bar"
   * @param methods the method pointers to bind to the comma separated names in
   * method_names
   */
  template <typename ClassType, typename... Methods>
  auto RegisterClassMultiString(std::string_view method_names,
                                Methods... methods) -> void;

  /**
   * @brief Registers a class to glua with the given names and callables for
   * methods to expose to the scripting environment for this type. This is
   * generally called from RegsiterClassMulitString and it's notrecommended to
   * call this method directly
   *
   * @tparam ClassType the class type to register to this instance of glua
   * @param method_names the names of the methods to register, must be same
   * length as the methods vector NOTE: the method names should still be fully
   * qualified,.e.g "&ClassName::Foo" as this method will deduce the class name
   * to use to register from the fully qualified names
   * @param methods the callables of the methods to register, must be same
   * length as the method_names vector
   */
  template <typename ClassType>
  auto RegisterClass(std::vector<std::string_view> method_names,
                     std::vector<std::unique_ptr<ICallable>> methods) -> void;

  /**
   * @brief Registers a  method to an already registered class
   *
   * @tparam ClassType the already registered class type
   * @param method_name the name of the method
   * @param method the callable for the method
   */
  template <typename ClassType>
  auto RegisterMethod(const std::string &method_name, Callable method) -> void;

  /**
   * @brief Calls a function in the scripting environment with the given name
   * using the given parameters
   *
   * @tparam Params the types of the parameters passed
   * @param function_name the name of the function in the scripting environment
   * to call
   * @param params the parameter values to call the function with
   *
   * @return a vector of the stack positions of all return arguments
   */
  template <typename... Params>
  auto CallScriptFunction(const std::string &function_name, Params &&... params)
      -> std::vector<StackPosition>;

  /**
   * @brief defaulted virtual destructor
   */
  virtual ~GluaBase() = default;

protected:
  /** GluaBase protected interface, implemented by language specific derivations
   * **/
  virtual auto push(std::nullopt_t) -> void = 0;
  virtual auto push(bool value) -> void = 0;
  virtual auto push(int8_t value) -> void = 0;
  virtual auto push(int16_t value) -> void = 0;
  virtual auto push(int32_t value) -> void = 0;
  virtual auto push(int64_t value) -> void = 0;
  virtual auto push(uint8_t value) -> void = 0;
  virtual auto push(uint16_t value) -> void = 0;
  virtual auto push(uint32_t value) -> void = 0;
  virtual auto push(uint64_t value) -> void = 0;
  virtual auto push(float value) -> void = 0;
  virtual auto push(double value) -> void = 0;
  virtual auto push(const char *value) -> void = 0;
  virtual auto push(std::string_view value) -> void = 0;
  virtual auto push(std::string value) -> void = 0;
  virtual auto pushArray(size_t size_hint) -> void = 0;
  virtual auto pushStartMap(size_t size_hint) -> void = 0;
  virtual auto arraySetFromStack() -> void = 0;
  virtual auto mapSetFromStack() -> void = 0;
  virtual auto pushUserType(const std::string &unique_type_name,
                            std::unique_ptr<IManagedTypeStorage> user_storage)
      -> void = 0;
  virtual auto getBool(int stack_index) const -> bool = 0;
  virtual auto getInt8(int stack_index) const -> int8_t = 0;
  virtual auto getInt16(int stack_index) const -> int16_t = 0;
  virtual auto getInt32(int stack_index) const -> int32_t = 0;
  virtual auto getInt64(int stack_index) const -> int64_t = 0;
  virtual auto getUInt8(int stack_index) const -> uint8_t = 0;
  virtual auto getUInt16(int stack_index) const -> uint16_t = 0;
  virtual auto getUInt32(int stack_index) const -> uint32_t = 0;
  virtual auto getUInt64(int stack_index) const -> uint64_t = 0;
  virtual auto getFloat(int stack_index) const -> float = 0;
  virtual auto getDouble(int stack_index) const -> double = 0;
  virtual auto getCharPointer(int stack_index) const -> const char * = 0;
  virtual auto getStringView(int stack_index) const -> std::string_view = 0;
  virtual auto getString(int stack_index) const -> std::string = 0;
  virtual auto getArraySize(int stack_index) const -> size_t = 0;
  virtual auto getArrayValue(size_t index_into_array,
                             int stack_index_of_array) const -> void = 0;
  virtual auto getMapKeys(int stack_index) const
      -> std::vector<std::string> = 0;
  virtual auto getMapValue(const std::string &key, int stack_index_of_map) const
      -> void = 0;
  virtual auto getUserType(const std::string &unique_type_name,
                           int stack_index) const -> IManagedTypeStorage * = 0;
  virtual auto isUserType(const std::string &unique_type_name,
                          int stack_index) const -> bool = 0;
  virtual auto isNull(int stack_index) const -> bool = 0;
  virtual auto isBool(int stack_index) const -> bool = 0;
  virtual auto isInt8(int stack_index) const -> bool = 0;
  virtual auto isInt16(int stack_index) const -> bool = 0;
  virtual auto isInt32(int stack_index) const -> bool = 0;
  virtual auto isInt64(int stack_index) const -> bool = 0;
  virtual auto isUInt8(int stack_index) const -> bool = 0;
  virtual auto isUInt16(int stack_index) const -> bool = 0;
  virtual auto isUInt32(int stack_index) const -> bool = 0;
  virtual auto isUInt64(int stack_index) const -> bool = 0;
  virtual auto isFloat(int stack_index) const -> bool = 0;
  virtual auto isDouble(int stack_index) const -> bool = 0;
  virtual auto isCharPointer(int stack_index) const -> bool = 0;
  virtual auto isStringView(int stack_index) const -> bool = 0;
  virtual auto isString(int stack_index) const -> bool = 0;
  virtual auto isArray(int stack_index) const -> bool = 0;
  virtual auto isMap(int stack_index) const -> bool = 0;
  virtual auto setGlobalFromStack(const std::string &name, int stack_index)
      -> void = 0;
  virtual auto pushGlobal(const std::string &name) -> void = 0;
  virtual auto popOffStack(size_t count) -> void = 0;
  virtual auto getStackTop() -> int = 0;
  virtual auto callScriptFunctionImpl(const std::string &function_name,
                                      size_t arg_count = 0) -> void = 0;
  virtual auto
  registerClassImpl(const std::string &class_name,
                    std::unordered_map<std::string, std::unique_ptr<ICallable>>
                        method_callables) -> void = 0;
  virtual auto registerMethodImpl(const std::string &class_name,
                                  const std::string &method_name,
                                  Callable method) -> void = 0;
  virtual auto transformObjectIndex(size_t index) -> size_t = 0;
  virtual auto transformFunctionParameterIndex(size_t index) -> size_t = 0;
  /********************************************************************************/

private:
  template <typename Type> auto push(const std::vector<Type> &value) -> void;
  template <typename Type>
  auto push(const std::unordered_map<std::string, Type> &value) -> void;

  template <typename Type> auto pushRegisteredType(Type &&custom_type) -> void;

  template <typename Type> auto getArray(int stack_index) -> std::vector<Type>;
  template <typename Type>
  auto getStringMap(int stack_index) -> std::unordered_map<std::string, Type>;
  template <typename Type>
  auto getOptional(int stack_index) -> std::optional<Type>;

  template <typename Type> auto getRegisteredType(int index) -> Type;
  template <typename Type> auto isRegisteredType(int index) -> bool;

  template <typename T>
  auto getUniqueClassName() const
      -> std::optional<std::reference_wrapper<const std::string>>;
  template <typename T>
  auto setUniqueClassName(std::string metatable_name) -> void;

  std::unordered_map<std::type_index, std::string> m_class_to_metatable_name;

  // friends for template resolvers
  template <typename T> friend struct GluaResolver;
};

} // namespace kdk::glua

// include stack position implementation to avoid circular dependency
#include "glua/StackPosition.tcc"

#include "glua/GluaBase.tcc"
