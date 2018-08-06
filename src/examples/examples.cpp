#include "FileUtil.h"
#include "Glua.h"

#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>

class ExampleClass : public std::enable_shared_from_this<ExampleClass>
{
public:
    static auto Create(int64_t value = 0) -> std::shared_ptr<ExampleClass>
    {
        return std::shared_ptr<ExampleClass>{new ExampleClass{value}};
    }

    auto SetValue(int64_t value) -> void { m_value = value; }
    auto GetValue() const -> int64_t { return m_value; }

    auto ExampleOverload() -> ExampleClass& { return *this; }
    auto ExampleOverload() const -> const ExampleClass& { return *this; }

    auto Increment(std::shared_ptr<ExampleClass> other) { m_value += other->GetValue(); }

private:
    ExampleClass(int64_t value) : m_value(value) {}
    int64_t m_value;
};

class BoxedValue
{
public:
    BoxedValue() : m_value(0) {}
    BoxedValue(int64_t value) : m_value(value) {}

    BoxedValue(BoxedValue&& move)      = default;
    BoxedValue(const BoxedValue& copy) = default;

    auto SetValue(int64_t value) { m_value = value; }
    auto GetValue() const -> int64_t { return m_value; }

private:
    int64_t m_value;
};

static auto receive_optional_str(std::optional<std::string> o)
{
    if (o.has_value())
    {
        std::cout << "received_optional_str called with " << o.value() << std::endl;
    }
    else
    {
        std::cout << "received_optional_str called with no value" << std::endl;
    }
}

static auto example_sandboxed_environment(kdk::glua::Glua::Ptr glua, const std::string& script) -> void
{
    std::cout << std::endl << __FUNCTION__ << " starting..." << std::endl;

    glua->CallScriptFunction("example_sandboxed_environment");
    glua->CallScriptFunction("example_sandboxed_environment");

    std::cout << __FUNCTION__ << " reset glua environment" << std::endl;
    glua->ResetEnvironment();

    glua->RunScript(script);
    glua->CallScriptFunction("example_sandboxed_environment");
    glua->CallScriptFunction("example_sandboxed_environment");
}

static auto example_library_functions(kdk::glua::Glua::Ptr glua) -> void
{
    std::cout << std::endl << __FUNCTION__ << " starting..." << std::endl;

    glua->CallScriptFunction("example_library_functions");
}

static auto example_managed_cpp_class(kdk::glua::Glua::Ptr glua) -> void
{
    std::cout << std::endl << __FUNCTION__ << " starting..." << std::endl;

    // to bind overloaded method we must manually bind the overload we desire
    glua->RegisterMethod<ExampleClass>(
        "ExampleOverload",
        glua->CreateLuaCallable(static_cast<ExampleClass& (ExampleClass::*)()>(&ExampleClass::ExampleOverload)));

    glua->CallScriptFunction("example_managed_cpp_class");
}

static auto example_binding(std::string_view some_str, double some_double) -> std::string
{
    std::stringstream stream;
    stream << "example_binding called with (" << some_str << ", " << some_double << ")" << std::endl;
    return stream.str();
}

static auto example_simple_binding(kdk::glua::Glua::Ptr glua) -> void
{
    std::cout << std::endl << __FUNCTION__ << " starting..." << std::endl;

    glua->CallScriptFunction("example_simple_binding");
}

static auto example_global_value(kdk::glua::Glua::Ptr glua) -> void
{
    std::cout << std::endl << __FUNCTION__ << " starting..." << std::endl;

    glua->SetGlobal("global_boxed_value", BoxedValue{1337});
    glua->CallScriptFunction("example_global_value");
}

static auto example_inferred_storage_cpp_class(kdk::glua::Glua::Ptr glua) -> void
{
    std::cout << std::endl << __FUNCTION__ << " starting..." << std::endl;

    glua->CallScriptFunction("example_inferred_storage_cpp_class");
}

static auto example_callable_from_cpp(kdk::glua::Glua::Ptr glua) -> void
{
    std::cout << std::endl << __FUNCTION__ << " starting..." << std::endl;

    glua->CallScriptFunction("example_callable_from_cpp", 1337, "herpaderp");

    // expect two return values
    auto second_return = glua->Pop<int64_t>();
    auto first_return  = glua->Pop<std::string>();

    std::cout << "example_callable_from_cpp returned: " << first_return << ", " << second_return << std::endl;
}

