#include "DireDefines.h"
#ifdef DIRE_SERIALIZATION_ENABLED
#	include <iomanip>
#	include <iostream>

#	include "catch2/catch_test_macros.hpp"

#	include "TestClasses.h"

DIRE_SEQUENTIAL_ENUM(Kings, int, Philippe, Alexandre, Cesar, Charles);
DIRE_BITMASK_ENUM(BitEnum, int, one, two, four, eight);
DIRE_BITMASK_ENUM(Jacks, short, Ogier, Lahire, Hector, Lancelot);
DIRE_BITMASK_ENUM(Queens, short, Judith, Rachel, Pallas, Argine);

enum class Faces : uint8_t
{
	Jack,
	Queen,
	King
};

dire_reflectable(struct enumTestType)
{
	DIRE_REFLECTABLE_INFO()

	DIRE_PROPERTY(Faces, aTestFace);
	DIRE_PROPERTY(Kings, bestKing, Kings::Alexandre);
	DIRE_ARRAY_PROPERTY(Kings, worstKings, [2])
	DIRE_PROPERTY((std::vector<Kings>), playableKings);
	DIRE_PROPERTY((std::map<Queens, bool>), allowedQueens);
	DIRE_PROPERTY((std::map<int, Jacks>), pointsPerJack);

	bool operator==(const enumTestType & pRhs) const
	{
		return aTestFace == pRhs.aTestFace && bestKing == pRhs.bestKing && memcmp(worstKings, pRhs.worstKings, sizeof(Kings) * 2) == 0
			&& playableKings == pRhs.playableKings && allowedQueens == pRhs.allowedQueens && pointsPerJack == pRhs.pointsPerJack;
	}
};

#	ifdef DIRE_SERIALIZATION_RAPIDJSON_ENABLED
#		include "dire/Serialization/DireJSONSerializer.h"
#		include "dire/Serialization/DireJSONDeserializer.h"


TEST_CASE("JSON simple object", "[Serialization]")
{
	dire::RapidJsonReflectorSerializer serializer;
	dire::RapidJsonReflectorDeserializer deserializer;
	std::string serialized;

	testcompound2 clonedComp;
	clonedComp.leet = 123456789;
	clonedComp.copyable.aUselessProp = 42.f;

	serialized = serializer.Serialize(clonedComp).AsString();
	REQUIRE(serialized == "{\"leet\":123456789,\"copyable\":{\"aUselessProp\":42.0}}");

	testcompound2 deserializedClonedComp;
	deserializer.DeserializeInto(serialized.data(), deserializedClonedComp);
	REQUIRE((deserializedClonedComp.leet == clonedComp.leet && deserializedClonedComp.copyable.aUselessProp == clonedComp.copyable.aUselessProp));
}

TEST_CASE("JSON array", "[Serialization]")
{
	dire::RapidJsonReflectorSerializer serializer;
	dire::RapidJsonReflectorDeserializer deserializer;
	std::string serialized;

	SuperCompound seriaArray;

	for (int i = 0; i < 5; ++i)
		seriaArray.titi[i] = i;
	serialized = serializer.Serialize(seriaArray).AsString();
	REQUIRE(serialized == "{\"titi\":[0,1,2,3,4]}");

	SuperCompound deserializedseriaArray;
	deserializer.DeserializeInto(serialized.data(), deserializedseriaArray);
	REQUIRE(memcmp(&seriaArray.titi, &deserializedseriaArray.titi, sizeof(deserializedseriaArray.titi)) == 0);
}

TEST_CASE("JSON objects, arrays, and array of objects", "[Serialization]")
{
	dire::RapidJsonReflectorSerializer serializer;
	dire::RapidJsonReflectorDeserializer deserializer;
	std::string serialized;

	MegaCompound megaSerialized;
	for (int i = 0; i < 3; i++)
	{
		for (int j = 0; j < 5; ++j)
		{
			megaSerialized.toto[i].titi[j] = i + j;
		}
	}

	serialized = serializer.Serialize(megaSerialized).AsString();
	REQUIRE(serialized == "{\"compint\":10794,\"compleet\":{\"leet\":1337,\"copyable\":{\"aUselessProp\":4.0}},\"toto\":[{\"titi\":[0,1,2,3,4]},{\"titi\":[1,2,3,4,5]},{\"titi\":[2,3,4,5,6]}]}");

	MegaCompound megaDeserialized;
	deserializer.DeserializeInto(serialized.data(), megaDeserialized);
	REQUIRE(memcmp(&megaDeserialized.toto, &megaSerialized.toto, sizeof(megaSerialized.toto)) == 0);
}

