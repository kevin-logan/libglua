# libglua
libglua is a C++ binding library to make using scripting languages in C++ even easier. It has support for many built-in types, such as numbers, strings, and pointers, while safely respecting object lifetimes and const correctness. It provides easy facilities to integrate with your own C++ functions and classes, and to integrate those C++ functions with script functions.

Currently libglua has backends for JavaScript (via SpiderMonkey) and Lua (via luajit), but the implementations are entirely decoupled from one another, and adding additional backends for other scripting languages is possible.

SpiderMonkey is used for JavaScript instead of V8 simply because V8 does not make any object lifetime guarantees, and though the original implementation was in V8 there was no way to avoid memory leaks, as the V8 garbage collector doesn't necessarily ever dispose of JavaScript objects (and call their corresponding C++ hooks to release C++ resources). This is an intentional design decision in V8 to favor performance, as V8 instances generally run in sandbox processes which can simply be terminated when no longer needed. Libglua, however, supports disposal, re-use, and cleanup within a process without memory or lifetime issues.

## Quick libglua

### Exceptions/Expected
libglua does not raise exceptions, and instead uses `std::expected` for any fallible calls. This is aliased as `glua::result<T>`, and the error type is always an `std::string` with an error message.

NOTE: clang (which must be used on Windows) does not, as of this writing, support std::expected, so libglua provides a barebones implementation that will be replaced with `std::expected` as soon as clang supports it. This barebones implementation is only used when compiling with clang.

### Creating a libglua instance
Include the backend you wish to use, e.g.:
```C++
#include <glua/backends/lua.hpp>
```
And simply call `create` on a `glua::instance` templated on that backend's type, e.g.:
```C++
glua::instance<glua::lua::backend>::create().and_then([&](auto glua_instance) { /* run your code using the instance here */ });
```
This `glua::instance` then gives you access to everything you'll need to run scripts, call functions, etc.

### Calling a script
With your `glua::instance` simply call `execute_script` with a string containing the script. `execute_script` is a template where you can provide the anticipated return type of the script, which will be converted and returned to you as a `result`:
```C++
auto script_result = glua_instance.template execute_script<std::string>(script_string);
if (script_result.has_value())
    std::cout << "script returned: " << script_result.value() << std::endl;
else
    std::cout << "script had error: " << script_result.error() << std::endl;
```

### Calling a script function from C++
Much like executing a script, your `glua::instance` can use `call_function` to call a function in the already-executed script. Executing a script which defines the function is required before the function can be called. Also like `execute_script`, `call_function` accepts a template parameter indicating the expected return type (in C++ types) of the function, which will be returned as a `result` object:
```C++
return glua_instance
    .template call_function<std::string>(
        "concat", 1, 2, true, false, std::string{"Hello, World!"})
    .transform([&](auto concat_ret) { format_print("script concat returned {}\n", concat_ret); });
```
This example calls a function called "concat" in the script, passing it several parameters of different types. It then transforms the successful `result<std::string>` into a `result<void>` and prints out the script result. "concat" is defined in the example scripts and are written to take an arbitrary number of parameters, and this demonstrates this functions properly, as you can add or remove as many arguments from the `call_function` call as you'd like.

### Registering a C++ functor to glua
Registering a functor to glua is simple, but requires providing a name for the function to be used in the script, and the functor to call. This functor can be anything a functor can be, including callable objects (like lambdas) and function pointers. glua will automatically attempt to convert script values to the correct C++ type and similarly automatically convert the C++ functions return value to an appropriate object for the script.
```C++
glua_instance.register_functor("foo", [&](std::string param) { std::cout << "foo called with paramter: " << param << std::endl; }).and_then([&]() {
    // "foo" successfully registered, we can now call scripts or script functions which will be able to call foo:
    auto result = glua_instance.template execute_script<void>(some_script_that_calls_foo);
});
```

