#define _CRT_SECURE_NO_WARNINGS // shut up about sprintf, MSVC!

#include <catch2/catch_test_macros.hpp>

#include "catch2/matchers/catch_matchers.hpp"
#include "catch2/matchers/catch_matchers_floating_point.hpp"
#include "Dire/Dire.h"
#include "dire/DireProperty.h"
#include "dire/DireReflectable.h"

#include <map>

// Declare some structs to be able to create a hierarchy of reflectable types...

dire_reflectable(struct SuperCompound)
{
	DIRE_REFLECTABLE_INFO()

	DIRE_ARRAY_PROPERTY(int, titi, [5])

};

dire_reflectable(struct Copyable)
{
	DIRE_REFLECTABLE_INFO()

	DIRE_PROPERTY(float, aUselessProp, 4.f)

	Copyable() = default;
	Copyable(Copyable const& pOther)
	{
		aUselessProp = pOther.aUselessProp;
	}

	Copyable& operator=(Copyable const& pOther)
	{
		aUselessProp = pOther.aUselessProp;
		return *this;
	}
};

dire_reflectable(struct testcompound2)
{
	DIRE_REFLECTABLE_INFO()

	DIRE_PROPERTY(int, leet, 1337);
	DIRE_PROPERTY(Copyable, copyable);

	testcompound2() = default;
};

dire_reflectable(struct testcompound)
{
	DIRE_REFLECTABLE_INFO()

	DIRE_PROPERTY(int, compint, 0x2a2a)
	DIRE_PROPERTY(testcompound2, compleet)

	testcompound() = default;
};


dire_reflectable(struct MegaCompound)
{
	DIRE_REFLECTABLE_INFO()

	DIRE_PROPERTY(int, compint, 0x2a2a)
	DIRE_PROPERTY(testcompound2, compleet)
	DIRE_ARRAY_PROPERTY(SuperCompound, toto, [3])

	MegaCompound() = default;
};

dire_reflectable(struct UltraCompound)
{
	DIRE_REFLECTABLE_INFO()

	DIRE_PROPERTY(MegaCompound, mega)

	UltraCompound() = default;
};

dire_reflectable(struct a)
{
	DIRE_REFLECTABLE_INFO()

	int test()
	{
		printf("I'm just a test!\n");
		return 42;
	}

	DIRE_FUNCTION_TYPEINFO(test);

	virtual ~a() = default;

	virtual void print()
	{
		printf("a\n");
	}

	DIRE_PROPERTY(bool, atiti, 0)
	DIRE_PROPERTY(float, atoto, 0)
};


struct NotAReflectable
{
	bool bibi;
	float fat[4];
};

dire_reflectable(struct b, a, NotAReflectable) // an example of multiple inheritance
{
	DIRE_REFLECTABLE_INFO()
	void Roger()
	{
		printf("Roger that sir\n");
	}
	DIRE_FUNCTION_TYPEINFO(Roger);

	void stream(int) {}

	void print() override
	{
		printf("yolo\n");
	}

	DIRE_PROPERTY(double, bdouble, 0)

	DIRE_PROPERTY(testcompound, compvar)

	DIRE_ARRAY_PROPERTY(char, pouet, [10])
};

dire_reflectable(struct c, b)
{
	DIRE_REFLECTABLE_INFO()

	DIRE_PROPERTY(unsigned, ctoto, 0xDEADBEEF)

	DIRE_PROPERTY((std::vector<int>), aVector, std::initializer_list<int>{1, 2, 3})
	DIRE_ARRAY_PROPERTY(int, anArray, [10])
	DIRE_ARRAY_PROPERTY(int, aMultiArray, [10][10])

	DIRE_PROPERTY(MegaCompound, mega);
	DIRE_PROPERTY(UltraCompound, ultra);

	c() = default;

	c(int, bool)
	{
		printf("bonjour int bool\n");
	}

	void print() override
	{
		printf("c\n");
	}
};

