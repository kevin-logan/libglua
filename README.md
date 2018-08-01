# libglua
libglua is a C++ Lua binding library to make using Lua in C++ even easier. It has support for many built-in types, such as numbers, strings, shared_ptrs, and vectors, and also provides easy integration with your own C++ functions and classes.

## Quick libglua
### Calling Lua with libglua
First you must instantiate an instance of Glua, and you can use this instance to call into Lua scripts:
```C++
auto glua = kdk::glua::Glua::Create(std::cout);
```
Notice to instantiate the Glua instance you must provide an std::ostream reference. This is where output from the Lua `print` function will be streamed. In this case we have provided std::cout so Lua output is redirected to the console.

Now you can run scripts with either `Glua::RunFile` or `Glua::RunScript`:
```C++
glua->RunFile("example.lua");
```
or
```C++
auto script = kdk::file_util::read_all("example.lua");
glua->RunScript(script);
```

### Using a C++ function in Lua
In your C++ code you could have a function like this:
```C++
static auto example_binding(std::string_view some_str, double some_double) -> std::string
{
    std::stringstream stream;
    stream << "example_binding called with (" << some_str << ", " << some_double << ")" << std::endl;
    return stream.str();
}
```
This function takes a string_view and a double, and returns a string. To register it to Lua you can use the macro `REGISTER_TO_LUA` like this:
```C++
REGISTER_TO_LUA(glua, example_binding);
```
Or you can do it a bit more verbosely if you don't like macros:
```C++
glua->RegisterCallable("example_binding",glua->CreateLuaCallable(&example_binding));
```
Now the function can be called in lua:
```lua
function example_simple_binding()
    print("example_binding('herp', 1337.1337) = " .. example_binding('herp', 1337.1337))
end
```
This will pass along both the parameters and print the string result!

### Using an overloaded C++ function in Lua
Unfortunately, when a function (or method) is overloaded, we can no longer deduce the parameter types automatically, and the macros `REGISTER_TO_LUA` and `REGISTER_CLASS_TO_LUA` won't be able to bind those functions/methods.

When this occurs, you must resolve the overload for glua, and instead use `Glua::RegisterCallable` or `Glua::RegisterMethod` directly.

Given the following functions:
```C++
static auto overloaded_function(int int_param) -> void {
    std::cout << "received param: " << int_param << std::endl;
}

static auto overloaded_function(std::string_view string_param) -> void {
    std::cout << "received param: " << string_param << std::endl;
}
```
We must register the methods manually, and since lua doesn't allow overloaded functions, you must provide unique names for each overload:
```C++
glua->RegisterCallable("overloaded_function_int", glua->CreateLuaCallable(static_cast<void (*)(int)>(&overloaded_function)));
glua->RegisterCallable("overloaded_function_sv", glua->CreateLuaCallable(static_cast<void (*)(std::string_view)>(&overloaded_function)));
```

### Using a C++ class in Lua
Given some C++ class:
```C++
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
```
We can easily register this class into Lua with `REGISTER_CLASS_TO_LUA`:
```C++
REGISTER_CLASS_TO_LUA(glua, ExampleClass, &ExampleClass::GetValue, &ExampleClass::SetValue, &ExampleClass::Increment);
```
Now notice two important things: We didn't register the `Create` static method, nor did we register the `ExampleOverload`.

For `Create` this was because Glua will actually register a `Create` method or a Default Constructor for you if it finds one, as `CreateX` and `ConstructX` respectively, where X is the class name. In this case there is no Default Constructor, but `CreateExampleClass` is bound to Lua automatically.

As for `ExampleOverload`, we must resolve the overload manually, just like function overloading. NOTE: having a const and a non-const method, like in this example, is indeed overloading, and is a very common case where you may need to manually resolve the overloads.

In this example we wish to expose the non-const overload (which is generally good practice as Lua can't enforce const correctness for us):
```C++
glua->RegisterMethod<ExampleClass>("ExampleOverload", glua->CreateLuaCallable(static_cast<ExampleClass* (ExampleClass::*)()>(&ExampleClass::ExampleOverload)));
```

NOTE: you must register a class with `REGISTER_CLASS_TO_LUA` or `Glua::RegisterClass` before you try and register additional methods

Now your class can be used in Lua:
```lua
function example_managed_cpp_class()
    bound_class_1 = CreateExampleClass(1337)
    bound_class_2 = CreateExampleClass(10101)

    print("bound_class_1:GetValue() = " .. bound_class_1:GetValue())
    print("bound_class_2:GetValue() = " .. bound_class_2:GetValue())

    bound_class_1:Increment(bound_class_2)

    print("post increment value = " .. bound_class_1:GetValue())

    print("bound_class_1:ExampleOverload():GetValue() = " .. bound_class_1:ExampleOverload():GetValue())
end
```

### Calling a specific Lua function from C++
In order to call a Lua function from C++, you must first run the script the function is defined in. This will execute the code in the global scope (if any), but won't execute any functions (unless they're called in the global scope).