NOTE: overloads are not supported on the script side, so if you're trying to register a function that is overloaded on the C++ side you must use `glua::resolve_overload` to tell glua which overload should be registered to the script.

### Registering a C++ class/struct to glua
Registering a class to glua allows that class to be used as parameters and return types of C++ functions bound to glua. The script may use returned objects as normal, and eventually pass them back to other C++ functions that accept that type as a parameter. glua automatically follows these semantics:
- a reference to an object of a registered class is always considered owned by C++, and if the C++ object is destroyed while a script is still using it it becomes a dangling reference
- pointers are similarly considered owned by C++
- return an `std::unique_ptr` to glua of a registered class will transfer ownership to the script, and when the script has finished using the object it will be destroyed regardless of what happens on the C++ end. Unfortunately this doesn't work in reverse, and you cannot call functions that expect a `std::unique_ptr` as a parameter, as there's no way to ensure the script has ownership, nor are there move semantics that can prevent reuse of the same variable in the script.

To register a class to glua one must simply specialize `glua::class_registration` for the type they wish to register. The specialization must define a `name`, `constructor`, `methods`, and `fields` that should be exposed to glua. The `name` is simply a string name for the type, which will also be registered as a function which will call `constructor` (which must return an `std::unique_ptr` to give ownership to the script). `methods` and `fields` are both tuples of the registered methods and fields, which can easily be created using provided macros (and macros can be avoided if preferred, it's just a bit more verbose):
```C++
template<>
struct glua::class_registration<sentinel>
{
    static inline const std::string name{"sentinel"};

    // TODO: change implementations to make `constructor` optional
    static inline auto constructor = glua::create_generic_functor([]() { return std::make_unique<sentinel>(); });
    static inline auto methods     = std::make_tuple(
        GLUABIND(sentinel::foo),
        GLUABIND(sentinel::bar),
        GLUABIND(sentinel::has_child),
        GLUABIND_AS(sentinel::get_child, sentinel& (sentinel::*)()),
        glua::bind<const sentinel& (sentinel::*)() const>("get_child_const", &sentinel::get_child),
        GLUABIND(sentinel::create_child));
    static inline auto fields = std::make_tuple(GLUABIND(sentinel::value_), GLUABIND(sentinel::instance_));
};
```
From the examples, this `sentinel` class attempts to demonstrate most common use cases, including a few gotchas:
- Methods are often overloaded with a const and non-const overload. As mentioned before overloads aren't supported script-side, so this demonstrates how to register both overloads of `sentinel::get_child`.
- The `constructor` is sort of a misnomer, and it instead must be a generic functor that returns an `std::unique_ptr` as that is how ownership is transferred to the script. It can take paramters just like any other function registered to glua and doesn't need to use the default constructor like this example.

If using the `GLUABIND` macro is undesireable, you can simply call `glua::bind` directly, `glua::bind("foo", sentinel::foo)` is the same as `GLUABIND(sentinel::foo)`. The macro simply allows the user to avoid repeating function/method names.

### Setting a global for the script from a value in C++
A global variable can be set using the value of any C++ object that is supported (or has been registered):
```C++
auto result = glua_instance.set_global("magic", 13.37).and_then([&]() {
    glua_instance.template register_class<sentinel>();

    return glua_instance.template execute_script<std::string>(some_script_that_uses_magic);
});
```

### Extracting a global in the script into a value in C++
Extracting a value is as simple as calling `glua::instance::get_global`, which is a template that requires the caller give the type the global should be converted to for C++:
```C++
auto foo_value = glua_instance.template get_global<std::string>("foo");
if (foo_value.has_value())
    std::cout << "foo has value: " << foo_value.value() << std::endl;
else
    std::cout << "error retrieving the value of foo: " << foo_value.error() << std::endl;
```

### Additional Examples
Many of these examples and more can be found in the repository. `src/examples/examples.cpp` is a somewhat all-inclusive example which includes many of the above examples and a few more complicated scenarios. It expects to run the one of the provided scripts `basic_test.lua` or `basic_test.js` found at the root of the repository.

When making, the examples are compiled and the binary `libglua-examples` is put into the root directory. It expects one argument, a path the the `example.lua` script, e.g. `./libglua-examples basic_test.lua`

# Running the Examples
Once liglua-examples is built, you can run it but you need to provide it a script file to execute. It expects a script with some particular functions/variables and two scripts are provided the fulfill these requirements:
- basic_test.js - A JavaScript script which can be used with the example executable
- basic_test.lua - A Lua script which can be used with the example executable

# Linux Development Environment
Building and running glua on Linux is generally very simple, as luajit and SpiderMonkey are often provided within the package manager of most distributions. Besides luajit and SpiderMonkey, glua doesn't have any other dependencies besides a modern C++23 compatible compiler such as clang or gcc.

## Building luajit
luajit uses CMake to build, but a basic Makefile is provided to demonstrate which configuration must be set and how to build glua via CMake.

# Windows Development Environment
Getting everything set up properly on linux is a breeze, but it can be difficult on Windows.

First things first, you need to install the Visual Studio Build Tools:
- Go to https://visualstudio.microsoft.com/downloads/?q=build+tools and scrol down to download "Build Tools for Visual Studio"
- Run the installer, select "Desktop development with C++", make sure C++ CMake tools for Windows is selected
- Under "Individual components" search for clang, ensure the compiler and clang-cl toolset are both selected
- Now run the installation

The project has two major dependencies: luajit and SpiderMonkey, but this presents a problem: luajit on Windows is designed to be built via MSVC, while MSVC is no longer supported for SpiderMonkey, and we must use clang. SpiderMonkey's headers do not work with MSVC, so this also forces us to use clang as the compiler for any project using libglua on Windows.

## Building luajit
Ensure the submodule for luajit is checked out at .\dependencies\luajit. If building for x64, ensure the following commands are run in a 64 bit command prompt, it _will not work_ on an x86 terminal and will force an x86 build of luajit:
```
cd .\dependencies\luajit\src
.\msvcbuild.bat static
```
This will place the static library at .\dependencies\luajit\src\lua51.lib, which is where the default Windows build helper expects it.
The sources will still be in .\dependencies\luajit\src, which is also where the default Windows build helper expects it.

## Building SpiderMonkey
First, get started with the normal documentation for building Firefox on Windows: https://firefox-source-docs.mozilla.org/setup/windows_build.html#building-firefox-on-windows
- Follow this up to (but not including) the actual build step itself. The prior steps will set up MozillaBuild and acquire any other dependencies needed.
- In a different directory, download and extract the correct version of the Firefox code, 115.0.3esr: https://ftp.mozilla.org/pub/firefox/releases/115.0.3esr/source/
- In this newly-downloaded source's directory (but within the MozillaBuild terminal) follow the instructions to build SpiderMonkey: https://firefox-source-docs.mozilla.org/js/build.html
- This will build SpiderMonkey but the files you need will actually be in several places:
- We need mozjs-115.lib, mozjs-115.dll, mozglue.lib, and mozglue.dll, copy these files into your project directory (or update the CMake configuration to point at them)
- We also need the source, which will be in this directory "firefox-115.0.3\obj-opt-x86_64-pc-windows-msvc\dist\include", update the CMake configuration to make sure SPIDERMONKEY_INCLUDE_PATH points to these sources

## Building libglua examples
At this point you should be able to build libglua and its examples. Windows Batch files are provided to make it easy, or to at a minimum demonstrate what CMake settings must be set and how to build. The only thing you'll likely need to set if you've followed the above instructions will be the SPIDERMONKEY_INCLUDE_PATH values set in these batch files:
- build_debug.bat (makes a debug build, copies the examples binary into the root directory)
- build_release.bat (makes a release build, copies the examples binary into the root directory)
- build_clean.bat (cleans up any artifacts from the above batch files including all build artifacts)