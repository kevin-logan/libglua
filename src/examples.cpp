#include <format>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include <glua/backends/lua.hpp>
#include <glua/backends/spidermonkey.hpp>

template <typename... Args>
void format_print(std::format_string<Args...> fmt, Args&&... args)
{
    std::cout << std::format(std::move(fmt), std::forward<Args>(args)...);
}

// The C++ function we want to expose to JavaScript
int cpp_add(int a, int b)
{
    return a + b;
}
double cpp_add(double a, double b)
{
    return a + b;
}

struct sentinel {
    sentinel() { format_print("sentinel::sentinel() with ptr {}\n", static_cast<void*>(this)); }
    ~sentinel() { format_print("sentinel::~sentinel() with ptr {} and value_ {}\n", static_cast<void*>(this), value_); }

    sentinel(const sentinel&) = delete;
    sentinel(sentinel&&) = delete;

    sentinel& operator=(const sentinel&) = delete;
    sentinel& operator=(sentinel&&) = delete;

    void foo(std::string value) { value_ += value; }
    const std::string& bar() const { return value_; }

    bool has_child() const { return child_ != nullptr; }
    sentinel& get_child() { return *child_; }
    const sentinel& get_child() const { return *child_; }
    sentinel* create_child()
    {
        child_ = std::make_unique<sentinel>();
        return child_.get();
    }

    std::string value_;
    std::unique_ptr<sentinel> child_;

    static inline std::size_t instance_counter { 0 };
    const std::size_t instance_ { ++instance_counter };
};

template <>
struct glua::class_registration<sentinel> {
    static inline const std::string name { "sentinel" };

    // TODO: change implementations to make `constructor` optional
    static inline auto constructor = glua::create_generic_functor([]() { return std::make_unique<sentinel>(); });
    static inline auto methods = std::make_tuple(
        GLUABIND(sentinel::foo),
        GLUABIND(sentinel::bar),
        GLUABIND(sentinel::has_child),
        GLUABIND_AS(sentinel::get_child, sentinel& (sentinel::*)()),
        glua::bind<const sentinel& (sentinel::*)() const>("get_child_const", &sentinel::get_child),
        GLUABIND(sentinel::create_child));
    static inline auto fields = std::make_tuple(GLUABIND(sentinel::value_), GLUABIND(sentinel::instance_));
};

template <typename Backend>
glua::result<void> basic_test_glua_instance(glua::instance<Backend>& glue)
{
    auto panic_value = glue.template get_global<bool>("panic");

    if (panic_value.has_value()) {
        if (panic_value.value()) {
            format_print("PANIC OH DANG!\n");
        } else {
            format_print("explicitly told not to panic, nothing to see here\n");
        }
    }

    return glue
        .template call_function<std::string>(
            "concat", 1, 2, true, false, std::string { "Hello, World!" })
        .transform([&](auto concat_ret) { format_print("script concat returned {}\n", concat_ret); });
}

template <typename Backend>
glua::result<void> advanced_test_glua_instance(glua::instance<Backend>& glue)
{
    return glue.template call_function<std::vector<int>>("get_array")
        .and_then([&](auto val) {
            format_print("Received vector of size {}\n", val.size());

            for (auto& v : val) {
                format_print("\t{}\n", v);
            }

            return glue.template call_function<std::unordered_map<std::string, glua::any>>("get_weights");
        })
        .and_then([&](auto weights) {
            format_print("Received map of size {}\n", weights.size());
            for (auto& [k, v] : weights) {
                format_print("\t{}: {}\n", k, "<any>");
            }

            return glue.template call_function<void>("print_weights", weights).and_then([&]() {
                // demonstrate using an any more than once
                return glue.template call_function<void>("print_weights", weights);
            });
        })
        .and_then([&]() {
            std::vector<std::string> a {
                "Hello", "World", "Derp", "Flerp"
            };
            return glue.template call_function<void>("print_array", a);
        })
        .and_then([&]() {
            // let's call get_weights again, to create another sentinel, but we'll store it in an any and ignore the any
            // despite never using the any the sentinel will destruct when the any destructs, rather than via JS ownership
            return glue.template call_function<glua::any>("get_weights").transform([&](auto our_any) {
                // ignore our_any
                format_print("Ignoring the resulting any {} received from get_weights\n", static_cast<void*>(our_any.impl_.get()));
            });
        });
}

template <typename Backend>
glua::result<void> test_glua_instance(std::string input, glua::instance<Backend>& glue)
{
    return glue.register_functor("add", glua::resolve_overload<int, int>(&cpp_add))
        .and_then([&]() { return glue.register_functor("print", [](std::string param) { format_print("{}", param); }); })
        .and_then([&]() {
            return glue.set_global("magic", 13.37);
        })
        .and_then([&]() {
            glue.template register_class<sentinel>();

            return glue.template execute_script<std::string>(input).and_then([&](auto script_result) {
                format_print("Script completed with result: {}\n", script_result);
                // check which style of script
                if (auto advanced_val = glue.template get_global<bool>("advanced_demonstration"); advanced_val.has_value() && advanced_val.value()) {
                    return advanced_test_glua_instance(glue);
                } else if (auto basic_val = glue.template get_global<bool>("basic_demonstration"); basic_val.has_value() && basic_val.value()) {
                    return basic_test_glua_instance(glue);
                } else {
                    format_print("Finished executing unrecognized script\n");
                    return glua::result<void> {};
                }
            });
        });
}

enum class script_type {
    javascript,
    lua
};

script_type type_for_filename(std::string_view filename)
{
    if (filename.ends_with(".lua"))
        return script_type::lua;
    else
        return script_type::javascript;
}

script_type type_for_code(std::string_view code)
{
    format_print("WARNING: guessing language based on code!\n");
    if (code.contains("local ") || code.contains("string.format") || code.contains("pcall"))
        return script_type::lua;
    else
        return script_type::javascript;
}

int main(int argc, char* argv[])
{
    auto [input, type] = [&]() {
        if (argc == 2) {
            std::string filename { argv[1] };
            std::ifstream input_file { filename, input_file.binary | input_file.ate | input_file.in };
            auto len = input_file.tellg();
            input_file.seekg(0, std::ios_base::beg);

            std::string result;
            result.resize(len);

            input_file.read(result.data(), len);

            return std::make_pair(std::move(result), type_for_filename(filename));
        } else {
            format_print("No input file provided, reading stdin [use <<end>> or EOF to end]\n");
            std::string input;
            std::string line;
            while (std::getline(std::cin, line)) {
                if (line == "<<end>>")
                    break;
                input += line;
                input += '\n';
            }

            auto type_guess = type_for_code(input);
            return std::make_pair(std::move(input), type_guess);
        }
    }();

    auto total_result = [&]() -> glua::result<void> {
        switch (type) {
        case script_type::lua:
            return glua::instance<glua::lua::backend>::create().and_then(
                [&](auto glue) { return test_glua_instance(std::move(input), glue); });
        case script_type::javascript:
            return glua::instance<glua::spidermonkey::backend>::create().and_then(
                [&](auto glue) { return test_glua_instance(std::move(input), glue); });
        }
        return glua::unexpected("no valid script type was determined");
    }();

    if (!total_result.has_value()) {
        format_print("Error: {}\n", total_result.error());
    }

    return 0;
}
