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
	another = BitEnum::two;

	REQUIRE((anEnum == BitEnum::one && anEnum != another));

	another = anEnum;
	REQUIRE((anEnum == BitEnum::one && anEnum == another));
}

TEST_CASE("Bitmask enum bit manipulation", "[Enums]")
{
	BitEnum be = BitEnum::two;
	REQUIRE(strcmp("two", be.GetString()) == 0);

	REQUIRE(strcmp("eight", be.GetStringFromSafeEnum((BitEnum::Values)8)) == 0);

	REQUIRE(!be.IsBitSet(BitEnum::one));

	be.SetBit(BitEnum::eight);
	REQUIRE((!be.IsBitSet(BitEnum::one) && be.IsBitSet(BitEnum::two) && !be.IsBitSet(BitEnum::four) && be.IsBitSet(BitEnum::eight)));

	be.ClearBit(BitEnum::eight);
	be.SetBit(BitEnum::one);
	REQUIRE((be.IsBitSet(BitEnum::one) && be.IsBitSet(BitEnum::two) && !be.IsBitSet(BitEnum::four) && !be.IsBitSet(BitEnum::eight)));
}