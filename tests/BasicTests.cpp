#include <sstream> // ostringstream
#include <catch2/catch_test_macros.hpp>
#include "Dire/Dire.h"

struct BasicTest
{
	DIRE_DECLARE_TYPEINFO()

	DIRE_PROPERTY(bool, aBool, true)
	DIRE_PROPERTY(char, aChar, 1)
	DIRE_PROPERTY(unsigned char, aUChar, 2)
	DIRE_PROPERTY(short, aShort, 3)
	DIRE_PROPERTY(unsigned short, aUShort, 4)
	DIRE_PROPERTY(int, anInt, 5)
	DIRE_PROPERTY(int64_t, anInt64, 6)
	DIRE_PROPERTY(unsigned, anUInt, 7)
	DIRE_PROPERTY(uint64_t, anUInt64, 8)
	DIRE_PROPERTY(float, aFloat, NAN)
	DIRE_PROPERTY(double, aDouble, NAN)
};

TEST_CASE("Test DIRE_PROPERTY sets values", "[Basic Properties]")
{
	BasicTest test;
	REQUIRE((test.aBool == true && test.aChar == 1
		&& test.aUChar == 2 && test.aShort == 3
		&& test.aUShort == 4 && test.anInt == 5
		&& test.anInt64 == 6 && test.anUInt == 7 && test.anUInt64 == 8
		&& isnan(test.aFloat) && isnan(test.aDouble)));
}

TEST_CASE("GetPropertyList with DECLARE_REFLECTABLE_INFO and basic types", "[Basic Properties]")
{
	std::ostringstream output;
	auto& refl = BasicTest::GetClassReflectableTypeInfo();

	for (dire::PropertyTypeInfo const& prop : refl.GetPropertyList())
	{
		output << prop.GetName().data() << ' ' << prop.GetSize() << ' ' << prop.GetOffset() << ' ' << prop.GetMetatype().GetString() << '\n';
	}
	auto result = output.str();
	REQUIRE(result == "aBool 1 0 Bool\naChar 1 1 Char\naUChar 1 2 UChar\naShort 2 4 Short\naUShort 2 6 UShort\nanInt 4 8 Int\nanInt64 8 16 Int64\nanUInt 4 24 Uint\nanUInt64 8 32 Uint64\naFloat 4 40 Float\naDouble 8 48 Double\n");
}
