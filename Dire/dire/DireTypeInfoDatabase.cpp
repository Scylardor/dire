#include "DireTypeInfoDatabase.h"
#include "DireTypeInfo.h"

#include <fstream>

const DIRE_NS::TypeInfo * DIRE_NS::TypeInfoDatabase::GetTypeInfo(ReflectableID classID) const
{
	// TypeInfos IDs are supposed to match their position in the vector, but not always because of possible binary import/exports.
	// if possible, use Reflectable::GetTypeInfo instead.
	if (classID < myReflectableTypeInfos.size() && myReflectableTypeInfos[classID]->GetID() == classID)
	{
		// lucky!
		return myReflectableTypeInfos[classID];
	}
	// not so lucky :( Linear search but for any reasonable workload it should still be OK.
	auto it = std::find_if(myReflectableTypeInfos.begin(), myReflectableTypeInfos.end(), [classID](const TypeInfo* pTI) { return pTI->GetID() == classID; });
	if (it != myReflectableTypeInfos.end())
	{
		return *it;
	}

	return nullptr;
}

DIRE_NS::TypeInfo* DIRE_NS::TypeInfoDatabase::EditTypeInfo(ReflectableID classID)
{
	const TypeInfo* typeInfo = GetTypeInfo(classID);
	return const_cast<TypeInfo*>(typeInfo);
}

DIRE_NS::Reflectable2* DIRE_NS::TypeInfoDatabase::TryInstantiate(ReflectableID pClassID, std::any const& pAnyParameterPack) const
{
	ReflectableFactory::InstantiateFunction anInstantiateFunc = myInstantiateFactory.GetInstantiator(pClassID);
	if (anInstantiateFunc == nullptr)
	{
		return nullptr;
	}

	Reflectable2* newInstance = anInstantiateFunc(pAnyParameterPack);
	return newInstance;
}

size_t	BinaryWriteAtOffset(char* pDest, const void* pSrc, size_t pCount, size_t pWriteOffset)
{
	memcpy(pDest + pWriteOffset, pSrc, pCount);
	return pWriteOffset + pCount;
}

DIRE_STRING	DIRE_NS::TypeInfoDatabase::BinaryExport() const
{
	// This follows a very simple binary serialization process right now. It encodes:
	// - file version
	// - total number of encoded types
	// - the reflectable type ID
	// - the typename string
	DIRE_STRING writeBuffer;
	auto nbTypeInfos = (unsigned)myReflectableTypeInfos.size();

	const int estimatedNamesStorage = (sizeof(ReflectableID) + 64) * nbTypeInfos; // expect 64 chars or less for a typename (i.e. 63 + 1 \0)
	writeBuffer.resize(sizeof(DATABASE_VERSION) + sizeof(nbTypeInfos) + estimatedNamesStorage);
	size_t offset = 0;

	offset = BinaryWriteAtOffset(writeBuffer.data(), (const char*)&DATABASE_VERSION, sizeof(DATABASE_VERSION), offset);
	offset = BinaryWriteAtOffset(writeBuffer.data(), (const char*)&nbTypeInfos, sizeof(nbTypeInfos), offset);

	for (const TypeInfo * typeInfo : myReflectableTypeInfos)
	{
		const ReflectableID typeID = typeInfo->GetID();

		const DIRE_STRING_VIEW& typeName = typeInfo->GetName();
		const auto nameLen = (unsigned)typeName.length();

		const size_t remainingSpace = writeBuffer.size() - offset;
		const size_t neededSpace = sizeof(typeID) + nameLen + 1;
		// Make sure we have enough space
		if (remainingSpace < neededSpace) // +1 for \0, if true, make it bigger
			writeBuffer.resize(writeBuffer.size() + neededSpace);

		offset = BinaryWriteAtOffset(writeBuffer.data(), (const char*)&typeID, sizeof(typeID), offset);

		// A bit "unsafe" to copy typeName.length()+1 bytes, but here we know that the string view actually will point to a null-terminated string.
		offset = BinaryWriteAtOffset(writeBuffer.data(), typeName.data(), typeName.length()+1, offset);
	}

	// eliminate extraneous allocated memory
	writeBuffer.resize(offset);
	writeBuffer.shrink_to_fit();

	return writeBuffer;
}
bool	DIRE_NS::TypeInfoDatabase::ExportToBinaryFile(DIRE_STRING_VIEW pWrittenSettingsFile) const
{
	auto binaryBuffer = BinaryExport();

	std::ofstream openFile{ pWrittenSettingsFile.data(), std::ios::binary };
	if (openFile.bad())
	{
		return false;
	}

	openFile.write(binaryBuffer.data(), binaryBuffer.size());

	return true;
}

