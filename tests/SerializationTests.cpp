
#include "catch2/catch_test_macros.hpp"
#include <dire/DireJSONSerializer.h>

#include "dire/DireBinaryDeserializer.h"
#include "dire/DireBinarySerializer.h"
#include "dire/DireJSONDeserializer.h"

TEST_CASE("JSON serialization", "[Serialization]")
{
	dire::RapidJsonReflectorSerializer jsonSerializer;

}

TEST_CASE("JSON deserialization", "[Serialization]")
{
	dire::RapidJsonReflectorDeserializer jsondeSerializer;

}


TEST_CASE("Binary serialization", "[Serialization]")
{
	dire::BinaryReflectorSerializer binarySerializer;

}

TEST_CASE("Binary deserialization", "[Serialization]")
{
	dire::BinaryReflectorDeserializer binaryDeserializer;

}