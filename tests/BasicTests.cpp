#include <catch2/catch_test_macros.hpp>
#include "Dire/Dire.h"

struct Test
{
	DIRE_DECLARE_TYPEINFO()

	DIRE_PROPERTY(int, aTest, 42)
};

TEST_CASE("DECLARE_REFLECTABLE_INFO basic test", "[Typeinfo]")
{

}