TEST_CASE("JSON std::vector (and other things)", "[Serialization]")
{
	dire::RapidJsonReflectorSerializer serializer;
	dire::RapidJsonReflectorDeserializer deserializer;
	std::string serialized;

	c serializedC;
	serializedC.aVector = { 42,43,44,45,46 };
	for (int i = 0; i < 10; i++)
	{
		serializedC.anArray[i] = i + 1;
		for (int j = 0; j < 10; ++j)
		{
			serializedC.aMultiArray[i][j] = i + j;
		}
	}
	serialized = serializer.Serialize(serializedC).AsString();
	REQUIRE(serialized ==
		"{\"atiti\":false,\"atoto\":0.0,\"bdouble\":0.0,\"compvar\":{\"compint\":10794,\"compleet\":{\"leet\":1337,\"copyable\":{\"aUselessProp\":4.0}}},\"ctoto\":3735928559,\"aVector\":[42,43,44,45,46],\"anArray\":[1,2,3,4,5,6,7,8,9,10],\"aMultiArray\":[[0,1,2,3,4,5,6,7,8,9],[1,2,3,4,5,6,7,8,9,10],[2,3,4,5,6,7,8,9,10,11],[3,4,5,6,7,8,9,10,11,12],[4,5,6,7,8,9,10,11,12,13],[5,6,7,8,9,10,11,12,13,14],[6,7,8,9,10,11,12,13,14,15],[7,8,9,10,11,12,13,14,15,16],[8,9,10,11,12,13,14,15,16,17],[9,10,11,12,13,14,15,16,17,18]],\"mega\":{\"compint\":10794,\"compleet\":{\"leet\":1337,\"copyable\":{\"aUselessProp\":4.0}},\"toto\":[{\"titi\":[0,0,0,0,0]},{\"titi\":[0,0,0,0,0]},{\"titi\":[0,0,0,0,0]}]},\"ultra\":{\"mega\":{\"compint\":10794,\"compleet\":{\"leet\":1337,\"copyable\":{\"aUselessProp\":4.0}},\"toto\":[{\"titi\":[0,0,0,0,0]},{\"titi\":[0,0,0,0,0]},{\"titi\":[0,0,0,0,0]}]}}}");

	c deSerializedC;
	deserializer.DeserializeInto(serialized.data(), deSerializedC);

	REQUIRE((memcmp(&serializedC.anArray, &deSerializedC.anArray, sizeof(serializedC.anArray)) == 0
	&& memcmp(&serializedC.aMultiArray, &deSerializedC.aMultiArray, sizeof(serializedC.aMultiArray)) == 0
	&& memcmp(&serializedC.mega, &deSerializedC.mega, sizeof(serializedC.mega)) == 0
	&& memcmp(&serializedC.ultra, &deSerializedC.ultra, sizeof(serializedC.ultra)) == 0
	&& serializedC.aVector == deSerializedC.aVector
	&& serializedC.ctoto == deSerializedC.ctoto));
}


TEST_CASE("JSON std::map", "[Serialization]")
{
	dire::RapidJsonReflectorSerializer serializer;
	dire::RapidJsonReflectorDeserializer deserializer;
	std::string serialized;

	mapType serializedMap;
	for (int i = 0; i < 10; i++)
	{
		serializedMap.aEvenOddMap[i] = (i % 2 == 0);
	}
	serialized = serializer.Serialize(serializedMap).AsString();
	REQUIRE(serialized == "{\"aEvenOddMap\":{\"0\":true,\"1\":false,\"2\":true,\"3\":false,\"4\":true,\"5\":false,\"6\":true,\"7\":false,\"8\":true,\"9\":false}}");

	mapType deSerializedMap;
	deserializer.DeserializeInto(serialized.data(), deSerializedMap);
	assert(serializedMap.aEvenOddMap == deSerializedMap.aEvenOddMap);
}

TEST_CASE("JSON Serialize compound, map in map, and compound value in map", "[Serialization]")
{
	dire::RapidJsonReflectorSerializer serializer;
	dire::RapidJsonReflectorDeserializer deserializer;
	std::string serialized;

	d aD;
	aD.SetProperty<int>("ultra.mega.toto[0].titi[2]", 0x1234);

	serialized = serializer.Serialize(aD).AsString();
	REQUIRE(serialized == "{\"atiti\":false,\"atoto\":0.0,\"bdouble\":0.0,\"compvar\":{\"compint\":10794,\"compleet\":{\"leet\":1337,\"copyable\":{\"aUselessProp\":4.0}}},\"ctoto\":3735928559,\"aVector\":[1,2,3],\"anArray\":[0,0,0,0,0,0,0,0,0,0],\"aMultiArray\":[[0,0,0,0,0,0,0,0,0,0],[0,0,0,0,0,0,0,0,0,0],[0,0,0,0,0,0,0,0,0,0],[0,0,0,0,0,0,0,0,0,0],[0,0,0,0,0,0,0,0,0,0],[0,0,0,0,0,0,0,0,0,0],[0,0,0,0,0,0,0,0,0,0],[0,0,0,0,0,0,0,0,0,0],[0,0,0,0,0,0,0,0,0,0],[0,0,0,0,0,0,0,0,0,0]],\"mega\":{\"compint\":10794,\"compleet\":{\"leet\":1337,\"copyable\":{\"aUselessProp\":4.0}},\"toto\":[{\"titi\":[0,0,0,0,0]},{\"titi\":[0,0,0,0,0]},{\"titi\":[0,0,0,0,0]}]},\"ultra\":{\"mega\":{\"compint\":10794,\"compleet\":{\"leet\":1337,\"copyable\":{\"aUselessProp\":4.0}},\"toto\":[{\"titi\":[0,0,4660,0,0]},{\"titi\":[0,0,0,0,0]},{\"titi\":[0,0,0,0,0]}]}},\"aMap\":{},\"xp\":42,\"aBoolMap\":{},\"aFatMap\":{},\"aMapInMap\":{},\"aStruct\":{\"aBoolMap\":{},\"aSuperMap\":{}}}");

	d deserializedD;
	deserializer.DeserializeInto(serialized.data(), deserializedD);
	std::string serialized2 = serializer.Serialize(deserializedD);
	REQUIRE(serialized == serialized2); // in theory, if serialization output is the same, objects are the same as far as reflection is concerned
}

