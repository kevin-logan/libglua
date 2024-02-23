advanced_demonstration = true;

function get_weights()
    local s = sentinel()
    s:foo("original data that's kinda long")
    return {
        first=0.5,
        second=13.37,
        blurgh=123.456,
        special=s
    }
end

function print_weights(weights)
    print('Lua received weights:\n')
    for k,v in pairs(weights) do
        if k == 'special' then
            print(string.format('received special! special.bar() = %s\n', weights[k]:bar()))
        else
            print(string.format('\t%s: %s\n', tostring(k), tostring(weights[k])))
        end
    end
end

function print_array(a)
    print('Lua received array:\n')
    for k,v in ipairs(a) do
        print(string.format('\t%s: %s\n', tostring(k), tostring(v)))
    end
end

function get_array() 
    return {1, 2, 3}
end

return 'Lua advanced demonstration complete'