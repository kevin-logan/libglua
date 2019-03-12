#include <glua/FileUtil.h>
#include <glua/GluaLua.h>

#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>

class ExampleClass : public std::enable_shared_from_this<ExampleClass> {
public:
  static auto Create(int64_t value = 0) -> std::shared_ptr<ExampleClass> {
    return std::shared_ptr<ExampleClass>{new ExampleClass{value}};
  }

  auto SetValue(int64_t value) -> void { m_value = value; }
  auto GetValue() const -> int64_t { return m_value; }

  auto ExampleOverload() -> ExampleClass & { return *this; }
  auto ExampleOverload() const -> const ExampleClass & { return *this; }

  auto Increment(std::shared_ptr<ExampleClass> other) {
    m_value += other->GetValue();
  }

private:
  ExampleClass(int64_t value) : m_value(value) {}
  int64_t m_value;
};

class BoxedValue {
public:
  BoxedValue() : m_value(0) {}
  BoxedValue(int64_t value) : m_value(value) {}

  BoxedValue(BoxedValue &&move) = default;
  BoxedValue(const BoxedValue &copy) = default;

  auto SetValue(int64_t value) { m_value = value; }
  auto GetValue() const -> int64_t { return m_value; }

private:
  int64_t m_value;
};

static auto receive_optional_str(std::optional<std::string> o) {
  if (o.has_value()) {
    std::cout << "received_optional_str called with " << o.value() << std::endl;
  } else {
    std::cout << "received_optional_str called with no value" << std::endl;
  }
}

static auto example_sandboxed_environment(kdk::glua::GluaLua &glua,
                                          const std::string &script) -> void {
  std::cout << std::endl << __FUNCTION__ << " starting..." << std::endl;

  glua.CallScriptFunction("example_sandboxed_environment");
  glua.CallScriptFunction("example_sandboxed_environment");

  std::cout << __FUNCTION__ << " reset glua environment" << std::endl;
  glua.ResetEnvironment();

  glua.RunScript(script);
  glua.CallScriptFunction("example_sandboxed_environment");
  glua.CallScriptFunction("example_sandboxed_environment");
}

static auto example_library_functions(kdk::glua::GluaLua &glua) -> void {
  std::cout << std::endl << __FUNCTION__ << " starting..." << std::endl;

  glua.CallScriptFunction("example_library_functions");
}

static auto example_managed_cpp_class(kdk::glua::GluaLua &glua) -> void {
  std::cout << std::endl << __FUNCTION__ << " starting..." << std::endl;

  // to bind overloaded method we must manually bind the overload we desire
  glua.RegisterMethod<ExampleClass>(
      "ExampleOverload",
      glua.CreateGluaCallable(static_cast<ExampleClass &(ExampleClass::*)()>(
          &ExampleClass::ExampleOverload)));

  auto param = ExampleClass::Create();

  std::cout << "reference param before any calls has value: "
            << param->GetValue() << std::endl;
  // pass reference with reference_wrapper
  glua.CallScriptFunction("example_managed_cpp_class", std::ref(*param));
  std::cout << "reference param after one call has value: " << param->GetValue()
            << std::endl;
  // pass reference with shared_ptr
  glua.CallScriptFunction("example_managed_cpp_class", param);
  std::cout << "reference param after two calls has value: "
            << param->GetValue() << std::endl;
  // pass by value will use a copy
  glua.CallScriptFunction("example_managed_cpp_class", *param);
  std::cout << "reference param after all calls has value: "
            << param->GetValue() << std::endl;
}

static auto example_binding(std::string_view some_str, double some_double)
    -> std::string {
  std::stringstream stream;
  stream << "example_binding called with (" << some_str << ", " << some_double
         << ")" << std::endl;
  return stream.str();
}

static auto example_simple_binding(kdk::glua::GluaLua &glua) -> void {
  std::cout << std::endl << __FUNCTION__ << " starting..." << std::endl;

  glua.CallScriptFunction("example_simple_binding");
}

static auto example_global_value(kdk::glua::GluaLua &glua) -> void {
  std::cout << std::endl << __FUNCTION__ << " starting..." << std::endl;

  glua.SetGlobal("global_boxed_value", BoxedValue{1337});
  glua.CallScriptFunction("example_global_value");
}

static auto example_inferred_storage_cpp_class(kdk::glua::GluaLua &glua)
    -> void {
  std::cout << std::endl << __FUNCTION__ << " starting..." << std::endl;

  glua.CallScriptFunction("example_inferred_storage_cpp_class");
}