Given this Lua script (as example.lua):
```lua
function example_callable_from_cpp(int_arg, string_arg)
    return "callable_from_cpp called with (" .. int_arg .. ", " .. string_arg .. ")", int_arg * 2
end
```

You can call `example_callable_from_cpp` from C++ like so:
```C++
glua->RunFile("example.lua");
glua->CallScriptFunction("example_callable_from_cpp", 1337, "herpaderp");
```
`Glua::CallScriptFunction` takes first the name of the function to call, and then any arguments you wish to pass to that function. The return value, however, is pushed onto the stack and not automatically returned for you (as a lua function could actually returned multiple arguments, which is precisely what is happening in this case).

To get the return values, you must pop them off manually, providing the types you expect them in:

```C++
// expect two return values
auto second_return = glua->Pop<int64_t>();
auto first_return  = glua->Pop<std::string>();

std::cout << "example_callable_from_cpp returned: " << first_return << ", " << second_return << std::endl;
```

Notice the second return value was pushed onto the stack last, so it is retrieved first.

### Reading Lua global values in C++
Another case, common if Lua were used as a configuration language, is for a script to simply provide global values that can be read into C++. Given this Lua script (as example.lua):
```lua
width = 200
height = 300
```

We can easily retrieve the width and height values. As with calling a lua function, we must first run the script:
```C++
glua->RunFile("example.lua");
auto width = glua->GetGlobal<uint64_t>("width");
auto height = glua->GetGlobal<uint64_t>("height");
```

This can be used to retrieve the value of any type supported by Glua, and any custom type that has been registered to Glua.

### Sandboxing in Glua
When creating a Glua instance, it defaults to a sandboxed environment which only allows Lua functions/tables to be used if they cannot pose a risk to the system (e.g. things like file IO are disabled). This does not include preventing infinite loops or consuming all the system's memory, however it provides a good starting point from a safety perspective.

Given the following Lua script (as example.lua):
```lua
file = io.open("some_file.txt", "a")
file:write("another successful write\n")
```

Given how The following C++ code will fail to execute the script:
```C++
auto glua = kdk::glua::Glua::Create(std::cout);
glua->RunFile("example.lua");
```

This is because of the default sandboxing. In order to disable the sandboxing, the Glua instance can be created with sandboxing disabled, or `Glua::ResetEnvironment` with `false` provided for the `sandboxed` argument:
```C++
// this works:
auto glua = kdk::glua::Glua::Create(std::cout, false);
glua->RunFile("example.lua");

// and this also works:
auto glua = kdk::glua::Glua::Create(std::cout);
glua->ResetEnvironment(false);
glua->RunFile("example.lua");
```

Of course, if you don't need sandboxing you should simply instantiate your Glua instance without sandboxing, as recreating the environment is an unnecessary cost. `Glua::ResetEnvironment` is intended to be used to reset the environment between script calls.

### Running multiple scripts
In Glua it's simple to run one script after another, you can simply call `Glua::RunFile` or `Glua::RunScript` one after another, if you like.

However there is a gotcha, consider this case where you have two lua scripts:

example_one.lua
```lua
if some_variable == nil then
    some_variable = "some value"
end
```
example_two.lua
```lua
if some_variable == nil then
    some_variable = "some other value"
else
    print("weird unexpected scenario!")
end
```

If you call these scripts in order, like so:
```C++
glua->RunFile("example_one.lua");
glua->RunFile("example_two.lua");
```

Then you'll receive that "weird unexpected scenario!" message, as the first script has polluted the environment of the second script.

This may be desireable behavior, however if it is not you can easily fix this problem by resetting the Lua environment between calls:
```C++
glua->RunFile("example_one.lua");
glua->ResetEnvironment();
glua->RunFile("example_two.lua");
```

NOTE: `Glua::ResetEnvironment` actually takes one argument, `sandboxed` which defaults to true. Remember to call it as `glua->ResetEnvironment(false)` if you wish to run trusted Lua code without a sandbox.

### Additional Examples
Many of these examples and more can be found in the repository. `src/examples/examples.cpp` is a somewhat all-inclusive example which includes many of the above examples and a few more complicated scenarios. It expects to run the script `example.lua` found at the root of the repository.

When making, the examples are compiled and the binary `libglua-examples` is put into the root directory. It expects one argument, a path the the `example.lua` script, e.g. `./libglua-examples example.lua`