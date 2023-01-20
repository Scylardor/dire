#include <catch2/catch_test_macros.hpp>
#include "Dire/DireEnums.h"

TEST_CASE("FindFirstSetBit", "[Enums]")
{
	unsigned value = 0b0;
	REQUIRE(dire::FindFirstSetBit(value) == UINT32_MAX);

	value = 0b1;
	REQUIRE(dire::FindFirstSetBit(value) == 0);

	value = 0b10;
	REQUIRE(dire::FindFirstSetBit(value) == 1);

	value = 0b100;
	REQUIRE(dire::FindFirstSetBit(value) == 2);

	value = 0b101;
	REQUIRE(dire::FindFirstSetBit(value) == 0);

	value = 0b111;
	REQUIRE(dire::FindFirstSetBit(value) == 0);

	value = 0b1010;
	REQUIRE(dire::FindFirstSetBit(value) == 1);

	value = 0b10000000000000000000000000000000;
	REQUIRE(dire::FindFirstSetBit(value) == 31);
}

DIRE_BITMASK_ENUM(BitEnum, int, one, two, four, eight);

TEST_CASE("Bitmask enum values", "[Enums]")
{
	REQUIRE((BitEnum::one == 1 && BitEnum::two == 2 && BitEnum::four == 4 && BitEnum::eight == 8));

	BitEnum anEnum{ BitEnum::one };
	BitEnum another{ anEnum };
	REQUIRE(another == anEnum);
	another = BitEnum::two;

	REQUIRE((anEnum == BitEnum::one && anEnum != another));

	another = anEnum;
	REQUIRE((anEnum == BitEnum::one && anEnum == another));

	std::string enumStr = anEnum.GetString();
	REQUIRE(enumStr == "one");

	enumStr = dire::String::to_string(anEnum);
	REQUIRE(enumStr == "one");

	std::string fromEnum = BitEnum::GetStringFromEnum(BitEnum::eight);
	REQUIRE(fromEnum == "eight");

	BitEnum fromString = BitEnum::GetValueFromString("one");
	REQUIRE(fromString == BitEnum::one);

	std::vector<std::string> strings;
	std::vector<std::string> goodStrings{ "one", "two", "four", "eight" };

	int check = 1;
	BitEnum::Enumerate([&](BitEnum value)
		{
			REQUIRE(check == value.Value);
			check <<= 1;
			strings.push_back(value.GetString());
		});
	REQUIRE(strings == goodStrings);

	std::string typeName = BitEnum::GetTypeName();
	REQUIRE(typeName == "BitEnum");
}