static auto example_callable_from_cpp(kdk::glua::GluaLua &glua) -> void {
  std::cout << std::endl << __FUNCTION__ << " starting..." << std::endl;

  auto retvals =
      glua.CallScriptFunction("example_callable_from_cpp", 1337, "herpaderp");

  // expect two return values
  auto first_return = retvals[0].As<std::string>();
  auto second_return = retvals[1].As<int64_t>();

  std::cout << "example_callable_from_cpp returned: " << first_return << ", "
            << second_return << std::endl;
}

static auto example_reverse_array(kdk::glua::GluaLua &glua) -> void {
  std::cout << std::endl << __FUNCTION__ << " starting..." << std::endl;

  std::vector<BoxedValue> v{BoxedValue{1}, BoxedValue{2}, BoxedValue{3},
                            BoxedValue{4}, BoxedValue{5}};
  auto retvals = glua.CallScriptFunction("example_reverse_array", v);

  auto reversed_array = retvals[0].As<std::vector<BoxedValue>>();

  std::cout << "reverse_array returned:";
  for (auto &val : reversed_array) {
    std::cout << " " << val.GetValue();
  }
  std::cout << std::endl;
}

static auto example_mutate_value(kdk::glua::GluaLua &glua) -> void {
  std::cout << std::endl << __FUNCTION__ << " starting..." << std::endl;

  BoxedValue our_value{5};

  glua.CallScriptFunction("example_mutate_value", our_value);
  std::cout << "mutate_boxed_value called without reference, value: "
            << our_value.GetValue() << std::endl;

  glua.CallScriptFunction("example_mutate_value", std::ref(our_value));
  std::cout << "mutate_boxed_value called with reference, value: "
            << our_value.GetValue() << std::endl;
}

static auto example_create_and_retrieve_global(kdk::glua::GluaLua &glua)
    -> void {
  std::cout << std::endl << __FUNCTION__ << " starting..." << std::endl;

  glua.CallScriptFunction("example_create_and_retrieve_global");

  std::cout << "glua global example_lua_global: "
            << glua.GetGlobal<std::string>("example_lua_global") << std::endl;
}

enum class ExampleEnum { ONE = 1, TWO = 2, RED = 3, BLACK = 4 };
static auto example_enumeration(kdk::glua::GluaLua &glua) -> void {
  std::cout << std::endl << __FUNCTION__ << " starting..." << std::endl;

  auto retvals =
      glua.CallScriptFunction("example_enumeration", ExampleEnum::BLACK);
  auto ret_val = retvals[0].As<ExampleEnum>();

  switch (ret_val) {
  case ExampleEnum::ONE:
    std::cout << "received ONE" << std::endl;
    break;
  case ExampleEnum::TWO:
    std::cout << "received TWO" << std::endl;
    break;
  case ExampleEnum::RED:
    std::cout << "received RED" << std::endl;
    break;
  case ExampleEnum::BLACK:
    std::cout << "received BLACK" << std::endl;
    break;
  }
}

template <typename ClassType, typename ValueType> class TemplateExample {
public:
  TemplateExample(ValueType v) : m_value(std::move(v)) {}

  auto GetValue() const -> const ValueType & { return m_value; }

private:
  ValueType m_value;
};

// specialize GluaResolver for our TemplateExample type
namespace kdk::glua {
template <typename ClassType, typename ValueType>
struct GluaResolver<TemplateExample<ClassType, ValueType>> {
  static auto as(GluaBase *glua, int stack_index)
      -> TemplateExample<ClassType, ValueType> {
    // get as value type
    return {GluaResolver<ValueType>::as(glua, stack_index)};
  }
  static auto is(GluaBase *glua, int stack_index) -> bool {
    return GluaResolver<ValueType>::is(glua, stack_index);
  }
  static auto push(GluaBase *glua, TemplateExample<ClassType, ValueType> value)
      -> void {
    // push as value type
    GluaResolver<ValueType>::push(glua, value.GetValue());
  }
};
} // namespace kdk::glua

static auto example_custom_template_binding(kdk::glua::GluaLua &glua) -> void {
  std::cout << std::endl << __FUNCTION__ << " starting..." << std::endl;

  glua.CallScriptFunction("example_custom_template_binding",
                          TemplateExample<BoxedValue, std::string>{"herp"});
  glua.CallScriptFunction("example_custom_template_binding",
                          TemplateExample<ExampleClass, int64_t>{1337});
}