TEST_CASE("JSON Serialize enumerations", "[Serialization]")
{
	dire::RapidJsonReflectorSerializer serializer;
	dire::RapidJsonReflectorDeserializer deserializer;
	std::string serialized;

	enumTestType enums;
	enums.aTestFace = Faces::Queen;
	enums.worstKings[0] = Kings::Philippe;
	enums.worstKings[1] = Kings::Charles;
	enums.playableKings = { Kings::Cesar, Kings::Alexandre };
	enums.allowedQueens = { {Queens::Judith, true}, {Queens::Rachel, false} };
	enums.pointsPerJack = { {10, Jacks::Ogier}, {20, Jacks::Lahire}, {30, Jacks::Hector}, {40, Jacks::Lancelot} };

	REQUIRE(Queens::GetStringFromEnum(Queens::Judith) == "Judith");
	REQUIRE(Jacks::GetStringFromEnum(Jacks::Lancelot) == "Lancelot");
	REQUIRE(BitEnum::GetStringFromEnum(BitEnum::one) == "one");
	REQUIRE((int)Queens::Judith == 1);
	REQUIRE((int)Jacks::Lancelot == 8);
	REQUIRE((int)BitEnum::eight == 8);

	serialized = serializer.Serialize(enums).AsString();
	REQUIRE(serialized ==
		"{\"aTestFace\":1,\"bestKing\":\"Alexandre\",\"worstKings\":[\"Philippe\",\"Charles\"],\"playableKings\":[\"Cesar\",\"Alexandre\"],\"allowedQueens\":{\"Judith\":true,\"Rachel\":false},\"pointsPerJack\":{\"10\":\"Ogier\",\"20\":\"Lahire\",\"30\":\"Hector\",\"40\":\"Lancelot\"}}"
	);

	enumTestType deserializedEnums;
	deserializer.DeserializeInto(serialized.data(), deserializedEnums);
	REQUIRE(enums == deserializedEnums);
}

#	endif // DIRE_SERIALIZATION_RAPIDJSON_ENABLED

#	ifdef DIRE_SERIALIZATION_BINARY_ENABLED

#  include "dire/Serialization/DireBinaryDeserializer.h"
#  include "dire/Serialization/DireBinarySerializer.h"

/* Utility function to print the output of binary generators */
auto writeBinaryString2 = [](const std::string& binarized)
{
	std::cout << "\"";
	for (int i = 0; i < binarized.size(); ++i)
		std::cout << "\\x" << std::hex << std::setfill('0') << std::setw(2) << unsigned(binarized[i]);
	std::cout << "\"" << std::endl;
};

TEST_CASE("Binary simple object", "[Serialization]")
{
	dire::BinaryReflectorSerializer serializer;
	dire::BinaryReflectorDeserializer deserializer;
	std::string binarized;

	testcompound2 clonedComp;
	clonedComp.leet = 123456789;
	clonedComp.copyable.aUselessProp = 42.f;

	dire::ISerializer::Result serializedBytes = serializer.Serialize(clonedComp);
	binarized = serializedBytes.AsString();
	REQUIRE(memcmp(binarized.data(),
		"\x07\x00\x00\x00\x02\x00\x00\x00\x03\x00\x00\x00\x0c\x00\x00\x00\x15\xcd\x5b\x07\x0c\x00\x00\x00\x10\x00\x00\x00\x06\x00\x00\x00\x01\x00\x00\x00\x07\x00\x00\x00\x08\x00\x00\x00\x00\x00\x28\x42",
		binarized.size()) == 0);

	testcompound2 deserializedClonedComp;
	deserializer.DeserializeInto(binarized.data(), deserializedClonedComp);
	REQUIRE((deserializedClonedComp.leet == clonedComp.leet && deserializedClonedComp.copyable.aUselessProp == clonedComp.copyable.aUselessProp));
}

TEST_CASE("Binary array", "[Serialization]")
{
	dire::BinaryReflectorSerializer serializer;
	dire::BinaryReflectorDeserializer deserializer;
	std::string binarized;

	SuperCompound seriaArray;
	for (int i = 0; i < 5; ++i)
		seriaArray.titi[i] = i;
	binarized = serializer.Serialize(seriaArray);

	REQUIRE(memcmp(binarized.data(),
		"\x05\x00\x00\x00\x01\x00\x00\x00\x09\x00\x00\x00\x08\x00\x00\x00\x03\x00\x00\x00\x00\x00\x00\x00\x04\x00\x00\x00\x00\x00\x00\x00\x05\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x01\x00\x00\x00\x02\x00\x00\x00\x03\x00\x00\x00\x04\x00\x00\x00",
		binarized.size()) == 0);

	SuperCompound deserializedseriaArray;
	deserializer.DeserializeInto(binarized.data(), deserializedseriaArray);
	REQUIRE(memcmp(&seriaArray.titi, &deserializedseriaArray.titi, sizeof(deserializedseriaArray.titi)) == 0);
}

