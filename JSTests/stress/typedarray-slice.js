load("./resources/typedarray-test-helper-functions.js");
description(
"This test checks the behavior of the TypedArray.prototype.slice function"
);

shouldBe("Int32Array.prototype.slice.length", "2");
shouldBe("Int32Array.prototype.slice.name", "'slice'");
shouldBeTrue("isSameFunctionForEachTypedArrayPrototype('slice')");
shouldBeTrue("testPrototypeReceivesArray('slice', [undefined, this, { }, [ ], true, ''])");
debug("");

debug("testPrototypeFunction has the following arg list (name, args, init, result [ , expectedArray ])");
debug("");

debug("1.0 Test Basic Functionality");
shouldBeTrue("testPrototypeFunction('slice', '(2, 3)', [12, 5, 8, 13, 44], [8], [12, 5, 8, 13, 44])");
shouldBeTrue("testPrototypeFunction('slice', '(5, 5)', [12, 5, 8, 13, 44], [])");
shouldBeTrue("testPrototypeFunction('slice', '(0, 5)', [12, 5, 8, 13, 44], [12, 5, 8, 13, 44])");
shouldBeTrue("testPrototypeFunction('slice', '()', [12, 5, 8, 13, 44], [12, 5, 8, 13, 44])");
shouldBeTrue("testPrototypeFunction('slice', '(0, -5)', [12, 5, 8, 13, 44], [])");
shouldBeTrue("testPrototypeFunction('slice', '(-3, -2)', [12, 5, 8, 13, 44], [8])");
shouldBeTrue("testPrototypeFunction('slice', '(4, 2)', [12, 5, 8, 13, 44], [])");
shouldBeTrue("testPrototypeFunction('slice', '(-50, 50)', [12, 5, 8, 13, 44], [12, 5, 8, 13, 44])");
shouldBeTrue("testPrototypeFunction('slice', '(0, 10)', 100000, [0,0,0,0,0,0,0,0,0,0])");
debug("");

debug("2.0 Preserve Underlying bits");

var intView = new Int32Array(5);
intView[0] = -1;
var floatView = new Float32Array(intView.buffer);
floatView = floatView.slice(0,1);
intView = new Int32Array(floatView.buffer);

shouldBe("intView[0]", "-1");
debug("");

debug("3.0 Creates New Buffer");

var intView = new Int32Array(1)
var newView = intView.slice(0,1);
newView[0] = 1;
debug("");

shouldBe("intView[0]", "0");
debug("");

debug("4.0 Symbol.species Test");
subclasses = typedArrays.map(function(constructor) { return class extends constructor { } } );

function testSpecies(array, constructor) {
    let newArray = array.slice(0, 0);
    return newArray instanceof constructor;
}
shouldBeTrue("forEachTypedArray(subclasses, testSpecies)");

Foo = class extends Int32Array { }
subclasses.forEach(function(constructor) { Object.defineProperty(constructor, Symbol.species, { value:Foo, writable:true }); });
function testSpeciesWithFoo(array, constructor) {
    let newArray = array.slice(0, 0);
    return newArray instanceof Foo;
}
shouldBeTrue("forEachTypedArray(subclasses, testSpeciesWithFoo)");
debug("");

debug("4.1 Symbol.species Test throws");
subclasses.forEach(function(constructor) { Object.defineProperty(constructor, Symbol.species, { value:1, writable:true }); });
shouldThrow("forEachTypedArray(subclasses, testSpecies)");

subclasses.forEach(function(constructor) { constructor[Symbol.species] = Array; });
shouldThrow("forEachTypedArray(subclasses, testSpecies)");
debug("");

debug("4.2 Symbol.species Test with Defaults");
subclasses.forEach(function(constructor) { constructor[Symbol.species] = null; });
function testSpeciesIsDefault(array, constructor) {
    let newArray = array.slice(0, 0);
    let defaultConstructor = typedArrays[subclasses.indexOf(constructor)];
    return newArray instanceof defaultConstructor;
}

shouldBeTrue("forEachTypedArray(subclasses, testSpeciesIsDefault)");

subclasses.forEach(function(constructor) { constructor[Symbol.species] = undefined; });
shouldBeTrue("forEachTypedArray(subclasses, testSpeciesIsDefault)");

subclasses = typedArrays.map(function(constructor) { return class extends constructor { } } );
function testSpeciesRemoveConstructor(array, constructor) {
    array.constructor = undefined;
    let newArray = array.slice(0, 0);
    let defaultConstructor = typedArrays[subclasses.indexOf(constructor)];
    return newArray instanceof defaultConstructor;
}

shouldBeTrue("forEachTypedArray(subclasses, testSpeciesRemoveConstructor)");
debug("");

debug("5.0 Symbol.species returns a different type");
buffer = new ArrayBuffer(64);
subclasses.forEach(function(constructor) { Object.defineProperty(constructor, Symbol.species, { value:1, writable:true }); });

function setArray(array) {
    array[0] = 43;
    array[1] = 1.345;
    array[3] = NaN;
    array[4] = -Infinity;
    array[5] = Infinity;
    array[6] = -1;
    array[7] = -0;
    for (let i = 0; i < array.length; i++)
        array[i] = 0;

}

function testSpeciesWithSameBuffer(unused, constructor) {
    return typedArrays.every(function(speciesConstructor) {
        constructor[Symbol.species] = function() { return new speciesConstructor(buffer); };
        let array = new constructor(buffer);
        let otherArray = new speciesConstructor(buffer);
        setArray(array);
        for (let i = 0; i < array.length; i++)
            otherArray[i] = array[i];

        let resultString = otherArray.toString();

        setArray(array);
        otherArray = array.slice(0,array.length)
        let sliceString = otherArray.toString();

        if (sliceString === resultString)
            return true;

        debug("Symbol.species didn't get the correct result got: " + sliceString + " but wanted: " + resultString);
        debug("with initial type: " + constructor.__proto__.name + " and species type " + otherArray.constructor.name);
        return false;
    });
}

shouldBeTrue("forEachTypedArray(subclasses, testSpeciesWithSameBuffer)");


finishJSTest();
