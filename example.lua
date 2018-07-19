bound_class_1 = CreateExampleClass(1337)
bound_class_2 = CreateExampleClass(10101)

print("bound_class_1:GetValue() = " .. bound_class_1:GetValue())
print("bound_class_2:GetValue() = " .. bound_class_2:GetValue())

bound_class_1:Increment(bound_class_2)

print("post increment value = " .. bound_class_1:GetValue())

print("example_binding('herp', 1337.1337) = " .. example_binding('herp', 1337.1337))

function callable_from_cpp(int_arg, string_arg)
    return "callable_from_cpp called with (" .. int_arg .. ", " .. string_arg .. ")", int_arg * 2
end