TEST_CASE("Binary objects, arrays, and array of objects", "[Serialization]")
{
	dire::BinaryReflectorSerializer serializer;
	dire::BinaryReflectorDeserializer deserializer;
	std::string binarized;

	MegaCompound megaSerialized;
	megaSerialized.compint = 13374242;
	megaSerialized.compleet.leet = 31337;
	megaSerialized.compleet.copyable.aUselessProp = 1.f;
	for (int i = 0; i < 3; i++)
	{
		for (int j = 0; j < 5; ++j)
		{
			megaSerialized.toto[i].titi[j] = i + j;
		}
	}

	binarized = serializer.Serialize(megaSerialized);

	REQUIRE(memcmp(binarized.data(),
		"\x09\x00\x00\x00\x03\x00\x00\x00\x03\x00\x00\x00\x08\x00\x00\x00\x22\x13\xcc\x00\x0c\x00\x00\x00\x10\x00\x00\x00\x07\x00\x00\x00\x02\x00\x00\x00\x03\x00\x00\x00\x0c\x00\x00\x00\x69\x7a\x00\x00\x0c\x00\x00\x00\x10\x00\x00\x00\x06\x00\x00\x00\x01\x00\x00\x00\x07\x00\x00\x00\x08\x00\x00\x00\x00\x00\x80\x3f\x09\x00\x00\x00\x30\x00\x00\x00\x0c\x00\x00\x00\x00\x00\x00\x00\x20\x00\x00\x00\x00\x00\x00\x00\x03\x00\x00\x00\x00\x00\x00\x00\x05\x00\x00\x00\x01\x00\x00\x00\x09\x00\x00\x00\x08\x00\x00\x00\x03\x00\x00\x00\x00\x00\x00\x00\x04\x00\x00\x00\x00\x00\x00\x00\x05\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x01\x00\x00\x00\x02\x00\x00\x00\x03\x00\x00\x00\x04\x00\x00\x00\x05\x00\x00\x00\x01\x00\x00\x00\x09\x00\x00\x00\x08\x00\x00\x00\x03\x00\x00\x00\x00\x00\x00\x00\x04\x00\x00\x00\x00\x00\x00\x00\x05\x00\x00\x00\x00\x00\x00\x00\x01\x00\x00\x00\x02\x00\x00\x00\x03\x00\x00\x00\x04\x00\x00\x00\x05\x00\x00\x00\x05\x00\x00\x00\x01\x00\x00\x00\x09\x00\x00\x00\x08\x00\x00\x00\x03\x00\x00\x00\x00\x00\x00\x00\x04\x00\x00\x00\x00\x00\x00\x00\x05\x00\x00\x00\x00\x00\x00\x00\x02\x00\x00\x00\x03\x00\x00\x00\x04\x00\x00\x00\x05\x00\x00\x00\x06\x00\x00\x00"
		, binarized.size()) == 0);

	MegaCompound megaDeserialized;
	deserializer.DeserializeInto(binarized.data(), megaDeserialized);
	REQUIRE(memcmp(&megaDeserialized.toto, &megaSerialized.toto, sizeof(megaSerialized.toto)) == 0);
}