TEST_CASE("Bitmask enum bit manipulation", "[Enums]")
{
	BitEnum be = BitEnum::two;
	REQUIRE(be.GetString() == "two");

	REQUIRE(strcmp("eight", be.GetStringFromSafeEnum((BitEnum::Values)8)) == 0);

	REQUIRE(!be.IsBitSet(BitEnum::one));

	be.SetBit(BitEnum::eight);
	REQUIRE((!be.IsBitSet(BitEnum::one) && be.IsBitSet(BitEnum::two) && !be.IsBitSet(BitEnum::four) && be.IsBitSet(BitEnum::eight)));

	be.ClearBit(BitEnum::eight);
	be.SetBit(BitEnum::one);
	REQUIRE((be.IsBitSet(BitEnum::one) && be.IsBitSet(BitEnum::two) && !be.IsBitSet(BitEnum::four) && !be.IsBitSet(BitEnum::eight)));

	std::string oredValues = be.GetString();
	REQUIRE(oredValues == "one | two");
	BitEnum test = BitEnum::GetValueFromString(oredValues);
	REQUIRE((test.IsBitSet(BitEnum::one) && test.IsBitSet(BitEnum::two) && !test.IsBitSet(BitEnum::four) && !test.IsBitSet(BitEnum::eight)));

	BitEnum test2 = BitEnum::GetValueFromString("one | two | four");
	REQUIRE((test2.IsBitSet(BitEnum::one) && test2.IsBitSet(BitEnum::two) && test2.IsBitSet(BitEnum::four) && !test2.IsBitSet(BitEnum::eight)));

	BitEnum test3 = BitEnum::GetValueFromString("eight");
	REQUIRE((!test3.IsBitSet(BitEnum::one) && !test3.IsBitSet(BitEnum::two) && !test3.IsBitSet(BitEnum::four) && test3.IsBitSet(BitEnum::eight)));

	BitEnum testops = BitEnum::one;
	testops |= BitEnum::two;
	REQUIRE((testops.IsBitSet(BitEnum::one) && testops.IsBitSet(BitEnum::two) && !testops.IsBitSet(BitEnum::four) && !testops.IsBitSet(BitEnum::eight)));
	testops = BitEnum::one | BitEnum::two;
	REQUIRE((testops.IsBitSet(BitEnum::one) && testops.IsBitSet(BitEnum::two) && !testops.IsBitSet(BitEnum::four) && !testops.IsBitSet(BitEnum::eight)));
	testops = test | testops | BitEnum::four;
	REQUIRE((testops.IsBitSet(BitEnum::one) && testops.IsBitSet(BitEnum::two) && testops.IsBitSet(BitEnum::four) && !testops.IsBitSet(BitEnum::eight)));

	testops &= BitEnum::two;
	REQUIRE((!testops.IsBitSet(BitEnum::one) && testops.IsBitSet(BitEnum::two) && !testops.IsBitSet(BitEnum::four) && !testops.IsBitSet(BitEnum::eight)));
	testops = BitEnum::one & BitEnum::two;
	REQUIRE((!testops.IsBitSet(BitEnum::one) && !testops.IsBitSet(BitEnum::two) && !testops.IsBitSet(BitEnum::four) && !testops.IsBitSet(BitEnum::eight)));
	testops = test;
	testops = test & testops & BitEnum::four;
	REQUIRE((!testops.IsBitSet(BitEnum::one) && !testops.IsBitSet(BitEnum::two) && !testops.IsBitSet(BitEnum::four) && !testops.IsBitSet(BitEnum::eight)));

	testops = BitEnum::one;
	testops <<= 1;
	REQUIRE((!testops.IsBitSet(BitEnum::one) && testops.IsBitSet(BitEnum::two) && !testops.IsBitSet(BitEnum::four) && !testops.IsBitSet(BitEnum::eight)));
	testops = BitEnum::one << 2;
	REQUIRE((!testops.IsBitSet(BitEnum::one) && !testops.IsBitSet(BitEnum::two) && testops.IsBitSet(BitEnum::four) && !testops.IsBitSet(BitEnum::eight)));

	testops = BitEnum::eight;
	REQUIRE((!testops.IsBitSet(BitEnum::one) && !testops.IsBitSet(BitEnum::two) && !testops.IsBitSet(BitEnum::four) && testops.IsBitSet(BitEnum::eight)));
	testops >>= 1;
	REQUIRE((!testops.IsBitSet(BitEnum::one) && !testops.IsBitSet(BitEnum::two) && testops.IsBitSet(BitEnum::four) && !testops.IsBitSet(BitEnum::eight)));

	test = BitEnum::one | BitEnum::two;
	testops = BitEnum::two | BitEnum::four;
	testops ^= test;
	REQUIRE((testops.IsBitSet(BitEnum::one) && !testops.IsBitSet(BitEnum::two) && testops.IsBitSet(BitEnum::four) && !testops.IsBitSet(BitEnum::eight)));
	testops = BitEnum::one ^ BitEnum::two;
	REQUIRE((testops.IsBitSet(BitEnum::one) && testops.IsBitSet(BitEnum::two) && !testops.IsBitSet(BitEnum::four) && !testops.IsBitSet(BitEnum::eight)));
	testops = test ^ testops ^ BitEnum::four;
	REQUIRE((!testops.IsBitSet(BitEnum::one) && !testops.IsBitSet(BitEnum::two) && testops.IsBitSet(BitEnum::four) && !testops.IsBitSet(BitEnum::eight)));

	testops = BitEnum::two;
	testops = ~testops;
	REQUIRE((testops.IsBitSet(BitEnum::one) && !testops.IsBitSet(BitEnum::two) && testops.IsBitSet(BitEnum::four) && testops.IsBitSet(BitEnum::eight)));
}


DIRE_SEQUENTIAL_ENUM(TestEnum, int, one, two, three, four);

TEST_CASE("Sequential Enum", "[Enums]")
{
	TestEnum enumtest(TestEnum::one);
	REQUIRE(enumtest == TestEnum::one);
	REQUIRE((enumtest != TestEnum::two && enumtest != TestEnum::three && enumtest != TestEnum::four));
	TestEnum enumtest2 = enumtest;
	REQUIRE(enumtest == enumtest2);
	enumtest = TestEnum::two;
	REQUIRE(enumtest == TestEnum::two);
	enumtest2 = enumtest;
	REQUIRE(enumtest2 == TestEnum::two);
	enumtest2 = TestEnum::four;
	REQUIRE(enumtest != enumtest2);

	using std::to_string;
	std::string enumStr = enumtest2.GetString();
	REQUIRE(enumStr == "four");

	enumStr = dire::String::to_string(enumtest2);
	REQUIRE(enumStr == "four");

	std::string fromEnum = TestEnum::GetStringFromEnum(TestEnum::three);
	REQUIRE(fromEnum == "three");

	TestEnum fromString = *TestEnum::GetValueFromString("one");
	REQUIRE(fromString == TestEnum::one);

	std::vector<std::string> strings;
	std::vector<std::string> goodStrings{"one", "two", "three", "four"};

	int check = 0;
	TestEnum::Enumerate([&](TestEnum value)
	{
		REQUIRE(check == value.Value);
		check++;
		strings.push_back(value.GetString());
	});
	REQUIRE(strings == goodStrings);

	std::string typeName = TestEnum::GetTypeName();
	REQUIRE(typeName == "TestEnum");
}