dire_reflectable(struct MapCompound)
{
	DIRE_REFLECTABLE_INFO()

	DIRE_PROPERTY((std::map<int, bool>), aBoolMap);

	DIRE_PROPERTY((std::map<int, SuperCompound>), aSuperMap);
};

dire_reflectable(struct d, c)
{
	DIRE_REFLECTABLE_INFO()

	DIRE_PROPERTY((std::map<int, bool>), aMap);

	DIRE_PROPERTY(int, xp, 42)
	DIRE_PROPERTY((std::map<int, bool>), aBoolMap);
	DIRE_PROPERTY((std::map<int, testcompound2>), aFatMap);
	DIRE_PROPERTY((std::map<int, std::map<bool, int>>), aMapInMap);

	DIRE_PROPERTY(MapCompound, aStruct);
};


TEST_CASE("GetProperty Simple", "[Property]")
{
	c superC;

	// direct property
	const unsigned * compu = superC.GetProperty<unsigned>("ctoto");
	REQUIRE(*compu == 0xdeadbeef);
	unsigned compu2 = superC.GetSafeProperty<unsigned>("ctoto");
	REQUIRE(compu2 == 0xdeadbeef);

	// inherited property
	superC.bdouble = 42.0;
	const double* doublePtr = superC.GetProperty<double>("bdouble");
	REQUIRE_THAT(*doublePtr, Catch::Matchers::WithinRel(42.0));
	const double double2 = superC.GetSafeProperty<double>("bdouble");
	REQUIRE_THAT(double2, Catch::Matchers::WithinRel(42.0));

	// in compound
	const int * compint = superC.GetProperty<int>("mega.compint");
	REQUIRE(*compint == 0x2a2a);
	int compint2 = superC.GetSafeProperty<int>("mega.compint");
	REQUIRE(compint2 == 0x2a2a);

	// in nested compound
	superC.ultra.mega.compint = 0x2a;
	compint = superC.GetProperty<int>("ultra.mega.compint");
	REQUIRE(*compint == 0x2a);
	compint2 = superC.GetSafeProperty<int>("ultra.mega.compint");
	REQUIRE(compint2 == 0x2a);

	// non existing
	compint = superC.GetProperty<int>("ABCD");
	REQUIRE(compint == nullptr);

	// TODO: supposed to assert / throw an exception.
	//compint = superC.GetSafeProperty<int>("ABCD");
}

TEST_CASE("GetProperty Array", "[Property]")
{
	c superC;
	for (int i = 0; i < 10; i++)
		superC.anArray[i] = i;

	// direct property
	for (int i = 0; i < 10; i++)
	{
		char buf[64]{};
		sprintf(buf, "anArray[%d]", i);
		const int* ai = superC.GetProperty<int>(buf);
		REQUIRE(*ai == i);
		int compu2 = superC.GetSafeProperty<int>(buf);
		REQUIRE(compu2 == i);
	}

	// inherited
	superC.pouet[6] = '*';
	const char* ch = superC.GetProperty<char>("pouet[6]");
	REQUIRE(*ch == '*');
	char ch2 = superC.GetSafeProperty<char>("pouet[6]");
	REQUIRE(ch2 == '*');

	// in multiarray
	superC.aMultiArray[1][2] = 1337;
	int const* arr1 = superC.GetProperty<int>("aMultiArray[1][2]");
	assert(*arr1 == 1337);
	int val = superC.GetSafeProperty<int>("aMultiArray[1][2]");
	REQUIRE(val == 1337);

	// nested arrays in compounds
	superC.mega.toto[2].titi[3] = 9999;
	arr1 = superC.GetProperty<int>("mega.toto[2].titi[3]");
	REQUIRE(*arr1 == 9999);
	val = superC.GetSafeProperty<int>("mega.toto[2].titi[3]");
	REQUIRE(val == 9999);

	// array-like data structures (std::vector)
	REQUIRE(superC.GetProperty<int>("aVector[0]") != nullptr);
	val = superC.GetSafeProperty<int>("aVector[2]");
	REQUIRE(val == 3);
}

