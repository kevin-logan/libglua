basic_demonstration = true

function concat(...)
    local result = ""
    local args = {...}
    for i,v in ipairs(args) do
        result = result .. tostring(v)
    end

    return result
end

local status, retval = pcall(function ()
    print(string.format('script called with magic: %s\n', tostring(magic)))

    panic = true

    local s1 = sentinel()
    local s2 = s1

    s1:foo('foo called from s1')
    s2:foo('foo called from s2')

    -- direct access to public property works
    print(string.format('calling s1:bar(): [%s]\n', s1.value_))

    -- demonstrates property can be reassigned
    s1.value_ = string.format('panic is currently: %s', panic)

    -- demonstrates s1 is s2 (not a copy) and accessor on C++ side see property value change
    print(string.format('calling s2:bar(): [%s]\n', s2:bar()))

    local parent = sentinel()
    parent:foo('parent')

    print(string.format('parent is instance %d\n', parent.instance_));

    local instance_status, instance_result = pcall(function()
        parent.instance_ = 5;
        print('[illegal] parent.instance_ = 5 succeeded!\n')
    end)
    if not instance_status then
        print(string.format('[illegal] parent.instance_ = 5 had exception: %s\n', instance_result))
    end

    local child1 = parent:create_child()
    child1:foo('child1')

    local const_child = parent:get_child_const()
    print(string.format('[legal] const_child:bar(): %s\n', const_child:bar()))
    local const_status, const_err = pcall(function ()
        local const_child_child = const_child:create_child()
        print('[illegal] const_child:create_child() succeeded!\n')
    end)
    if not const_status then
        print(string.format('[illegal] const_child:create_child() had exception: %s\n', const_err))
    end

    -- this is the same as the result from create_child, but on the C++ side it's different,
    -- demonstrating that C++ functions returning pointers or references both work from lua
    local child2 = parent:get_child()
    child2:foo('child2')

    print(string.format('parent has value [%s], child1 [%s], child2 [%s] \n', parent.value_, child1.value_, child2.value_))

    return string.format('hello world, add(\'21\', 2) = %d', add('21', 2))
end)
if not status then
    return string.format('Execution failed: %s', retval)
end

return retval