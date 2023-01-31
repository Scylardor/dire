# What is DIRE?

DIRE is a C++17 dynamic reflection framework that provides usual features found in a game engine's reflection toolbox:

- "Smart" Enum types (Sequential Enum and Bitmask Enum, similar to Unreal's UENUM)
- Properties (and property metadata, similar to Unreal's UPROPERTY)
- Functions (similar to Unreal's UFUNCTION)
- Subclass (similar to Unreal's TSubclassOf<T>)
- Reflectables (similar to Unreal's UObject)
- Can provide Self (for the class itself) and Super (for parent class) typedefs
- De/Serialization API (JSON and binary formats)
- CRUD access to properties via string representation (think of it as a poor man's Boost.Spirit)
- Stateful TypeInfo Database that can be exported to/imported from a file to restore reflectable type information for each type
- Doesn't rely on RTTI to work.

Some minor features need C++20 to work (i.e. DisplayName or FValueRange metadata types) but the only real requirement is C++17.

The name stands for Dynamically Interpreted Reflection Expressions (or Deeply Insane Reflection Experiment, depending on how you look at it).

# Examples

Some examples of what you can do with DIRE (adapted from the unit tests):

## Bitmask enums
```cpp
DIRE_BITMASK_ENUM(BitEnum, int, one, two, four, eight);

BitEnum be = BitEnum::two;
std::string str = be.GetString(); // "two"

str = "one | two | four";
BitEnum test = BitEnum::GetValueFromString(oredValues);
assert(be.IsBitSet(BitEnum::one) && be.IsBitSet(BitEnum::two) && be.IsBitSet(BitEnum::four) && !be.IsBitSet(BitEnum::eight));

BitEnum testops = BitEnum::one;
testops |= BitEnum::two;
assert(testops.IsBitSet(BitEnum::one) && testops.IsBitSet(BitEnum::two) && !testops.IsBitSet(BitEnum::four) && !testops.IsBitSet(BitEnum::eight));
```

## Regular sequential enums (with superpowers)
```cpp
DIRE_SEQUENTIAL_ENUM(TestEnum, int, one, two, three, four);

TestEnum fromString = *TestEnum::GetValueFromString("one");
assert(fromString == TestEnum::one);

std::vector<std::string> strings;
std::vector<std::string> goodStrings{"one", "two", "three", "four"};

int check = 0;
TestEnum::Enumerate([&](TestEnum value)
{
	assert(check == value.Value);
	check++;
	strings.push_back(value.GetString());
});
assert(strings == goodStrings);
```


## Property manipulation
```cpp
dire_reflectable(struct testcompound)
{
	testcompound() = default;
	
	DIRE_REFLECTABLE_INFO()

	DIRE_PROPERTY(int, answerTwice, 0x2a2a)
};

dire_reflectable(struct b)
{
	DIRE_REFLECTABLE_INFO()

	DIRE_PROPERTY(testcompound, aMemberStruct)
};

dire_reflectable(struct c, b) // c inherits from b
{
	c() = default;

	DIRE_REFLECTABLE_INFO()

	DIRE_PROPERTY(unsigned, anErrorCode, 0xDEADBEEF)

	DIRE_PROPERTY((std::vector<int>), aVector, std::initializer_list<int>{1, 2, 3})

	DIRE_ARRAY_PROPERTY(int, anArray, [10])

	DIRE_PROPERTY((std::map<int, bool>), anEvenOddMap);
};


c aC;
const unsigned * error = aC.GetProperty<unsigned>("anErrorCode");
assert(*compu == 0xDEADBEEF);

const int * answerTwice = aC.GetProperty<int>("aMemberStruct.answerTwice");
REQUIRE(*answerTwice == 0x2a2a);

aC.anArray[5] = '*';
const char* ch = aC.GetProperty<char>("anArray[5]");
REQUIRE(*ch == '*');

const int* veci = aC.GetProperty<int>("aVector[1]");
assert(*val == 2);

bool modified = aC.SetProperty<bool>("aEvenOddMap[1]", true);
assert(modified && aC.aEvenOddMap[1] == true);
```

## Type introspection
```cpp

namespace testNS
{
	dire_reflectable(struct Nested)
	{
		DIRE_REFLECTABLE_INFO()

		int TheAnswer()
		{
			return 42;
		}
		DIRE_FUNCTION_TYPEINFO(TheAnswer);
	};
}

dire_reflectable(struct SubCompound)
{
	DIRE_REFLECTABLE_INFO()

	DIRE_PROPERTY(float, aUselessProp, 4.f)
};

dire_reflectable(struct Compound)
{
	DIRE_REFLECTABLE_INFO()

	DIRE_PROPERTY(int, leet, 1337);
	DIRE_PROPERTY(SubCompound, sub);
};

// Reusing the b and c types from previous example...

c aC;
assert(aC.IsA<b>());
static_assert(std::is_same_v<c::Self, c>);
static_assert(std::is_same_v<c::Super, b>);

assert(b::GetTypeInfo().GetName() == "b");
assert(c::GetTypeInfo().GetName() == "c");
assert(testNS::Nested::GetTypeInfo().GetName() == "testNS::Nested");

// generic instantiate
dire::Subclass<b> aSubClass;
assert(aSubClass.GetClassID() == b::GetTypeInfo().GetID());

aSubClass.SetClass(c::GetTypeInfo().GetID());
assert(aSubClass.GetClassID() == c::GetTypeInfo().GetID());
b* newB = aSubClass.Instantiate();
assert((nullptr != newB && dynamic_cast<c*>(newB) != nullptr));
delete newB; // So far, Instantiate is handled with new, so deleting the allocated pointer is up to the caller.

testNS::Nested aNested;
int answer = dire::Invoke<int>("TheAnswer", aNested);
assert(answer == 42);

Compound clonedComp;
clonedComp.leet = 123456789;
clonedComp.sub.aUselessProp = 42.f;
Compound* clone = clonedComp.Clone<Compound>();
assert(clone && clone->leet == clonedComp.leet && clone->sub.aUselessProp == clonedComp.sub.aUselessProp && &clone->leet != &clonedComp.leet);
delete clone; // So far, Cloning is handled with new, so deleting the pointer is up to the caller.
```

## Serialization
```cpp
// Reusing Compound from previous example...

dire::JsonReflectorSerializer serializer;
dire::JsonReflectorDeserializer deserializer;
std::string serialized;

Compound clonedComp;
clonedComp.leet = 123456789;
clonedComp.sub.aUselessProp = 42.f;

serialized = serializer.Serialize(clonedComp).AsString();
assert(serialized == "{\"leet\":123456789,\"sub\":{\"aUselessProp\":42.0}}");

Compound deserializedClonedComp;
deserializer.DeserializeInto(serialized.data(), deserializedClonedComp);
assert(deserializedClonedComp.leet == clonedComp.leet && deserializedClonedComp.sub.aUselessProp == clonedComp.sub.aUselessProp);

// or...

dire::BinaryReflectorSerializer serializer;
dire::BinaryReflectorDeserializer deserializer;

Compound clonedComp;
clonedComp.leet = 123456789;
clonedComp.copyable.aUselessProp = 42.f;

dire::ISerializer::Result serializedBytes = serializer.Serialize(clonedComp);
auto binarized = serializedBytes.GetBytes();

const char expectedResult[] = 
	"\x06\x00\x00\x00\x02\x00\x00\x00\x03\x00\x00\x00\x0c\x00\x00\x00\x15\xcd\x5b\x07\x0c\x00\x00\x00\x10\x00\x00\x00\x05\x00\x00\x00\x01\x00\x00\x00\x07\x00\x00\x00\x08\x00\x00\x00\x00\x00\x28\x42";
auto expectedVec = std::vector<std::byte>((const std::byte*)expectedResult, (const std::byte*)expectedResult + sizeof expectedResult - 1);
assert(binarized == expectedVec);

Compound deserializedClonedComp;
deserializer.DeserializeInto((const char*)binarized.data(), deserializedClonedComp);
assert(deserializedClonedComp.leet == clonedComp.leet && deserializedClonedComp.copyable.aUselessProp == clonedComp.copyable.aUselessProp);
```

# What does it need?

A C++17-able compiler. An non-exhaustive list of used features follows:
- std::any
- std::optional
- std::variant
- std::from_chars
- structured bindings
- [[nodiscard]]

Some metadata types, namely FValueRange and DisplayName, also require C++20 to be used.

# What are the limitations?

There are some things to bear in mind when using DIRE:

- All classes who inherit Reflectable become virtual because of its pure functions.
It means that it will make your classes non-standard layout, and a bit heavier because of the virtual table pointer.
Since the TypeInfo database and Reflectables are two separate entities, it should be possible to address that issue in the future.

- Using DIRE could also increase the program's startup time, because of the static initialization machinery needed to initialize everything.
Granted, it's also going to make your executable size bigger, because more static memory will have to be bundled into it.

- DIRE relies a lot on virtual function calls. Yhose are known for being more expensive to call than regular functions.
Modern compilers are however getting better at devirtualizing virtual function calls, so: YMMV.

- DIRE uses a fair amount of macros and complex templates to work. As a result, I've seen measurable increases in compile time by using it.
I'm currently trying to address this.

- DIRE does not support template reflectables at the moment (if you try, it's going to cause syntax errors and won't compile).

- The de/serialization API is at the proof-of-concept stage right now, and could use some optimization.

- Always keep in mind that:
	- I have a full-time job, so I will support this library "when I can".
	- This project is experimental.


# License

This library is licensed under the LGPL.

In other words, any usage is allowed, even for commercial purpose.

But if you modify the library to make improvements, those improvements have to go back to the public domain.


# But... Why?

Because it's a fun hobby project. :-)

DIRE's feature set was inspired by the various game engines I use (Unity, Unreal Engine...).
I programmed a lot with the Unreal Engine, so that's why the features of DIRE were heavily inspired by it.
I wanted to be able to reproduce as many features as I could from UE in my personal game engine project.

It may not be *perfect*,but it seems to me that the other reflection libraries' don't really overlap with what I actually want.
Most C++17/20 reflection libraries are impressively easy to use when the serialized type is known at compile-time (thanks to templates).
But if we're dealing with dynamic objects that are only a pointer to an interface, they cannot use templates anymore.

And I don't want to add a big dependency like Boost to my project, and *still* have a lot to program myself on top of it anyway.

In a game engine, it's pretty common to work with abstracted types, especially while communicating with a game editor.
When the editor sends an updated property sheet, I want to be able to use an abstraction layer that deals with the updating
without knowing what type of object are we actually modifying.

Thanks to the fact that DIRE is able to create, update and delete properties just by parsing text, I don't have to know the type of object I'm editing in advance.
All I need to know is that it's a Reflectable!

On the other hand, I want to have a solution to serialize the properties of an object without having to know its actual type,
so that I can write generic serialization code that only relies on the Reflectable interface.

DIRE is able to do that in two formats: JSON (mostly useful to communicate back and forth with a game editor) and binary (mostly useful for packaged games).

Hence, DIRE was born.