TEST_CASE("Binary std::vector (and other things)", "[Serialization]")
{
	dire::BinaryReflectorSerializer serializer;
	dire::BinaryReflectorDeserializer deserializer;
	std::string binarized;

	c serializedC;
	serializedC.aVector = { 42,43,44,45,46 };
	for (int i = 0; i < 10; i++)
	{
		serializedC.anArray[i] = i + 1;
		for (int j = 0; j < 10; ++j)
		{
			serializedC.aMultiArray[i][j] = i + j;
		}
	}
	binarized = serializer.Serialize(serializedC);

	REQUIRE(memcmp(binarized.data(),
		"\x0d\x00\x00\x00\x0a\x00\x00\x00\x02\x00\x00\x00\x08\x00\x00\x00\x00\x07\x00\x00\x00\x0c\x00\x00\x00\x00\x00\x00\x00\x08\x00\x00\x00\x28\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x0c\x00\x00\x00\x30\x00\x00\x00\x08\x00\x00\x00\x02\x00\x00\x00\x03\x00\x00\x00\x08\x00\x00\x00\x2a\x2a\x00\x00\x0c\x00\x00\x00\x10\x00\x00\x00\x07\x00\x00\x00\x02\x00\x00\x00\x03\x00\x00\x00\x0c\x00\x00\x00\x39\x05\x00\x00\x0c\x00\x00\x00\x10\x00\x00\x00\x06\x00\x00\x00\x01\x00\x00\x00\x07\x00\x00\x00\x08\x00\x00\x00\x00\x00\x80\x40\x04\x00\x00\x00\x60\x00\x00\x00\xef\xbe\xad\xde\x09\x00\x00\x00\x68\x00\x00\x00\x03\x00\x00\x00\x00\x00\x00\x00\x04\x00\x00\x00\x00\x00\x00\x00\x05\x00\x00\x00\x00\x00\x00\x00\x2a\x00\x00\x00\x2b\x00\x00\x00\x2c\x00\x00\x00\x2d\x00\x00\x00\x2e\x00\x00\x00\x09\x00\x00\x00\x88\x00\x00\x00\x03\x00\x00\x00\x00\x00\x00\x00\x04\x00\x00\x00\x00\x00\x00\x00\x0a\x00\x00\x00\x00\x00\x00\x00\x01\x00\x00\x00\x02\x00\x00\x00\x03\x00\x00\x00\x04\x00\x00\x00\x05\x00\x00\x00\x06\x00\x00\x00\x07\x00\x00\x00\x08\x00\x00\x00\x09\x00\x00\x00\x0a\x00\x00\x00\x09\x00\x00\x00\xb0\x00\x00\x00\x09\x00\x00\x00\x00\x00\x00\x00\x28\x00\x00\x00\x00\x00\x00\x00\x0a\x00\x00\x00\x00\x00\x00\x00\x03\x00\x00\x00\x00\x00\x00\x00\x04\x00\x00\x00\x00\x00\x00\x00\x0a\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x01\x00\x00\x00\x02\x00\x00\x00\x03\x00\x00\x00\x04\x00\x00\x00\x05\x00\x00\x00\x06\x00\x00\x00\x07\x00\x00\x00\x08\x00\x00\x00\x09\x00\x00\x00\x03\x00\x00\x00\x00\x00\x00\x00\x04\x00\x00\x00\x00\x00\x00\x00\x0a\x00\x00\x00\x00\x00\x00\x00\x01\x00\x00\x00\x02\x00\x00\x00\x03\x00\x00\x00\x04\x00\x00\x00\x05\x00\x00\x00\x06\x00\x00\x00\x07\x00\x00\x00\x08\x00\x00\x00\x09\x00\x00\x00\x0a\x00\x00\x00\x03\x00\x00\x00\x00\x00\x00\x00\x04\x00\x00\x00\x00\x00\x00\x00\x0a\x00\x00\x00\x00\x00\x00\x00\x02\x00\x00\x00\x03\x00\x00\x00\x04\x00\x00\x00\x05\x00\x00\x00\x06\x00\x00\x00\x07\x00\x00\x00\x08\x00\x00\x00\x09\x00\x00\x00\x0a\x00\x00\x00\x0b\x00\x00\x00\x03\x00\x00\x00\x00\x00\x00\x00\x04\x00\x00\x00\x00\x00\x00\x00\x0a\x00\x00\x00\x00\x00\x00\x00\x03\x00\x00\x00\x04\x00\x00\x00\x05\x00\x00\x00\x06\x00\x00\x00\x07\x00\x00\x00\x08\x00\x00\x00\x09\x00\x00\x00\x0a\x00\x00\x00\x0b\x00\x00\x00\x0c\x00\x00\x00\x03\x00\x00\x00\x00\x00\x00\x00\x04\x00\x00\x00\x00\x00\x00\x00\x0a\x00\x00\x00\x00\x00\x00\x00\x04\x00\x00\x00\x05\x00\x00\x00\x06\x00\x00\x00\x07\x00\x00\x00\x08\x00\x00\x00\x09\x00\x00\x00\x0a\x00\x00\x00\x0b\x00\x00\x00\x0c\x00\x00\x00\x0d\x00\x00\x00\x03\x00\x00\x00\x00\x00\x00\x00\x04\x00\x00\x00\x00\x00\x00\x00\x0a\x00\x00\x00\x00\x00\x00\x00\x05\x00\x00\x00\x06\x00\x00\x00\x07\x00\x00\x00\x08\x00\x00\x00\x09\x00\x00\x00\x0a\x00\x00\x00\x0b\x00\x00\x00\x0c\x00\x00\x00\x0d\x00\x00\x00\x0e\x00\x00\x00\x03\x00\x00\x00\x00\x00\x00\x00\x04\x00\x00\x00\x00\x00\x00\x00\x0a\x00\x00\x00\x00\x00\x00\x00\x06\x00\x00\x00\x07\x00\x00\x00\x08\x00\x00\x00\x09\x00\x00\x00\x0a\x00\x00\x00\x0b\x00\x00\x00\x0c\x00\x00\x00\x0d\x00\x00\x00\x0e\x00\x00\x00\x0f\x00\x00\x00\x03\x00\x00\x00\x00\x00\x00\x00\x04\x00\x00\x00\x00\x00\x00\x00\x0a\x00\x00\x00\x00\x00\x00\x00\x07\x00\x00\x00\x08\x00\x00\x00\x09\x00\x00\x00\x0a\x00\x00\x00\x0b\x00\x00\x00\x0c\x00\x00\x00\x0d\x00\x00\x00\x0e\x00\x00\x00\x0f\x00\x00\x00\x10\x00\x00\x00\x03\x00\x00\x00\x00\x00\x00\x00\x04\x00\x00\x00\x00\x00\x00\x00\x0a\x00\x00\x00\x00\x00\x00\x00\x08\x00\x00\x00\x09\x00\x00\x00\x0a\x00\x00\x00\x0b\x00\x00\x00\x0c\x00\x00\x00\x0d\x00\x00\x00\x0e\x00\x00\x00\x0f\x00\x00\x00\x10\x00\x00\x00\x11\x00\x00\x00\x03\x00\x00\x00\x00\x00\x00\x00\x04\x00\x00\x00\x00\x00\x00\x00\x0a\x00\x00\x00\x00\x00\x00\x00\x09\x00\x00\x00\x0a\x00\x00\x00\x0b\x00\x00\x00\x0c\x00\x00\x00\x0d\x00\x00\x00\x0e\x00\x00\x00\x0f\x00\x00\x00\x10\x00\x00\x00\x11\x00\x00\x00\x12\x00\x00\x00\x0c\x00\x00\x00\x40\x02\x00\x00\x09\x00\x00\x00\x03\x00\x00\x00\x03\x00\x00\x00\x08\x00\x00\x00\x2a\x2a\x00\x00\x0c\x00\x00\x00\x10\x00\x00\x00\x07\x00\x00\x00\x02\x00\x00\x00\x03\x00\x00\x00\x0c\x00\x00\x00\x39\x05\x00\x00\x0c\x00\x00\x00\x10\x00\x00\x00\x06\x00\x00\x00\x01\x00\x00\x00\x07\x00\x00\x00\x08\x00\x00\x00\x00\x00\x80\x40\x09\x00\x00\x00\x30\x00\x00\x00\x0c\x00\x00\x00\x00\x00\x00\x00\x20\x00\x00\x00\x00\x00\x00\x00\x03\x00\x00\x00\x00\x00\x00\x00\x05\x00\x00\x00\x01\x00\x00\x00\x09\x00\x00\x00\x08\x00\x00\x00\x03\x00\x00\x00\x00\x00\x00\x00\x04\x00\x00\x00\x00\x00\x00\x00\x05\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x05\x00\x00\x00\x01\x00\x00\x00\x09\x00\x00\x00\x08\x00\x00\x00\x03\x00\x00\x00\x00\x00\x00\x00\x04\x00\x00\x00\x00\x00\x00\x00\x05\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x05\x00\x00\x00\x01\x00\x00\x00\x09\x00\x00\x00\x08\x00\x00\x00\x03\x00\x00\x00\x00\x00\x00\x00\x04\x00\x00\x00\x00\x00\x00\x00\x05\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x0c\x00\x00\x00\xd0\x02\x00\x00\x0a\x00\x00\x00\x01\x00\x00\x00\x0c\x00\x00\x00\x08\x00\x00\x00\x09\x00\x00\x00\x03\x00\x00\x00\x03\x00\x00\x00\x08\x00\x00\x00\x2a\x2a\x00\x00\x0c\x00\x00\x00\x10\x00\x00\x00\x07\x00\x00\x00\x02\x00\x00\x00\x03\x00\x00\x00\x0c\x00\x00\x00\x39\x05\x00\x00\x0c\x00\x00\x00\x10\x00\x00\x00\x06\x00\x00\x00\x01\x00\x00\x00\x07\x00\x00\x00\x08\x00\x00\x00\x00\x00\x80\x40\x09\x00\x00\x00\x30\x00\x00\x00\x0c\x00\x00\x00\x00\x00\x00\x00\x20\x00\x00\x00\x00\x00\x00\x00\x03\x00\x00\x00\x00\x00\x00\x00\x05\x00\x00\x00\x01\x00\x00\x00\x09\x00\x00\x00\x08\x00\x00\x00\x03\x00\x00\x00\x00\x00\x00\x00\x04\x00\x00\x00\x00\x00\x00\x00\x05\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x05\x00\x00\x00\x01\x00\x00\x00\x09\x00\x00\x00\x08\x00\x00\x00\x03\x00\x00\x00\x00\x00\x00\x00\x04\x00\x00\x00\x00\x00\x00\x00\x05\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x05\x00\x00\x00\x01\x00\x00\x00\x09\x00\x00\x00\x08\x00\x00\x00\x03\x00\x00\x00\x00\x00\x00\x00\x04\x00\x00\x00\x00\x00\x00\x00\x05\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
		, binarized.size()) == 0);

	c deSerializedC;
	deserializer.DeserializeInto(binarized.data(), deSerializedC);
	REQUIRE((memcmp(&serializedC.anArray, &deSerializedC.anArray, sizeof(serializedC.anArray)) == 0
	&& memcmp(&serializedC.aMultiArray, &deSerializedC.aMultiArray, sizeof(serializedC.aMultiArray)) == 0
	&& memcmp(&serializedC.mega, &deSerializedC.mega, sizeof(serializedC.mega)) == 0
	&& memcmp(&serializedC.ultra, &deSerializedC.ultra, sizeof(serializedC.ultra)) == 0
	&& serializedC.aVector == deSerializedC.aVector
	&& serializedC.ctoto == deSerializedC.ctoto));
}


