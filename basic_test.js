function concat(...args) { return args.join(''); }

// use var makes it visible after script exit, let/const will not be
try {
    print(`script called with magic: ${magic}\n`);

    var panic = true;

    var s1 = new sentinel();
    var s2 = s1;

    s1.foo('foo called from s1');
    s2.foo('foo called from s2');

    // direct access to public property works
    print(`calling s1.bar(): [${s1.value_}]\n`);

    // demonstrates property can be reassigned
    s1.value_ = `panic is currently: ${panic}`

    // demonstrates s1 is s2 (not a copy) and accessor on C++ side see property value change
    print(`calling s2.bar(): [${s2.bar()}]\n`);

    var parent = new sentinel();
    parent.foo('parent');

    print(`parent is instance ${parent.instance_}\n`);
    try {
        parent.instance_ = 5;
        print(`[illegal] parent.instance_ = 5 succeeded!\n`);
    } catch (e) {
        print(`[illegal] parent.instance_ = 5 had exception ${e}\n`);
    }

    var child1 = parent.create_child();
    child1.foo('child1');

    var const_child = parent.get_child_const();
    print(`[legal] const_child.bar(): ${const_child.bar()}\n`);
    try {
        var const_child_child = const_child.create_child();
        print(`[illegal] const_child.create_child() succeeded!\n`);
    } catch (e) {
        print(`[illegal] const_child.create_child() had exception: ${e}\n`);
    }

    // this is the same as the result from create_child, but on the C++ side it's different,
    // demonstrating that C++ functions returning pointers or references both work from JS
    var child2 = parent.get_child();
    child2.foo('child2');

    print(`parent has value [${parent.value_}], child1 [${child1.value_}], child2 [${child2.value_}] \n`);

    `hello world, add('21', 2) = ${add('21', 2)}`;
} catch (e) {
    `Execution failed: ${e}`;
}