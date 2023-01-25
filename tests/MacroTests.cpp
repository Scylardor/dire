#include <catch2/catch_test_macros.hpp>
#include "Dire/Utils/DireMacros.h"


TEST_CASE("DIRE_NARGS (Args counting macro)", "[macros]")
{
    REQUIRE(9 == DIRE_NARGS(1, 2, 3, 4, 5, 6, 7, 8, 9));
    REQUIRE(2 == DIRE_NARGS(1, 2));
    REQUIRE(0 == DIRE_NARGS());
}