static auto example_optionals(kdk::glua::GluaLua &glua) -> void {
  std::cout << std::endl << __FUNCTION__ << " starting..." << std::endl;

  REGISTER_TO_GLUA(glua, receive_optional_str);

  std::optional<std::string> opt_str;
  std::optional<int64_t> opt_int;

  glua.CallScriptFunction("example_optionals", opt_str, opt_int);

  opt_str = "herp";
  opt_int = 1337;
  glua.CallScriptFunction("example_optionals", opt_str, opt_int);
}

static auto example_nested_table(kdk::glua::GluaLua &glua) -> void {
  std::cout << std::endl << __FUNCTION__ << " starting..." << std::endl;

  auto retvals = glua.CallScriptFunction("example_nested_table");
  auto &return_table = retvals[0];
  {
    /* now a table like this is on the stack:
    {
        level_one = {
            level_two = {
                level_three = {
                    value = 1337
                },
                value = 3
            },
            value = 2
        },
        value = 1
    }
    */
    auto first_value = return_table.PushChild("value").Get<int32_t>();
    auto first_nest_index = return_table.PushChild("level_one");
    {
      auto second_value = first_nest_index.PushChild("value").Get<int32_t>();
      auto second_nest_index = first_nest_index.PushChild("level_two");
      {
        auto third_value = second_nest_index.PushChild("value").Get<int32_t>();
        auto third_nest_index = second_nest_index.PushChild("level_three");
        {
          auto final_value = third_nest_index.PushChild("value").Get<int32_t>();

          std::cout << __PRETTY_FUNCTION__ << " results:" << std::endl;
          std::cout << "\tgot outmost value: " << first_value << std::endl;
          std::cout << "\tgot level_one value: " << second_value << std::endl;
          std::cout << "\tgot level_two value: " << third_value << std::endl;
          std::cout << "\tgot level_three value: " << final_value << std::endl;
        }
      }
    }
  }
}

static auto example_bind_lambda(kdk::glua::GluaLua &glua) -> void {
  std::cout << std::endl << __FUNCTION__ << " starting..." << std::endl;
  auto callable = glua.CreateGluaCallable([](int number, int power) {
    int result = 1;
    while (power-- > 0) {
      result *= number;
    }
    return result;
  });

  glua.RegisterCallable("example_lambda", std::move(callable));

  glua.CallScriptFunction("example_bind_lambda");
}

static auto example_lua_array(kdk::glua::GluaLua &glua) -> void {
  std::cout << std::endl << __FUNCTION__ << " starting..." << std::endl;
  auto retvals = glua.CallScriptFunction("example_lua_array");

  auto &array_position = retvals[0];

  auto length = array_position.GetArrayLength();

  std::cout << "example_lua_array returned array with " << length << " children"
            << std::endl;

  for (size_t i = 0; i < length; ++i) {
    std::cout << "\tchild[" << i
              << "]: " << array_position.PushChild(i).As<std::string>()
              << std::endl;
  }
}

auto main(int argc, char *argv[]) -> int {
  kdk::glua::GluaLua glua{std::cout};

  ///////////// ADD CUSTOM BINDINDS HERE /////////////
  REGISTER_TO_GLUA(glua, example_binding);
  REGISTER_CLASS_TO_GLUA(glua, ExampleClass, &ExampleClass::GetValue,
                         &ExampleClass::SetValue, &ExampleClass::Increment);
  REGISTER_CLASS_TO_GLUA(glua, BoxedValue, &BoxedValue::GetValue,
                         &BoxedValue::SetValue);

  if (argc != 2) {
    std::cout << "Usage: " << argv[0] << " <path_to_example_lua_script>"
              << std::endl;
  } else {
    std::string script = kdk::file_util::read_all(argv[1]);

    if (script.empty()) {
      std::cout << "File [" << argv[1] << "] not found or was invalid"
                << std::endl;
    }

    auto retvals = glua.RunScript(script);
    for (auto &retval : retvals) {
      std::cout << "RunScript returned: " << retval.As<std::string>()
                << std::endl;
    }

    example_sandboxed_environment(glua, script);
    example_library_functions(glua);
    example_managed_cpp_class(glua);
    example_simple_binding(glua);
    example_global_value(glua);
    example_inferred_storage_cpp_class(glua);
    example_callable_from_cpp(glua);
    example_reverse_array(glua);
    example_mutate_value(glua);
    example_create_and_retrieve_global(glua);
    example_enumeration(glua);
    example_custom_template_binding(glua);
    example_optionals(glua);
    example_nested_table(glua);
    example_bind_lambda(glua);
    example_lua_array(glua);
  }

  return 0;
}
