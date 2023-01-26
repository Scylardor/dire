#define _CRT_SECURE_NO_WARNINGS // shut up about sprintf, MSVC!

#include <catch2/catch_test_macros.hpp>

#include "catch2/matchers/catch_matchers.hpp"
#include "catch2/matchers/catch_matchers_floating_point.hpp"
#include "Dire/Dire.h"
#include "dire/DireProperty.h"
#include "dire/DireReflectable.h"

#include "TestClasses.h"

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

	// Safe property MUSTS refer to a valid name
	REQUIRE_THROWS_AS(superC.GetSafeProperty<int>("ABCD"), std::runtime_error);
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
	testNS::Nested3 child;
	child.pouet[6] = '*';
	const char* ch = child.GetProperty<char>("pouet[6]");
	REQUIRE(*ch == '*');
	char ch2 = child.GetSafeProperty<char>("pouet[6]");
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

	// Array of Reflectables
	MegaCompound mega;
	mega.toto[0].titi[2] = 0x1337;
	REQUIRE(mega.GetProperty<int>("toto[0]") != nullptr);
	const SuperCompound& super = mega.GetSafeProperty<SuperCompound>("toto[0]");
	REQUIRE((&super == &mega.toto[0] && super.titi[2] == 0x1337));

	// mismatched brackets
	dire::Reflectable::PropertyAccessor accessor = superC.GetProperty("aVector[error");
	REQUIRE((!accessor.IsValid() && *accessor.GetError() == "Syntax error: Mismatched bracket or empty brackets."));

	// We don't bother checking for the right bracket if there's no left bracket so it's going to end up in generic syntax error but that's ok.
	accessor = superC.GetProperty("aVector]error");
	REQUIRE((!accessor.IsValid() && *accessor.GetError() == "Syntax error"));

	// empty brackets
	accessor = superC.GetProperty("aVector[]");
	REQUIRE((!accessor.IsValid() && *accessor.GetError() == "Syntax error: Mismatched bracket or empty brackets."));
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

	// enum key
	enumTestType enums;
	enums.allowedQueens[Queens::Judith] = true;
	const bool* allowed = enums.GetProperty<bool>("allowedQueens[Judith]");
	REQUIRE(allowed == &enums.allowedQueens[Queens::Judith]);

	// mismatched brackets
	dire::Reflectable::PropertyAccessor accessor = aD.GetProperty("aMapInMap[error");
	REQUIRE((!accessor.IsValid() && *accessor.GetError() == "Syntax error: Mismatched bracket or empty brackets."));

	// We don't bother checking for the right bracket if there's no left bracket so it's going to end up in generic syntax error but that's ok.
	accessor = aD.GetProperty("aMapInMap]error");
	REQUIRE((!accessor.IsValid() && *accessor.GetError() == "Syntax error"));

	// empty brackets
	accessor = aD.GetProperty("aMapInMap[]");
	REQUIRE((!accessor.IsValid() && *accessor.GetError() == "Syntax error: Mismatched bracket or empty brackets."));
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

	// erase in vector
	CHECK(anotherC.aVector.size() == 3);
	REQUIRE(anotherC.GetProperty<int>("aVector[2]"));
	bool erased = anotherC.EraseProperty("aVector[2]");
	REQUIRE((erased && anotherC.aVector.size() == 2));

	// erase in static array
	anotherC.anArray[5] = 1337;
	erased = anotherC.EraseProperty("anArray[5]");
	REQUIRE((erased && anotherC.anArray[5] == 0));

	// erase in map
	MapCompound aMapType;
	aMapType.aBoolMap[0] = false;
	aMapType.aBoolMap[1] = true;
	erased = aMapType.EraseProperty("aBoolMap[0]");
	REQUIRE((erased && aMapType.aBoolMap.size() == 1 && aMapType.aBoolMap.find(0) == aMapType.aBoolMap.end()));

	// cannot erase a normal prop
	erased = anotherC.EraseProperty("ctoto");
	REQUIRE(!erased);

	// cannot erase non existing
	erased = anotherC.EraseProperty("NOPE");
	REQUIRE(!erased);

	// cannot erase invalid index
	erased = anotherC.EraseProperty("aVector[9999]");
	REQUIRE((!erased && anotherC.aVector.size() == 2));

	// cannot erase invalid key
	erased = aMapType.EraseProperty("aBoolMap[42]");
	REQUIRE((!erased && aMapType.aBoolMap.size() == 1 && aMapType.aBoolMap.find(42) == aMapType.aBoolMap.end()));

	// erasing a vector should clear it
	erased = anotherC.EraseProperty("aVector");
	REQUIRE((erased && anotherC.aVector.empty()));

	// erasing a map should clear it
	erased = aMapType.EraseProperty("aBoolMap");
	REQUIRE((erased && aMapType.aBoolMap.empty()));

	// erasing a static array should fill with default values (e.g. 0 for ints)
	for (int i = 0; i < 10; ++i)
		anotherC.anArray[i] = i;

	static const int testArray[10]{ 0 };
	erased = anotherC.EraseProperty("anArray");
	REQUIRE((erased && memcmp(testArray, anotherC.anArray, sizeof(testArray)) == 0));

}