#include "file_util.h"
#include "luacallable.h"

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
    BoxedValue() : m_value(0)
    {
        std::cout << "DEFAULT CONSTRUCTED ANOTHER BOXED VALUE #" << ++g_count << " (" << m_value << ")" << std::endl;
    }
    BoxedValue(int64_t value) : m_value(value)
    {
        std::cout << "CONSTRUCTED ANOTHER BOXED VALUE #" << ++g_count << " (" << m_value << ")" << std::endl;
    }

    BoxedValue(BoxedValue&& move) : m_value(move.m_value)
    {
        std::cout << "MOVE CONSTRUCTED ANOTHER BOXED VALUE #" << ++g_count << " (" << m_value << ")" << std::endl;
    }

    BoxedValue(const BoxedValue& copy) : m_value(copy.m_value)
    {
        std::cout << "COPY CONSTRUCTED ANOTHER BOXED VALUE #" << ++g_count << " (" << m_value << ")" << std::endl;
    }

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

auto main(int argc, char* argv[]) -> int
{
    auto lua = kdk::lua::Lua::Create();

    ///////////// ADD CUSTOM BINDINDS HERE /////////////
    REGISTER_TO_LUA(lua, example_binding);
    REGISTER_TO_LUA(lua, some_func);
    REGISTER_CLASS_TO_LUA(lua, ExampleClass, &ExampleClass::GetValue, &ExampleClass::SetValue, &ExampleClass::Increment);
    REGISTER_CLASS_TO_LUA(lua, BoxedValue, &BoxedValue::GetValue, &BoxedValue::SetValue);

    if (argc != 2)
    {
        std::cout << "Usage: " << argv[0] << " <lua_script_filename>" << std::endl;
    }
    else
    {
        std::string script = kdk::file_util::read_all(argv[1]);

        if (script.empty())
        {
            std::cout << "File [" << argv[1] << "] not found or was invalid" << std::endl;
        }

        std::vector<BoxedValue> derp{BoxedValue{1}, BoxedValue{2}, BoxedValue{3}, BoxedValue{4}, BoxedValue{5}};

        lua->SetGlobal("global_boxed_value", BoxedValue{1337});

        lua->RunScript(script);
        lua->CallScriptFunction("callable_from_cpp", 1337, "herpaderp");
        // expect two return values
        auto second_return = lua->Pop<int64_t>();
        auto first_return  = lua->Pop<std::string>();

        lua->CallScriptFunction("reverse_array", derp);
        auto reversed_array = lua->PopArray<BoxedValue>();

        BoxedValue our_value{5};

        lua->CallScriptFunction("mutate_boxed_value", our_value);
        std::cout << "mutate_boxed_value called without reference, value: " << our_value.GetValue() << std::endl;

        lua->CallScriptFunction("mutate_boxed_value", std::ref(our_value));
        std::cout << "mutate_boxed_value called with reference, value: " << our_value.GetValue() << std::endl;

        std::cout << "script output:" << std::endl << lua->GetNewOutput() << std::endl;
        std::cout << "callable_from_cpp returned: " << first_return << ", " << second_return << std::endl;

        std::cout << "reverse_array returned:";
        for (auto& val : reversed_array)
        {
            std::cout << " " << val.GetValue();
        }
        std::cout << std::endl;

        std::cout << "lua global bound_class_1: "
                  << lua->GetGlobal<std::shared_ptr<ExampleClass>>("bound_class_1")->GetValue() << std::endl;
    }

    return 0;
}
