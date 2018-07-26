function example_sandboxed_environment()
    function pretty_print_table(t, depth)
        local depth_str = string.rep('\t', depth)
        for key, value in pairs(t) do
            if key ~= "_G" then -- _G has a reference to itself, don't recurse forever
                if type(value) == "table" then
                    print(depth_str .. key .. " {")
                    pretty_print_table(value, depth + 1)
                    print(depth_str .. "}")
                else
                    print(depth_str .. key .. ": " .. tostring(value))
                end
            end
        end
    end

    if been_here_before == nil then
        print("fresh sandboxed environment: ")
        been_here_before = true
    else
        print("re-used sandboxed environment: ")
    end

    pretty_print_table(_G, 1)
end

function example_library_functions()
    print("os.time() = " .. os.time())
end

function example_managed_cpp_class()
    bound_class_1 = CreateExampleClass(1337)
    bound_class_2 = CreateExampleClass(10101)

    print("bound_class_1:GetValue() = " .. bound_class_1:GetValue())
    print("bound_class_2:GetValue() = " .. bound_class_2:GetValue())

    bound_class_1:Increment(bound_class_2)

    print("post increment value = " .. bound_class_1:GetValue())
end

function example_simple_binding()
    print("example_binding('herp', 1337.1337) = " .. example_binding('herp', 1337.1337))
end

function example_global_value()
    print("global_boxed_value:GetValue() = " .. global_boxed_value:GetValue())
end

function example_inferred_storage_cpp_class()
    lua_constructed_boxed_value = ConstructBoxedValue()
    lua_constructed_boxed_value:SetValue(9999)
    print("lua_constructed_boxed_value:GetValue() = " .. lua_constructed_boxed_value:GetValue())
end

function example_callable_from_cpp(int_arg, string_arg)
    return "callable_from_cpp called with (" .. int_arg .. ", " .. string_arg .. ")", int_arg * 2
end

function example_reverse_array(arr)
    local reversed = {}
    local i = #arr
    local j = 1

    while i >= 1 do
        reversed[j] = arr[i]
        j = j + 1
        i = i - 1
    end

    return reversed;
end

function example_mutate_value(boxed_value)
    boxed_value:SetValue(boxed_value:GetValue() * 1000)
end

function example_create_and_retrieve_global()
    example_lua_global = "this global was created in lua"
end

function example_enumeration(enum_value)
    print("Received enum value: " .. enum_value)

    return 1
end

function example_custom_template_binding(custom_type)
    print("Received custom handled object with value: " .. custom_type)
end

function  example_optionals(opt_str, opt_int)
    print("example_optionals received: " .. tostring(opt_str) .. ", " .. tostring(opt_int))

    receive_optional_str(opt_str)
end