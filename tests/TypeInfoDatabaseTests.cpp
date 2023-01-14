
#include "catch2/catch_test_macros.hpp"
#include "dire/DireTypeInfo.h"
#include "Dire/DireTypeInfoDatabase.h"

#include <iomanip>

#include <iostream>
#include <fstream>


/* Utility function to print the output of binary generators */
auto writeBinaryString = [](const std::string& binarized)
{
	std::cout << "\"";
	for (int i = 0; i < binarized.size(); ++i)
		std::cout << "\\x" << std::hex << std::setfill('0') << std::setw(2) << unsigned(binarized[i]);
	std::cout << "\"" << std::endl;
};

TEST_CASE("ExportTypeInfoSettings", "[TypeInfoDatabase]")
{
	dire::Reflector3 aDatabase;

	dire::TypeInfo tata("tata");
	tata.SetID(aDatabase.RegisterTypeInfo(&tata));

	dire::TypeInfo tete("tete");
	tete.SetID(aDatabase.RegisterTypeInfo(&tete));

	dire::TypeInfo titi("titi");
	titi.SetID(aDatabase.RegisterTypeInfo(&titi));

	dire::TypeInfo toto("toto");
	toto.SetID(aDatabase.RegisterTypeInfo(&toto));

	dire::TypeInfo tutu("tutu");
	tutu.SetID(aDatabase.RegisterTypeInfo(&tutu));

	REQUIRE((tata.GetID() == 0 && tete.GetID() == 1 && titi.GetID() == 2 && toto.GetID() == 3 && tutu.GetID() == 4));

	std::string binaryDatabase = aDatabase.BinaryExport();
	REQUIRE(memcmp(binaryDatabase.data(),
		"\x00\x00\x00\x00\x05\x00\x00\x00\x00\x00\x00\x00\x74\x61\x74\x61\x00\x01\x00\x00\x00\x74\x65\x74\x65\x00\x02\x00\x00\x00\x74\x69\x74\x69\x00\x03\x00\x00\x00\x74\x6f\x74\x6f\x00\x04\x00\x00\x00\x74\x75\x74\x75\x00"
		, binaryDatabase.length()) == 0);

	bool success = aDatabase.ExportToBinaryFile("database.bin");
	REQUIRE(success);

	{
		using std::ifstream;
		std::ifstream check("database.bin", ifstream::binary | ifstream::ate);
		REQUIRE(check.good());

		auto fileSize = check.tellg();
		std::string fileBuffer;
		fileBuffer.resize(fileSize);
		check.seekg(ifstream::beg);
		check.read(fileBuffer.data(), fileBuffer.size());
		REQUIRE(binaryDatabase == fileBuffer);
	}

	success = !std::remove("database.bin");
	REQUIRE(success);
}

TEST_CASE("ImportTypeInfoSettings", "[TypeInfoDatabase]")
{
	// For example : types can have changed names, the same type can now have a different reflectable ID,
	// there can be "twin types" (types with the same names, how to differentiate them?),
	// "orphaned types" (types that were imported but are not in the executable anymore)...
	// This function tries to "patch the holes" as best as it can.

}