TEST_CASE("GetProperty Map", "[Property]")
{
	d aD;
	aD.aBoolMap[0] = false;

	// direct property
	bool const* mapbool = aD.GetProperty<bool>("aBoolMap[0]");
	REQUIRE(mapbool == &aD.aBoolMap[0]);

	bool bval = aD.GetSafeProperty<bool>("aBoolMap[0]");
	REQUIRE(bval == aD.aBoolMap[0]);

	aD.aBoolMap[1] = true;
	bval = aD.GetSafeProperty<bool>("aBoolMap[0]");
	REQUIRE(bval == aD.aBoolMap[0]);

	// map in compound
	aD.aStruct.aBoolMap[1] = true;
	mapbool = aD.GetProperty<bool>("aStruct.aBoolMap[1]");
	REQUIRE(mapbool == &aD.aStruct.aBoolMap[1]);

	// compound in map
	aD.aFatMap[0].leet = 4242;
	int const* mapint = aD.GetProperty<int>("aFatMap[0].leet");
	REQUIRE(mapint == &aD.aFatMap[0].leet);

	// nested map in compounds
	aD.aStruct.aSuperMap[42].titi[0] = 1;
	aD.aStruct.aSuperMap[42].titi[1] = 2;
	aD.aStruct.aSuperMap[42].titi[2] = 3;

	mapint = aD.GetProperty<int>("aStruct.aSuperMap[42].titi[1]");
	REQUIRE((mapint == &aD.aStruct.aSuperMap[42].titi[1] && *mapint == 2));

	// map in map
	aD.aMapInMap[0][true] = 9999;
	mapint = aD.GetProperty<int>("aMapInMap[0][true]");
	REQUIRE(mapint == &aD.aMapInMap[0][true]);
}

TEST_CASE("SetProperty", "[Property]")
{
	c anotherC;

	// arrays of arrays in compound
	bool modified = anotherC.SetProperty<int>("mega.toto[2].titi[3]", 5678);
	REQUIRE((modified && anotherC.mega.toto[2].titi[3] == 5678));

	// compound + array property in parent class
	anotherC.ultra.mega.toto[0].titi[2] = 0x7f7f;
	modified = anotherC.SetProperty<int>("ultra.mega.toto[0].titi[2]", 0xabcd);
	REQUIRE((modified && anotherC.ultra.mega.toto[0].titi[2] == 0xabcd));

	// test dynamic array (should resize the vector)
	CHECK(anotherC.aVector.size() == 3);
	modified = anotherC.SetProperty<int>("aVector[4]", 0x42);
	REQUIRE((modified && anotherC.aVector.size() == 5 && anotherC.aVector[4] == 0x42));

	// non existing
	modified = anotherC.SetProperty<int>("yolo", 0x42);
	REQUIRE(!modified);
}

TEST_CASE("EraseProperty", "[Property]")
{
	c anotherC;

	CHECK(anotherC.aVector.size() == 3);
	REQUIRE(anotherC.GetProperty<int>("aVector[2]"));
	bool erased = anotherC.EraseProperty("aVector[2]");
	REQUIRE((erased && anotherC.aVector.size() == 2));

	// erase in static array
	anotherC.anArray[5] = 1337;
	erased = anotherC.EraseProperty("anArray[5]");
	REQUIRE((erased && anotherC.anArray[5] == 0));

	// cannot erase a normal prop
	erased = anotherC.EraseProperty("ctoto");
	REQUIRE(!erased);

	// cannot erase non existing
	erased = anotherC.EraseProperty("NOPE");
	REQUIRE(!erased);

	// cannot erase invalid index
	erased = anotherC.EraseProperty("aVector[9999]");
	REQUIRE((!erased && anotherC.aVector.size() == 2));
}