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

auto main(int argc, char* argv[]) -> int
{
    auto lua = kdk::lua::Lua::Create();

    ///////////// ADD CUSTOM BINDINDS HERE /////////////
    REGISTER_TO_LUA(lua, example_binding);
    REGISTER_CLASS_TO_LUA(lua, ExampleClass, &ExampleClass::GetValue, &ExampleClass::SetValue, &ExampleClass::Increment);

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

        lua->RunScript(script);
        lua->CallScriptFunction("callable_from_cpp", 1337, "herpaderp");
        // expect two return values
        auto second_return = lua->Pop<int64_t>();
        auto first_return  = lua->Pop<std::string>();

        std::cout << "script output:" << std::endl << lua->GetNewOutput() << std::endl;
        std::cout << "callable_from_cpp returned: " << first_return << ", " << second_return << std::endl;
    }

    return 0;
}