TEST_CASE("Binary std::map", "[Serialization]")
{
	dire::BinaryReflectorSerializer serializer;
	dire::BinaryReflectorDeserializer deserializer;
	std::string binarized;

	mapType serializedMap;
	for (int i = 0; i < 10; i++)
	{
		serializedMap.aEvenOddMap[i] = (i % 2 == 0);
	}
	binarized = serializer.Serialize(serializedMap);

	REQUIRE(memcmp(binarized.data(),
		"\x0e\x00\x00\x00\x01\x00\x00\x00\x0a\x00\x00\x00\x08\x00\x00\x00\x03\x00\x00\x00\x00\x00\x00\x00\x04\x00\x00\x00\x00\x00\x00\x00\x02\x00\x00\x00\x00\x00\x00\x00\x01\x00\x00\x00\x00\x00\x00\x00\x0a\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x01\x01\x00\x00\x00\x00\x02\x00\x00\x00\x01\x03\x00\x00\x00\x00\x04\x00\x00\x00\x01\x05\x00\x00\x00\x00\x06\x00\x00\x00\x01\x07\x00\x00\x00\x00\x08\x00\x00\x00\x01\x09\x00\x00\x00\x00"
		, binarized.size()) == 0);

	mapType deSerializedMap;
	deserializer.DeserializeInto(binarized.data(), deSerializedMap);
	REQUIRE(serializedMap.aEvenOddMap == deSerializedMap.aEvenOddMap);
}

