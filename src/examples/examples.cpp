#include "FileUtil.h"
#include "Glua.h"

#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>

static auto example_binding(std::string_view some_str, double some_double) -> std::string
{
    std::stringstream stream;
    stream << "example_binding called with (" << some_str << ", " << some_double << ")" << std::endl;
    return stream.str();
}

class ExampleClass : public std::enable_shared_from_this<ExampleClass>
{
public:
    static auto Create(int64_t value = 0) -> std::shared_ptr<ExampleClass>
    {
        return std::shared_ptr<ExampleClass>{new ExampleClass{value}};
    }

    auto SetValue(int64_t value) -> void { m_value = value; }
    auto GetValue() const -> int64_t { return m_value; }

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

    static uint64_t g_count;
};

uint64_t BoxedValue::g_count = 0ul;

auto some_func(BoxedValue& derp) -> void
{
    std::cout << __PRETTY_FUNCTION__ << " called with " << derp.GetValue() << std::endl;
}

auto example_sandboxed_environment(kdk::glua::Glua::Ptr glua, const std::string& script) -> void
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

auto example_library_functions(kdk::glua::Glua::Ptr glua) -> void
{
    std::cout << std::endl << __FUNCTION__ << " starting..." << std::endl;

    glua->CallScriptFunction("example_library_functions");
}

auto example_managed_cpp_class(kdk::glua::Glua::Ptr glua) -> void
{
    std::cout << std::endl << __FUNCTION__ << " starting..." << std::endl;

    glua->CallScriptFunction("example_managed_cpp_class");
}

auto example_simple_binding(kdk::glua::Glua::Ptr glua) -> void
{
    std::cout << std::endl << __FUNCTION__ << " starting..." << std::endl;

    glua->CallScriptFunction("example_simple_binding");
}

auto example_global_value(kdk::glua::Glua::Ptr glua) -> void
{
    std::cout << std::endl << __FUNCTION__ << " starting..." << std::endl;

    glua->SetGlobal("global_boxed_value", BoxedValue{1337});
    glua->CallScriptFunction("example_global_value");
}

auto example_inferred_storage_cpp_class(kdk::glua::Glua::Ptr glua) -> void
{
    std::cout << std::endl << __FUNCTION__ << " starting..." << std::endl;

    glua->CallScriptFunction("example_inferred_storage_cpp_class");
}

auto example_callable_from_cpp(kdk::glua::Glua::Ptr glua) -> void
{
    std::cout << std::endl << __FUNCTION__ << " starting..." << std::endl;

    glua->CallScriptFunction("example_callable_from_cpp", 1337, "herpaderp");

    // expect two return values
    auto second_return = glua->Pop<int64_t>();
    auto first_return  = glua->Pop<std::string>();

    std::cout << "example_callable_from_cpp returned: " << first_return << ", " << second_return << std::endl;
}

auto example_reverse_array(kdk::glua::Glua::Ptr glua) -> void
{
    std::cout << std::endl << __FUNCTION__ << " starting..." << std::endl;

    std::vector<BoxedValue> v{BoxedValue{1}, BoxedValue{2}, BoxedValue{3}, BoxedValue{4}, BoxedValue{5}};
    glua->CallScriptFunction("example_reverse_array", v);

    auto reversed_array = glua->PopArray<BoxedValue>();

    std::cout << "reverse_array returned:";
    for (auto& val : reversed_array)
    {
        std::cout << " " << val.GetValue();
    }
    std::cout << std::endl;
}

auto example_mutate_value(kdk::glua::Glua::Ptr glua) -> void
{
    std::cout << std::endl << __FUNCTION__ << " starting..." << std::endl;

    BoxedValue our_value{5};

    glua->CallScriptFunction("example_mutate_value", our_value);
    std::cout << "mutate_boxed_value called without reference, value: " << our_value.GetValue() << std::endl;

    glua->CallScriptFunction("example_mutate_value", std::ref(our_value));
    std::cout << "mutate_boxed_value called with reference, value: " << our_value.GetValue() << std::endl;
}

auto example_create_and_retrieve_global(kdk::glua::Glua::Ptr glua) -> void
{
    std::cout << std::endl << __FUNCTION__ << " starting..." << std::endl;

    glua->CallScriptFunction("example_create_and_retrieve_global");

    std::cout << "glua global example_lua_global: " << glua->GetGlobal<std::string>("example_lua_global") << std::endl;
}

auto main(int argc, char* argv[]) -> int
{
    auto glua = kdk::glua::Glua::Create(std::cout);

    ///////////// ADD CUSTOM BINDINDS HERE /////////////
    REGISTER_TO_LUA(glua, example_binding);
    REGISTER_TO_LUA(glua, some_func);
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
    }

    return 0;
}
