#include "DireTypeInfoDatabase.h"
#include "DireTypeInfo.h"

#include <fstream>

bool	DIRE_NS::Reflector3::ExportTypeInfoSettings(DIRE_STRING_VIEW pWrittenSettingsFile) const
{
	// This follows a very simple binary serialization process right now. It encodes:
// - file version
// - total number of encoded types
// - the reflectable type ID
// - the length of typename string
// - the typename string
	std::string writeBuffer;
	auto nbTypeInfos = (unsigned)myReflectableTypeInfos.size();
	// TODO: unsigned -> ReflectableID
	writeBuffer.resize(sizeof(REFLECTOR_VERSION) + sizeof(nbTypeInfos) + sizeof(size_t) * myReflectableTypeInfos.size());
	size_t offset = 0;

	// TODO: it's possible to compress the data even more if we're willing to make the logic more complex:
	// - use a unsigned int instead of size_t (4 billions reflectable IDs are probably enough)
	// - adding a size_t to flag each string length allows us to avoid doing Strlen's to figure out the length of each string.
	//		But it's probably not worth it as it stores 8 bytes for a size_t, and the type names are usually short (less than 32 chars).
	//		It then would make sense to stop storing the length and store a single '\0' character at the end of each name instead.
	//		It needs us to perform 1 strlen per typename but since these are supposed to be short it should be ok and allows us to save on memory.

	memcpy(writeBuffer.data() + offset, (const char*)&REFLECTOR_VERSION, sizeof(REFLECTOR_VERSION));
	offset += sizeof(REFLECTOR_VERSION);

	memcpy(writeBuffer.data() + offset, (const char*)&nbTypeInfos, sizeof(nbTypeInfos));
	offset += sizeof(nbTypeInfos);

	for (TypeInfo const* typeInfo : myReflectableTypeInfos)
	{
		const DIRE_STRING_VIEW& typeName = typeInfo->GetName();
		auto nameLen = (unsigned)typeName.length();
		memcpy(writeBuffer.data() + offset, (const char*)&nameLen, sizeof(nameLen));
		offset += sizeof(nameLen);

		writeBuffer.resize(writeBuffer.size() + typeName.length());
		memcpy(writeBuffer.data() + offset, typeName.data(), typeName.length());
		offset += typeName.length();
	}

	std::ofstream openFile{ pWrittenSettingsFile.data(), std::ios::binary };
	if (openFile.bad())
	{
		return false;
	}

	// TODO: check for exception?
	openFile.write(writeBuffer.data(), writeBuffer.size());
	return true;
}

bool	DIRE_NS::Reflector3::ImportTypeInfoSettings(DIRE_STRING_VIEW pReadSettingsFile)
{
	// Importing settings is trickier than exporting them because all the static initialization
// is already done at import time. This gives a lot of opportunities to mess up!
// For example : types can have changed names, the same type can now have a different reflectable ID,
// there can be "twin types" (types with the same names, how to differentiate them?),
// "orphaned types" (types that were imported but are not in the executable anymore)...
// This function tries to "patch the holes" as best as it can.

	std::string readBuffer;
	{
		std::ifstream openFile(pReadSettingsFile.data(), std::ios::binary);
		if (openFile.bad())
		{
			return false;
		}

		openFile.seekg(0, std::ios::end);
		readBuffer.resize(openFile.tellg());
		openFile.seekg(0, std::ios::beg);
		openFile.read(readBuffer.data(), readBuffer.size());
	}

	size_t offset = 0;

	int& fileVersion = *reinterpret_cast<int*>(readBuffer.data() + offset);
	offset += sizeof(fileVersion);

	if (fileVersion != REFLECTOR_VERSION)
	{
		return false; // Let's assume that reflector is not backward compatible for now.
	}

	const unsigned& nbTypeInfos = *reinterpret_cast<unsigned*>(readBuffer.data() + offset);
	offset += sizeof(nbTypeInfos);
	std::vector<ExportedTypeInfoData> theReadData(nbTypeInfos);

	unsigned iTypeInfo = 0;
	while (iTypeInfo < nbTypeInfos)
	{
		ExportedTypeInfoData& curData = theReadData[iTypeInfo];

		curData.ReflectableID = iTypeInfo;

		unsigned& typeNameLen = *reinterpret_cast<unsigned*>(readBuffer.data() + offset);
		offset += sizeof(typeNameLen);

		curData.TypeName.resize(typeNameLen);
		memcpy(curData.TypeName.data(), readBuffer.data() + offset, typeNameLen);
		offset += typeNameLen;

		iTypeInfo++;
	}

	// Walk our registry and see if we find the imported types. Patch the reflectable type ID if necessary.
	std::vector<ExportedTypeInfoData*> orphanedTypes; // Types that were "lost" - cannot find them in the registry. Will have to "guess" if an existing type matches them or not.

	// Always resize the registry to at least the size of data we read,
	// because we absolutely have to keep the same ID for imported types.
	if (myReflectableTypeInfos.size() < theReadData.size())
	{
		myReflectableTypeInfos.resize(theReadData.size());
	}

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