DIRE_STRING	DIRE_NS::TypeInfoDatabase::BinaryImport(DIRE_STRING_VIEW pReadSettingsFile)
{
	using std::ios;
	std::ifstream file(pReadSettingsFile.data(), ios::binary | ios::ate);
	if (file.bad())
		return {};
	DIRE_STRING fileBuffer;
	fileBuffer.resize(file.tellg());
	file.seekg(ios::beg);
	file.read(fileBuffer.data(), fileBuffer.size());

	return fileBuffer;
}

bool	DIRE_NS::TypeInfoDatabase::ImportFromBinaryFile(DIRE_STRING_VIEW pReadSettingsFile)
{
	// Importing settings is trickier than exporting them because all the static initialization
	// is already done at import time. This gives a lot of opportunities to mess up!
	// For example : types can have changed names, the same type can now have a different reflectable ID,
	// there can be "twin types" (types with the same names, how to differentiate them?),
	// "orphaned types" (types that were imported but are not in the executable anymore)...
	// This function tries to "patch the holes" as best as it can.

	DIRE_STRING readBuffer = BinaryImport(pReadSettingsFile);
	if (readBuffer.empty())
		return false;

	size_t offset = 0;

	int& fileVersion = *reinterpret_cast<int*>(readBuffer.data() + offset);
	offset += sizeof(fileVersion);

	if (fileVersion != DATABASE_VERSION)
	{
		return false; // reflector is not backward compatible for now.
	}

	const unsigned& nbTypeInfos = *reinterpret_cast<unsigned*>(readBuffer.data() + offset);
	offset += sizeof(nbTypeInfos);
	std::vector<ExportedTypeInfoData> theReadData(nbTypeInfos);

	unsigned iTypeInfo = 0;
	ReflectableID maxTypeInfoID = 0; // will be useful to assign new IDs to new types not in the database
	while (iTypeInfo < nbTypeInfos)
	{
		ExportedTypeInfoData& curData = theReadData[iTypeInfo];

		ReflectableID& storedReflectableID = *reinterpret_cast<ReflectableID*>(readBuffer.data() + offset);
		curData.ReflectableID = storedReflectableID;
		offset += sizeof(storedReflectableID);

		const char* typeName = readBuffer.data() + offset;
		curData.TypeName = typeName;
		offset += curData.TypeName.length() + 1; // +1 for \0

		maxTypeInfoID = std::max(maxTypeInfoID, storedReflectableID);

		iTypeInfo++;
	}

	// To assign new IDs to new types not in the database
	ReflectableID nextAvailableID = maxTypeInfoID + 1;

	for (int iReg = 0; iReg < myReflectableTypeInfos.size(); ++iReg)
	{
		TypeInfo* registeredTypeInfo = myReflectableTypeInfos[iReg];

		// Is this type in the read info ?
		auto it = std::find_if(theReadData.begin(), theReadData.end(),
			[registeredTypeInfo](const ExportedTypeInfoData& pReadTypeInfo)
			{
				return pReadTypeInfo.TypeName == registeredTypeInfo->GetName();
			});

		if (it != theReadData.end())
		{
			// we found it in the read data. Fix its ID if necessary
			if (it->ReflectableID != registeredTypeInfo->GetID())
			{
				registeredTypeInfo->SetID(it->ReflectableID);
			}
		}
		else
		{
			// it's a new type that wasn't here when the database was built.
			// It may have a "valid" ID (an ID that wasnt taken by any type in the database),
			// but it's more trouble than it's worth to check. Just assign it a new ID.
			registeredTypeInfo->SetID(nextAvailableID++);
		}
	}

	return true;
}
