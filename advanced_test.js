advanced_demonstration = true;

function get_weights() {
    var special = new sentinel();
    special.foo("original data that's kinda long");
    return {
        "first": 0.5,
        "second": 13.37,
        "blurgh": 123.456,
        "special": special
    };
}

function print_weights(weights) {
    print(`JS received weights:\n`);
    for (const k in weights) {
        if (k == 'special') {
            print(`received special! special.bar() = ${weights[k].bar()}\n`);
        } else {
            print(`\t${k}: ${weights[k]}\n`);
        }
    }
}

function print_array(a) {
    print(`JS received array:\n`);
    for (const [k, v] of a.entries()) {
        print(`\t${k}: ${v}\n`);
    }
}

function get_array() {
    return [1, 2, 3];
}

`JS advanced demonstration complete`