TEST_CASE("Binary Serialize compound, map in map, and compound value in map", "[Serialization]")
{
	dire::BinaryReflectorSerializer serializer;
	dire::BinaryReflectorDeserializer deserializer;
	std::string binarized;

	d aD;
	aD.SetProperty<int>("ultra.mega.toto[0].titi[2]", 0x1234);

	binarized = serializer.Serialize(aD);

	REQUIRE(memcmp(binarized.data(),
		"\x10\x00\x00\x00\x10\x00\x00\x00\x02\x00\x00\x00\x08\x00\x00\x00\x00\x07\x00\x00\x00\x0c\x00\x00\x00\x00\x00\x00\x00\x08\x00\x00\x00\x28\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x0c\x00\x00\x00\x30\x00\x00\x00\x08\x00\x00\x00\x02\x00\x00\x00\x03\x00\x00\x00\x08\x00\x00\x00\x2a\x2a\x00\x00\x0c\x00\x00\x00\x10\x00\x00\x00\x07\x00\x00\x00\x02\x00\x00\x00\x03\x00\x00\x00\x0c\x00\x00\x00\x39\x05\x00\x00\x0c\x00\x00\x00\x10\x00\x00\x00\x06\x00\x00\x00\x01\x00\x00\x00\x07\x00\x00\x00\x08\x00\x00\x00\x00\x00\x80\x40\x04\x00\x00\x00\x60\x00\x00\x00\xef\xbe\xad\xde\x09\x00\x00\x00\x68\x00\x00\x00\x03\x00\x00\x00\x00\x00\x00\x00\x04\x00\x00\x00\x00\x00\x00\x00\x03\x00\x00\x00\x00\x00\x00\x00\x01\x00\x00\x00\x02\x00\x00\x00\x03\x00\x00\x00\x09\x00\x00\x00\x88\x00\x00\x00\x03\x00\x00\x00\x00\x00\x00\x00\x04\x00\x00\x00\x00\x00\x00\x00\x0a\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x09\x00\x00\x00\xb0\x00\x00\x00\x09\x00\x00\x00\x00\x00\x00\x00\x28\x00\x00\x00\x00\x00\x00\x00\x0a\x00\x00\x00\x00\x00\x00\x00\x03\x00\x00\x00\x00\x00\x00\x00\x04\x00\x00\x00\x00\x00\x00\x00\x0a\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x03\x00\x00\x00\x00\x00\x00\x00\x04\x00\x00\x00\x00\x00\x00\x00\x0a\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x03\x00\x00\x00\x00\x00\x00\x00\x04\x00\x00\x00\x00\x00\x00\x00\x0a\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x03\x00\x00\x00\x00\x00\x00\x00\x04\x00\x00\x00\x00\x00\x00\x00\x0a\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x03\x00\x00\x00\x00\x00\x00\x00\x04\x00\x00\x00\x00\x00\x00\x00\x0a\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x03\x00\x00\x00\x00\x00\x00\x00\x04\x00\x00\x00\x00\x00\x00\x00\x0a\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x03\x00\x00\x00\x00\x00\x00\x00\x04\x00\x00\x00\x00\x00\x00\x00\x0a\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x03\x00\x00\x00\x00\x00\x00\x00\x04\x00\x00\x00\x00\x00\x00\x00\x0a\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x03\x00\x00\x00\x00\x00\x00\x00\x04\x00\x00\x00\x00\x00\x00\x00\x0a\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x03\x00\x00\x00\x00\x00\x00\x00\x04\x00\x00\x00\x00\x00\x00\x00\x0a\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x0c\x00\x00\x00\x40\x02\x00\x00\x09\x00\x00\x00\x03\x00\x00\x00\x03\x00\x00\x00\x08\x00\x00\x00\x2a\x2a\x00\x00\x0c\x00\x00\x00\x10\x00\x00\x00\x07\x00\x00\x00\x02\x00\x00\x00\x03\x00\x00\x00\x0c\x00\x00\x00\x39\x05\x00\x00\x0c\x00\x00\x00\x10\x00\x00\x00\x06\x00\x00\x00\x01\x00\x00\x00\x07\x00\x00\x00\x08\x00\x00\x00\x00\x00\x80\x40\x09\x00\x00\x00\x30\x00\x00\x00\x0c\x00\x00\x00\x00\x00\x00\x00\x20\x00\x00\x00\x00\x00\x00\x00\x03\x00\x00\x00\x00\x00\x00\x00\x05\x00\x00\x00\x01\x00\x00\x00\x09\x00\x00\x00\x08\x00\x00\x00\x03\x00\x00\x00\x00\x00\x00\x00\x04\x00\x00\x00\x00\x00\x00\x00\x05\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x05\x00\x00\x00\x01\x00\x00\x00\x09\x00\x00\x00\x08\x00\x00\x00\x03\x00\x00\x00\x00\x00\x00\x00\x04\x00\x00\x00\x00\x00\x00\x00\x05\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x05\x00\x00\x00\x01\x00\x00\x00\x09\x00\x00\x00\x08\x00\x00\x00\x03\x00\x00\x00\x00\x00\x00\x00\x04\x00\x00\x00\x00\x00\x00\x00\x05\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x0c\x00\x00\x00\xd0\x02\x00\x00\x0a\x00\x00\x00\x01\x00\x00\x00\x0c\x00\x00\x00\x08\x00\x00\x00\x09\x00\x00\x00\x03\x00\x00\x00\x03\x00\x00\x00\x08\x00\x00\x00\x2a\x2a\x00\x00\x0c\x00\x00\x00\x10\x00\x00\x00\x07\x00\x00\x00\x02\x00\x00\x00\x03\x00\x00\x00\x0c\x00\x00\x00\x39\x05\x00\x00\x0c\x00\x00\x00\x10\x00\x00\x00\x06\x00\x00\x00\x01\x00\x00\x00\x07\x00\x00\x00\x08\x00\x00\x00\x00\x00\x80\x40\x09\x00\x00\x00\x30\x00\x00\x00\x0c\x00\x00\x00\x00\x00\x00\x00\x20\x00\x00\x00\x00\x00\x00\x00\x03\x00\x00\x00\x00\x00\x00\x00\x05\x00\x00\x00\x01\x00\x00\x00\x09\x00\x00\x00\x08\x00\x00\x00\x03\x00\x00\x00\x00\x00\x00\x00\x04\x00\x00\x00\x00\x00\x00\x00\x05\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x34\x12\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x05\x00\x00\x00\x01\x00\x00\x00\x09\x00\x00\x00\x08\x00\x00\x00\x03\x00\x00\x00\x00\x00\x00\x00\x04\x00\x00\x00\x00\x00\x00\x00\x05\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x05\x00\x00\x00\x01\x00\x00\x00\x09\x00\x00\x00\x08\x00\x00\x00\x03\x00\x00\x00\x00\x00\x00\x00\x04\x00\x00\x00\x00\x00\x00\x00\x05\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x0a\x00\x00\x00\x68\x03\x00\x00\x03\x00\x00\x00\x00\x00\x00\x00\x04\x00\x00\x00\x00\x00\x00\x00\x02\x00\x00\x00\x00\x00\x00\x00\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x03\x00\x00\x00\x80\x03\x00\x00\x2a\x00\x00\x00\x0a\x00\x00\x00\x88\x03\x00\x00\x03\x00\x00\x00\x00\x00\x00\x00\x04\x00\x00\x00\x00\x00\x00\x00\x02\x00\x00\x00\x00\x00\x00\x00\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x0a\x00\x00\x00\xa0\x03\x00\x00\x03\x00\x00\x00\x00\x00\x00\x00\x04\x00\x00\x00\x00\x00\x00\x00\x0c\x00\x00\x00\x00\x00\x00\x00\x20\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x0a\x00\x00\x00\xb8\x03\x00\x00\x03\x00\x00\x00\x00\x00\x00\x00\x04\x00\x00\x00\x00\x00\x00\x00\x0a\x00\x00\x00\x00\x00\x00\x00\x18\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x0c\x00\x00\x00\xd0\x03\x00\x00\x0f\x00\x00\x00\x02\x00\x00\x00\x0a\x00\x00\x00\x08\x00\x00\x00\x03\x00\x00\x00\x00\x00\x00\x00\x04\x00\x00\x00\x00\x00\x00\x00\x02\x00\x00\x00\x00\x00\x00\x00\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x0a\x00\x00\x00\x20\x00\x00\x00\x03\x00\x00\x00\x00\x00\x00\x00\x04\x00\x00\x00\x00\x00\x00\x00\x0c\x00\x00\x00\x00\x00\x00\x00\x20\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
		, binarized.size()) == 0);

	d deserializedD;
	deserializer.DeserializeInto(binarized.data(), deserializedD);
	std::string serialized2 = serializer.Serialize(deserializedD);
	REQUIRE(binarized == serialized2); // in theory, if serialization output is the same, objects are the same as far as reflection is concerned
}


