#include "DireTypeInfoDatabase.h"
#include "DireTypeInfo.h"

#include <fstream>

const DIRE_NS::TypeInfo * DIRE_NS::Reflector3::GetTypeInfo(unsigned classID) const
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

DIRE_NS::TypeInfo* DIRE_NS::Reflector3::EditTypeInfo(unsigned classID)
{
	const TypeInfo* typeInfo = GetTypeInfo(classID);
	return const_cast<TypeInfo*>(typeInfo);
}

DIRE_NS::Reflectable2* DIRE_NS::Reflector3::TryInstantiate(unsigned pClassID, std::any const& pAnyParameterPack) const
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

DIRE_STRING	DIRE_NS::Reflector3::BinaryExport() const
{
	// This follows a very simple binary serialization process right now. It encodes:
	// - file version
	// - total number of encoded types
	// - the reflectable type ID
	// - the typename string
	DIRE_STRING writeBuffer;
	auto nbTypeInfos = (unsigned)myReflectableTypeInfos.size();
	// TODO: unsigned -> ReflectableID
	const int estimatedNamesStorage = (sizeof(unsigned) + 64) * nbTypeInfos; // expect 64 chars or less for a typename (i.e. 63 + 1 \0)
	writeBuffer.resize(sizeof(DATABASE_VERSION) + sizeof(nbTypeInfos) + estimatedNamesStorage);
	size_t offset = 0;

	offset = BinaryWriteAtOffset(writeBuffer.data(), (const char*)&DATABASE_VERSION, sizeof(DATABASE_VERSION), offset);
	offset = BinaryWriteAtOffset(writeBuffer.data(), (const char*)&nbTypeInfos, sizeof(nbTypeInfos), offset);

	for (TypeInfo const* typeInfo : myReflectableTypeInfos)
	{
		const unsigned typeID = typeInfo->GetID(); // TODO: reflectableID

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
bool	DIRE_NS::Reflector3::ExportToBinaryFile(DIRE_STRING_VIEW pWrittenSettingsFile) const
{
	auto binaryBuffer = BinaryExport();

	std::ofstream openFile{ pWrittenSettingsFile.data(), std::ios::binary };
	if (openFile.bad())
	{
		return false;
	}
	// TODO: check for exception?
	openFile.write(binaryBuffer.data(), binaryBuffer.size());

	return true;
}

DIRE_STRING	DIRE_NS::Reflector3::BinaryImport(DIRE_STRING_VIEW pReadSettingsFile)
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

bool	DIRE_NS::Reflector3::ImportFromBinaryFile(DIRE_STRING_VIEW pReadSettingsFile)
{
	// Importing settings is trickier than exporting them because all the static initialization
	// is already done at import time. This gives a lot of opportunities to mess up!
	// For example : types can have changed names, the same type can now have a different reflectable ID,
	// there can be "twin types" (types with the same names, how to differentiate them?),
	// "orphaned types" (types that were imported but are not in the executable anymore)...
	// This function tries to "patch the holes" as best as it can.

	std::string readBuffer = BinaryImport(pReadSettingsFile);
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
	while (iTypeInfo < nbTypeInfos)
	{
		ExportedTypeInfoData& curData = theReadData[iTypeInfo];

		curData.ReflectableID = iTypeInfo;

		//TODO: probably useless, to remove!
		unsigned& storedReflectableID = *reinterpret_cast<unsigned*>(readBuffer.data() + offset);
		curData.ReflectableID = storedReflectableID;
		offset += sizeof(storedReflectableID);

		const char* typeName = readBuffer.data() + offset;
		curData.TypeName = typeName;
		offset += curData.TypeName.length() + 1; // +1 for \0

		iTypeInfo++;
	}

	// Walk our registry and see if we find the imported types. Patch the reflectable type ID if necessary.
	std::vector<ExportedTypeInfoData*> orphanedTypes; // Types that were "lost" - cannot find them in the registry. Will have to "guess" if an existing type matches them or not.

	// Always resize the registry to at least the size of data we read,
	// because we absolutely have to keep the same ID for imported types.
	//if (myReflectableTypeInfos.size() < theReadData.size())
	//{
	//	myReflectableTypeInfos.resize(theReadData.size());
	//}


	for (int iReg = 0; iReg < myReflectableTypeInfos.size(); ++iReg)
	{
		TypeInfo* registeredTypeInfo = myReflectableTypeInfos[iReg];

		//if (registeredTypeInfo == nullptr) // possible due to the resize
		//{
		//	continue;
		//}

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
				//// Registered type info should be in sequential order, by ID. If this type info is not at the right place, "switch" it.
				//if (iReg != registeredTypeInfo->GetID())
				//{
				//	std::iter_swap(myReflectableTypeInfos.begin() + iReg, myReflectableTypeInfos.begin() + registeredTypeInfo->GetID());
				//	iReg--; // loop over the same item, as there was a swap
				//}
			}
		}
	}

	// erase-remove all the null pointers.
	auto isNull = [](TypeInfo* pInfo) { return pInfo == nullptr; };
	myReflectableTypeInfos.erase(std::remove_if(myReflectableTypeInfos.begin(), myReflectableTypeInfos.end(), isNull), myReflectableTypeInfos.end());

	return true;

	for (int iReg = 0; iReg < myReflectableTypeInfos.size(); ++iReg)
	{
		auto& registeredTypeInfo = myReflectableTypeInfos[iReg];

		if (iReg >= theReadData.size())
		{
			// We reached the end of imported types : all the next ones are new types
			break;
		}

		auto& readTypeInfo = theReadData[iReg];

		// There can be null type infos due to a previous resize if there are more imported types than registered types.
		if (registeredTypeInfo == nullptr || readTypeInfo.TypeName != registeredTypeInfo->GetName())
		{
			// mismatch: try to find the read typeinfo in the registered data
			auto it = std::find_if(myReflectableTypeInfos.begin(), myReflectableTypeInfos.end(),
				[readTypeInfo](TypeInfo const* pTypeInfo)
				{
					return pTypeInfo != nullptr && pTypeInfo->GetName() == readTypeInfo.TypeName;
				});
			if (it != myReflectableTypeInfos.end())
			{
				std::iter_swap(myReflectableTypeInfos.begin() + iReg, it);
				myReflectableTypeInfos[iReg]->SetID(readTypeInfo.ReflectableID);
			}
			else
			{
				orphanedTypes.push_back(&readTypeInfo);
				if (registeredTypeInfo != nullptr)
				{
					// If we are here, the registered type is new one (not found in imported types),
					// so push it back to fix its reflectable ID later. We should let the not found type block "empty".
					myReflectableTypeInfos.push_back(registeredTypeInfo);
					registeredTypeInfo = nullptr;
				}
			}
		}
		else if (readTypeInfo.ReflectableID != registeredTypeInfo->GetID())
		{
			registeredTypeInfo->SetID(readTypeInfo.ReflectableID);
		}
	}

	// In case of new types - make sure their IDs are correct, otherwise fix them
	for (unsigned iReg = (unsigned)theReadData.size(); iReg < myReflectableTypeInfos.size(); ++iReg)
	{
		auto& registeredTypeInfo = myReflectableTypeInfos[iReg];
		if (registeredTypeInfo != nullptr && registeredTypeInfo->GetID() != iReg)
		{
			registeredTypeInfo->SetID(iReg);
		}
	}

	return true;
}
