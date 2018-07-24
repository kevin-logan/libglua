bound_class_1 = CreateExampleClass(1337)
bound_class_2 = CreateExampleClass(10101)

print("bound_class_1:GetValue() = " .. bound_class_1:GetValue())
print("bound_class_2:GetValue() = " .. bound_class_2:GetValue())

bound_class_1:Increment(bound_class_2)

print("post increment value = " .. bound_class_1:GetValue())

print("example_binding('herp', 1337.1337) = " .. example_binding('herp', 1337.1337))

print("global_boxed_value:GetValue() = " .. global_boxed_value:GetValue())

lua_constructed_boxed_value = ConstructBoxedValue()
lua_constructed_boxed_value:SetValue(9999)
print("lua_constructed_boxed_value:GetValue() = " .. lua_constructed_boxed_value:GetValue())

function callable_from_cpp(int_arg, string_arg)
    return "callable_from_cpp called with (" .. int_arg .. ", " .. string_arg .. ")", int_arg * 2
end

function reverse_array(arr)
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

function mutate_boxed_value(boxed_value)
    boxed_value:SetValue(boxed_value:GetValue() * 1000)
end