TEST_CASE("Binary Serialize enumerations", "[Serialization]")
{
	dire::BinaryReflectorSerializer serializer;
	dire::BinaryReflectorDeserializer deserializer;
	std::string binarized;

	enumTestType enums;
	enums.aTestFace = Faces::Queen;
	enums.worstKings[0] = Kings::Philippe;
	enums.worstKings[1] = Kings::Charles;
	enums.playableKings = { Kings::Cesar, Kings::Alexandre };
	enums.allowedQueens = { {Queens::Judith, true}, {Queens::Rachel, false} };
	enums.pointsPerJack = { {10, Jacks::Ogier}, {20, Jacks::Lahire}, {30, Jacks::Hector}, {40, Jacks::Lancelot} };
	binarized = serializer.Serialize(enums);

	REQUIRE(Queens::GetStringFromEnum(Queens::Judith) == "Judith");
	REQUIRE(Jacks::GetStringFromEnum(Jacks::Lancelot) == "Lancelot");
	REQUIRE(BitEnum::GetStringFromEnum(BitEnum::one) == "one");
	REQUIRE((int)Queens::Judith == 1);
	REQUIRE((int)Jacks::Lancelot == 8);
	REQUIRE((int)BitEnum::eight == 8);

	REQUIRE(memcmp(binarized.data(),
		"\x13\x00\x00\x00\x06\x00\x00\x00\x0d\x00\x00\x00\x08\x00\x00\x00\x01\x0b\x00\x00\x00\x0c\x00\x00\x00\x01\x00\x00\x00\x09\x00\x00\x00\x10\x00\x00\x00\x0b\x00\x00\x00\x00\x00\x00\x00\x04\x00\x00\x00\x00\x00\x00\x00\x02\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x03\x00\x00\x00\x09\x00\x00\x00\x18\x00\x00\x00\x0b\x00\x00\x00\x00\x00\x00\x00\x04\x00\x00\x00\x00\x00\x00\x00\x02\x00\x00\x00\x00\x00\x00\x00\x02\x00\x00\x00\x01\x00\x00\x00\x0a\x00\x00\x00\x38\x00\x00\x00\x0b\x00\x00\x00\x00\x00\x00\x00\x02\x00\x00\x00\x00\x00\x00\x00\x02\x00\x00\x00\x00\x00\x00\x00\x01\x00\x00\x00\x00\x00\x00\x00\x02\x00\x00\x00\x00\x00\x00\x00\x01\x00\x01\x02\x00\x00\x0a\x00\x00\x00\x50\x00\x00\x00\x03\x00\x00\x00\x00\x00\x00\x00\x04\x00\x00\x00\x00\x00\x00\x00\x0b\x00\x00\x00\x00\x00\x00\x00\x02\x00\x00\x00\x00\x00\x00\x00\x04\x00\x00\x00\x00\x00\x00\x00\x0a\x00\x00\x00\x01\x00\x14\x00\x00\x00\x02\x00\x1e\x00\x00\x00\x04\x00\x28\x00\x00\x00\x08\x00"
		, binarized.size()) == 0);

	enumTestType deserializedEnums;
	deserializer.DeserializeInto(binarized.data(), deserializedEnums);
	REQUIRE(enums == deserializedEnums);
}
#	endif // DIRE_SERIALIZATION_BINARY_ENABLED

#endif // DIRE_SERIALIZATION_ENABLED