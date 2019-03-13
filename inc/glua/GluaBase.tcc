#include "glua/GluaBase.h"
#include "glua/GluaBaseHelperTemplates.h"

namespace kdk::glua {
/** DEFERRED ARGUMENT ArgumentStack CONTRACT IMPLEMENTATION **/
template <typename Type>
auto GluaBase::deferred_argument_get(const ICallable *callable, size_t index)
    -> Type {
  auto *glua_ptr = static_cast<GluaBase *>(callable->GetImplementationData());
  return glua_ptr->Get<Type>(
      static_cast<int>(glua_ptr->transformFunctionParameterIndex(index)));
}
template <typename Type>
auto GluaBase::deferred_argument_push(const ICallable *callable, Type &&value)
    -> void {
  auto *glua_ptr = static_cast<GluaBase *>(callable->GetImplementationData());
  glua_ptr->Push(std::forward<Type>(value));
}
/*************************************************************/

template <typename Type> auto GluaBase::Push(Type &&value) -> int {
  // can't push by anything other than value, so decay
  // If user wants to push a reference (which they must guarantee the
  // lifetime of) they can use std::reference_wrapper which would then
  // be passed by value
  GluaResolver<std::decay_t<Type>>::push(this, std::forward<Type>(value));

  return getStackTop();
}
template <typename Type>
auto GluaBase::GetChild(int parent_index, size_t child_index) -> Type {
  auto result_index = PushChild(parent_index, child_index);

  auto retval = As<Type>(result_index);

  popOffStack(1);

  return retval;
}

template <typename Type>
auto GluaBase::GetChild(int parent_index, const std::string &child_key)
    -> Type {
  auto result_index = PushChild(parent_index, child_key);

  auto retval = As<Type>(result_index);

  popOffStack(1);

  return retval;
}

template <typename Type>
auto GluaBase::SafeGetChild(int parent_index, size_t child_index) -> Type {
  auto result_index = SafePushChild(parent_index, child_index);

  if (Is<Type>(result_index)) {
    auto retval = As<Type>(result_index);

    popOffStack(1);

    return retval;
  }

  popOffStack(1);
  throw std::runtime_error("Child of invalid type at index: " +
                           std::to_string(child_index));
}

template <typename Type>
auto GluaBase::SafeGetChild(int parent_index, const std::string &child_key)
    -> Type {
  auto result_index = SafePushChild(parent_index, child_key);

  if (Is<Type>(result_index)) {
    auto retval = As<Type>(result_index);

    popOffStack(1);

    return retval;
  }

  popOffStack(1);
  throw std::runtime_error("Child of invalid type with key: " + child_key);
}

template <typename Type> auto GluaBase::Is(int stack_index) -> bool {
  return GluaResolver<Type>::is(this, stack_index);
}

template <typename Type> auto GluaBase::As(int stack_index) -> Type {
  return GluaResolver<Type>::as(this, stack_index);
}

template <typename Type> auto GluaBase::Get(int stack_index) -> Type {
  if (Is<Type>(stack_index)) {
    return As<Type>(stack_index);
  }

  throw std::runtime_error("GluaBase::Get with invalid type");
}

template <typename T> auto GluaBase::Pop() -> T {
  if constexpr (std::is_same<T, void>::value) {
    popOffStack(1);
  } else {
    auto val = Get<T>(-1);
    popOffStack(1);

    return val;
  }
}

template <typename T> auto GluaBase::GetGlobal(const std::string &name) -> T {
  pushGlobal(name);
  auto val = Get<T>(-1);
  popOffStack(1);

  return val;
}

template <typename T>
auto GluaBase::SetGlobal(const std::string &name, T value) -> void {
  Push(std::move(value));
  setGlobalFromStack(name, -1);
  popOffStack(1); // pop off the value we pushed
}

template <typename Functor>
auto GluaBase::CreateGluaCallable(Functor &&f) -> Callable {
  return createGluaCallableImpl(std::forward<Functor>(f));
}

template <typename Method, typename... Methods>
auto emplace_methods(GluaBase &lua, std::vector<std::unique_ptr<ICallable>> &v,
                     Method m, Methods... methods) -> void {
  v.emplace_back(lua.CreateGluaCallable(m).AcquireCallable());

  if constexpr (sizeof...(Methods) > 0) {
    emplace_methods(lua, v, methods...);
  }
}

template <typename ClassType, typename... Methods>
auto GluaBase::RegisterClassMultiString(std::string_view method_names,
                                        Methods... methods) -> void {
  // split method names (as parameters) into vector, trim whitespace
  auto comma_separated = string_util::remove_all_whitespace(method_names);

  auto method_names_vector =
      string_util::split(comma_separated, std::string_view{","});

  std::vector<std::unique_ptr<ICallable>> methods_vector;
  methods_vector.reserve(sizeof...(Methods));
  emplace_methods(*this, methods_vector, methods...);

  RegisterClass<ClassType>(method_names_vector, std::move(methods_vector));
}

template <typename T> auto default_construct_type() -> T { return T{}; }

template <typename ClassType>
auto GluaBase::RegisterClass(std::vector<std::string_view> method_names,
                             std::vector<std::unique_ptr<ICallable>> methods)
    -> void {
  std::string class_name;
  auto name_pos = method_names.begin();
  std::unordered_map<std::string, std::unique_ptr<ICallable>> method_callables;

  for (auto &method : methods) {
    if (name_pos != method_names.end()) {
      auto full_name = *name_pos;
      ++name_pos;

      auto last_scope = full_name.rfind("::");

      if (last_scope != std::string_view::npos) {
        auto method_class = full_name.substr(0, last_scope);
        auto method_name = full_name.substr(last_scope + 2);

        // get rid of & from &ClassName::MethodName syntax
        if (!method_class.empty() && method_class.front() == '&') {
          method_class.remove_prefix(1);
        }

        if (class_name.empty()) {
          class_name = std::string{method_class};
        }

        if (class_name == method_class) {
          // add method to metatable
          method_callables.emplace(std::string{method_name}, std::move(method));
        } else {
          throw std::logic_error(
              "RegisterClass had methods from various classes, only one class "
              "may be registered per call");
        }
      } else {
        throw std::logic_error("RegisterClass had method of invalid format, "
                               "expecting ClassName::MethodName");
      }
    } else {
      throw std::logic_error(
          "RegisterClass with fewer method names than methods!");
    }
  }

  // if we got here and have a class name then we had no expections, build Glua
  // bindings
  if (!class_name.empty()) {
    setUniqueClassName<ClassType>(class_name);
    registerClassImpl(class_name, std::move(method_callables));

    // now register the Create function if it exists
    if constexpr (HasCreate<ClassType>::value) {
      auto create_callable = CreateGluaCallable(&ClassType::Create);
      RegisterCallable("Create" + class_name, std::move(create_callable));
    }

    if constexpr (std::is_default_constructible<ClassType>::value) {
      auto construct_callable =
          CreateGluaCallable(&default_construct_type<ClassType>);
      RegisterCallable("Construct" + class_name, std::move(construct_callable));
    }
  }
}

template <typename ClassType>
auto GluaBase::RegisterMethod(const std::string &method_name, Callable method)
    -> void {
  auto unique_name_opt = getUniqueClassName<ClassType>();

  if (unique_name_opt.has_value()) {
    registerMethodImpl(unique_name_opt.value(), method_name, std::move(method));
  } else {
    throw exceptions::LuaException(
        "Tried to register method to unregistered class [no metatable]");
  }
}

template <typename... Params>
auto GluaBase::CallScriptFunction(const std::string &function_name,
                                  Params &&... params)
    -> std::vector<StackPosition> {
  auto previous_top = getStackTop();

  // push all params onto the stack
  ((Push(std::forward<Params>(params))), ...);

  callScriptFunctionImpl(function_name, sizeof...(params));

  auto new_top = getStackTop();

  std::vector<StackPosition> results;

  if (new_top > previous_top) {
    auto value_count = static_cast<size_t>(new_top - previous_top);
    results.reserve(value_count);

    for (auto return_index = previous_top + 1; return_index <= new_top;
         ++return_index) {
      results.emplace_back(this, return_index);
    }
  }

  return results;
}

template <typename T>
auto GluaBase::getUniqueClassName() const
    -> std::optional<std::reference_wrapper<const std::string>> {
  auto index = std::type_index{typeid(T)};

  auto pos = m_class_to_metatable_name.find(index);

  if (pos != m_class_to_metatable_name.end()) {
    return std::cref(pos->second);
  }

  return std::nullopt;
}

template <typename T>
auto GluaBase::setUniqueClassName(std::string metatable_name) -> void {
  auto index = std::type_index{typeid(T)};

  m_class_to_metatable_name[index] = std::move(metatable_name);
}

template <typename Functor>
auto GluaBase::createGluaCallableImpl(Functor f) -> Callable {
  // must deduce parameters to this functor, get the call operator
  auto operator_ptr = &Functor::operator();

  return createGluaCallableImpl(std::move(f), operator_ptr);
}

template <typename Functor, typename ReturnType, typename... Params>
auto GluaBase::createGluaCallableImpl(
    Functor f, ReturnType (Functor::*reference_call_operator)(Params...))
    -> Callable {
  // we only needed this to deduce Params, but having a name is nice
  (void)reference_call_operator;

  return Callable{
      std::make_unique<GluaCallable<Functor, Params...>>(this, std::move(f))};
}

template <typename Functor, typename ReturnType, typename... Params>
auto GluaBase::createGluaCallableImpl(
    Functor f, ReturnType (Functor::*reference_call_operator)(Params...) const)
    -> Callable {
  // we only needed this to deduce Params, but having a name is nice
  (void)reference_call_operator;

  return Callable{
      std::make_unique<GluaCallable<Functor, Params...>>(this, std::move(f))};
}

template <typename ReturnType, typename... Params>
auto GluaBase::createGluaCallableImpl(ReturnType (*callable)(Params...))
    -> Callable {
  return Callable{std::make_unique<GluaCallable<decltype(callable), Params...>>(
      this, callable)};
}

template <typename ClassType, typename ReturnType, typename... Params>
auto GluaBase::createGluaCallableImpl(
    ReturnType (ClassType::*callable)(Params...) const) -> Callable {
  auto method_call_lambda = [callable](const ClassType &object,
                                       Params... params) -> ReturnType {
    return (object.*callable)(std::forward<Params>(params)...);
  };

  return Callable{std::make_unique<
      GluaCallable<decltype(method_call_lambda), const ClassType &, Params...>>(
      this, std::move(method_call_lambda))};
}

template <typename ClassType, typename ReturnType, typename... Params>
auto GluaBase::createGluaCallableImpl(
    ReturnType (ClassType::*callable)(Params...)) -> Callable {
  auto method_call_lambda = [callable](ClassType &object,
                                       Params... params) -> ReturnType {
    return (object.*callable)(std::forward<Params>(params)...);
  };

  return Callable{std::make_unique<
      GluaCallable<decltype(method_call_lambda), ClassType &, Params...>>(
      this, std::move(method_call_lambda))};
}

} // namespace kdk::glua