static auto example_reverse_array(kdk::glua::Glua::Ptr glua) -> void
{
    std::cout << std::endl << __FUNCTION__ << " starting..." << std::endl;

    std::vector<BoxedValue> v{BoxedValue{1}, BoxedValue{2}, BoxedValue{3}, BoxedValue{4}, BoxedValue{5}};
    glua->CallScriptFunction("example_reverse_array", v);

    auto reversed_array = glua->Pop<std::vector<BoxedValue>>();

    std::cout << "reverse_array returned:";
    for (auto& val : reversed_array)
    {
        std::cout << " " << val.GetValue();
    }
    std::cout << std::endl;
}

static auto example_mutate_value(kdk::glua::Glua::Ptr glua) -> void
{
    std::cout << std::endl << __FUNCTION__ << " starting..." << std::endl;

    BoxedValue our_value{5};

    glua->CallScriptFunction("example_mutate_value", our_value);
    std::cout << "mutate_boxed_value called without reference, value: " << our_value.GetValue() << std::endl;

    glua->CallScriptFunction("example_mutate_value", std::ref(our_value));
    std::cout << "mutate_boxed_value called with reference, value: " << our_value.GetValue() << std::endl;
}

static auto example_create_and_retrieve_global(kdk::glua::Glua::Ptr glua) -> void
{
    std::cout << std::endl << __FUNCTION__ << " starting..." << std::endl;

    glua->CallScriptFunction("example_create_and_retrieve_global");

    std::cout << "glua global example_lua_global: " << glua->GetGlobal<std::string>("example_lua_global") << std::endl;
}

enum class ExampleEnum
{
    ONE   = 1,
    TWO   = 2,
    RED   = 3,
    BLACK = 4
};
static auto example_enumeration(kdk::glua::Glua::Ptr glua) -> void
{
    std::cout << std::endl << __FUNCTION__ << " starting..." << std::endl;

    glua->CallScriptFunction("example_enumeration", ExampleEnum::BLACK);
    auto ret_val = glua->Pop<ExampleEnum>();

    switch (ret_val)
    {
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

template<typename ClassType, typename ValueType>
class TemplateExample
{
public:
    TemplateExample(ValueType v) : m_value(std::move(v)) {}

    auto GetValue() const -> const ValueType& { return m_value; }

private:
    ValueType m_value;
};

namespace kdk::glua
{
template<typename ClassType, typename ValueType>
struct CustomTypeHandler<TemplateExample<ClassType, ValueType>>
{
    static auto get(lua_State* state, size_t stack_index) -> TemplateExample<ClassType, ValueType>
    {
        // get as value type
        return TemplateExample<ClassType, ValueType>{kdk::glua::Glua::get<ValueType>(state, stack_index)};
    }
    static auto push(lua_State* state, TemplateExample<ClassType, ValueType> value) -> void
    {
        // get as value type
        Glua::GetInstanceFromState(state).Push(value.GetValue());
    }
};
} // namespace kdk::glua

static auto example_custom_template_binding(kdk::glua::Glua::Ptr glua) -> void
{
    std::cout << std::endl << __FUNCTION__ << " starting..." << std::endl;

    glua->CallScriptFunction("example_custom_template_binding", TemplateExample<BoxedValue, std::string>{"herp"});
    glua->CallScriptFunction("example_custom_template_binding", TemplateExample<ExampleClass, int64_t>{1337});
}

static auto example_optionals(kdk::glua::Glua::Ptr glua) -> void
{
    std::cout << std::endl << __FUNCTION__ << " starting..." << std::endl;

    REGISTER_TO_LUA(glua, receive_optional_str);

    std::optional<std::string> opt_str;
    std::optional<int64_t>     opt_int;

    glua->CallScriptFunction("example_optionals", opt_str, opt_int);

    opt_str = "herp";
    opt_int = 1337;
    glua->CallScriptFunction("example_optionals", opt_str, opt_int);
}

auto main(int argc, char* argv[]) -> int
{
    auto glua = kdk::glua::Glua::Create(std::cout);

    ///////////// ADD CUSTOM BINDINDS HERE /////////////
    REGISTER_TO_LUA(glua, example_binding);
    REGISTER_CLASS_TO_LUA(glua, ExampleClass, &ExampleClass::GetValue, &ExampleClass::SetValue, &ExampleClass::Increment);
    REGISTER_CLASS_TO_LUA(glua, BoxedValue, &BoxedValue::GetValue, &BoxedValue::SetValue);

    if (argc != 2)
    {
        std::cout << "Usage: " << argv[0] << " <path_to_example_lua_script>" << std::endl;
    }
    else
    {
        std::string script = kdk::file_util::read_all(argv[1]);

        if (script.empty())
        {
            std::cout << "File [" << argv[1] << "] not found or was invalid" << std::endl;
        }

        glua->RunScript(script);

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
    }

    return 0;
}
