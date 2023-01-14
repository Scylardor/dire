
#include "catch2/catch_test_macros.hpp"
#include "dire/DireTypeInfo.h"
#include "Dire/DireTypeInfoDatabase.h"

#include <iomanip>

#include <iostream>



/* Utility function to print the output of binary generators */
auto writeBinaryString = [](const std::string& binarized)
{
	std::cout << "\"";
	for (int i = 0; i < binarized.size(); ++i)
		std::cout << "\\x" << std::hex << std::setfill('0') << std::setw(2) << unsigned(binarized[i]);
	std::cout << "\"" << std::endl;
};

TEST_CASE("Binary Export", "[TypeInfoDatabase]")
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

	SECTION("Binary export to string")
	{
		std::string binaryDatabase = aDatabase.BinaryExport();

		REQUIRE(memcmp(binaryDatabase.data(),
			"\x00\x00\x00\x00\x05\x00\x00\x00\x00\x00\x00\x00\x74\x61\x74\x61\x00\x01\x00\x00\x00\x74\x65\x74\x65\x00\x02\x00\x00\x00\x74\x69\x74\x69\x00\x03\x00\x00\x00\x74\x6f\x74\x6f\x00\x04\x00\x00\x00\x74\x75\x74\x75\x00"
			, binaryDatabase.length()) == 0);
	}

	bool success;

	SECTION("Export to file")
	{
		success = aDatabase.ExportToBinaryFile("database.bin");
		REQUIRE(success);
	}

	SECTION("Import from file")
	{
		std::string binaryDatabase = aDatabase.BinaryExport();

		std::string fileBuffer = dire::Reflector3::BinaryImport("database.bin");
		REQUIRE(binaryDatabase == fileBuffer);
	}

	SECTION("Cleanup")
	{
		success = !std::remove("database.bin");
		CHECK(success);
	}
}

TEST_CASE("Binary Import", "[TypeInfoDatabase]")
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

	bool success = aDatabase.ExportToBinaryFile("database.bin");
	REQUIRE(success);
	// For example : types can have changed names, the same type can now have a different reflectable ID,
	// there can be "twin types" (types with the same names, how to differentiate them?),
	// "orphaned types" (types that were imported but are not in the executable anymore)...
	// This function tries to "patch the holes" as best as it can.

	SECTION("Ideal scenario: everything is as we saved it")
	{
		success = aDatabase.ImportFromBinaryFile("database.bin");
		REQUIRE(success);
		REQUIRE((tata.GetID() == 0 && tete.GetID() == 1 && titi.GetID() == 2 && toto.GetID() == 3 && tutu.GetID() == 4));
	}

	// Case study 1 : Existing types moved around
	SECTION("Case study 1 : Existing types moved around")
	{
		auto tmp = tata.GetID();
		tata.SetID(toto.GetID());
		toto.SetID(tmp);

		tmp = titi.GetID();
		titi.SetID(tutu.GetID());
		tutu.SetID(tmp);

		success = aDatabase.ImportFromBinaryFile("database.bin");
		REQUIRE(success);

		// The database is supposed to be authoritative and reset all types to their original ID.
		REQUIRE((tata.GetID() == 0 && tete.GetID() == 1 && titi.GetID() == 2 && toto.GetID() == 3 && tutu.GetID() == 4));
	}

	SECTION("Case study 2 : Types are missing")
	{
		{
			dire::Reflector3 aDatabase2;
			tete.SetID(aDatabase2.RegisterTypeInfo(&tete));

			toto.SetID(aDatabase2.RegisterTypeInfo(&toto));

			tutu.SetID(aDatabase2.RegisterTypeInfo(&tutu));

			REQUIRE((tete.GetID() == 0 && toto.GetID() == 1 && tutu.GetID() == 2));

			success = aDatabase2.ImportFromBinaryFile("database.bin");
			REQUIRE(success);

			// Types that still exist must have their original ID back.
			REQUIRE((tete.GetID() == 1 && toto.GetID() == 3 && tutu.GetID() == 4));

			// Export that
			success = aDatabase2.ExportToBinaryFile("database.bin");
			REQUIRE(success);
		}

		// Reimport again

		dire::Reflector3 aDatabase3;
		tete.SetID(aDatabase3.RegisterTypeInfo(&tete));
		toto.SetID(aDatabase3.RegisterTypeInfo(&toto));
		tutu.SetID(aDatabase3.RegisterTypeInfo(&tutu));
		REQUIRE((tete.GetID() == 0 && toto.GetID() == 1 && tutu.GetID() == 2));

		success = aDatabase3.ImportFromBinaryFile("database.bin");
		REQUIRE(success);
		REQUIRE((tete.GetID() == 1 && toto.GetID() == 3 && tutu.GetID() == 4));

	}


	// Case study 3 : New types appeared


	// Case study 4 : the "mega mix" : existing types moved around, some are missing, and new types appeared

	SECTION("Cleanup the test file")
	{
		success = !std::remove("database.bin");
		CHECK(success);
	}
}