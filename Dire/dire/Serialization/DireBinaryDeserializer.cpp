#include "DireBinaryDeserializer.h"
#ifdef DIRE_COMPILE_BINARY_SERIALIZATION

#include "dire/DireReflectable.h"
#include "dire/Types/DireTypeInfoDatabase.h"
#include "DireBinaryHeaders.h"

#define BINARY_DESERIALIZE_VALUE_CASE(TypeEnum) \
case MetaType::TypeEnum:\
{\
	using ActualType = FromEnumTypeToActualType<MetaType::TypeEnum>::ActualType;\
	*static_cast<ActualType*>(pPropPtr) = ReadFromBytes<ActualType>();\
	break;\
}

namespace DIRE_NS
{
	IDeserializer::Result BinaryReflectorDeserializer::DeserializeInto(const char * pSerialized, Reflectable& pDeserializedObject)
	{
		if (pSerialized == nullptr)
			return {"The binary string is nullptr."};

		mySerializedBytes = pSerialized;
		myReadingOffset = 0;

		// should start with a header...
		auto& header = ReadFromBytes<BinarySerializationHeaders::Object>();
		if (header.PropertiesCount == 0)
			return &pDeserializedObject; // just an empty object. Doesn't count like an error I guess?

		const TypeInfo* deserializedTypeInfo = TypeInfoDatabase::GetSingleton().GetTypeInfo(header.ID);
		const TypeInfo* objTypeInfo = TypeInfoDatabase::GetSingleton().GetTypeInfo(pDeserializedObject.GetReflectableClassID());

		// cannot dump properties into incompatible reflectable type
		if (!deserializedTypeInfo->IsParentOf(objTypeInfo->GetID()))
			return { "The serialized data is incompatible with the reflectable to be deserialized into." };

		const auto* nextPropertyHeader = &ReadFromBytes<BinarySerializationHeaders::Property>();

		char* objectPtr = reinterpret_cast<char*>(&pDeserializedObject);
		unsigned iProp = 0; // cppcheck-suppress variableScope

		objTypeInfo->ForEachPropertyInHierarchy([&](const PropertyTypeInfo& pProperty)
		{
			// In theory, property will come in ascending order of offset so we should not be missing any.
			if (pProperty.GetOffset() == nextPropertyHeader->PropertyOffset)
			{
				void* propPtr = objectPtr + pProperty.GetOffset();
				DeserializeValue(nextPropertyHeader->PropertyType, propPtr, &pProperty.GetDataStructureHandler());

				iProp++;
				if (iProp < header.PropertiesCount)
				{
					// Only read a following property header if we're sure there are more properties coming
					nextPropertyHeader = &ReadFromBytes<BinarySerializationHeaders::Property>();
				}
			}
		});

		return &pDeserializedObject;
	}


	void BinaryReflectorDeserializer::DeserializeArrayValue(void* pPropPtr, const IArrayDataStructureHandler * pArrayHandler) const
	{
		if (pArrayHandler != nullptr)
		{
			DataStructureHandler elemHandler = pArrayHandler->ElementHandler();
			auto& arrayHeader = ReadFromBytes<BinarySerializationHeaders::Array>();

			DIRE_ASSERT(pArrayHandler->ElementType() == arrayHeader.ElementType
			&& pArrayHandler->ElementSize() == arrayHeader.SizeofElement);
			if (arrayHeader.ElementType != MetaType::Unknown)
			{
				for (size_t iElem = 0; iElem < arrayHeader.ArraySize; ++iElem)
				{
					void* elemVal = const_cast<void*>(pArrayHandler->Read(pPropPtr, iElem));
					DeserializeValue(arrayHeader.ElementType, elemVal, &elemHandler);
				}
			}
		}
	}

	void BinaryReflectorDeserializer::DeserializeMapValue(void* pPropPtr, const IMapDataStructureHandler * pMapHandler) const
	{
		if (pPropPtr == nullptr || pMapHandler == nullptr)
			return;

		const DataStructureHandler valueHandler = pMapHandler->ValueDataHandler();
		const MetaType valueType = pMapHandler->ValueMetaType();
		const size_t sizeofKeyType = pMapHandler->SizeofKey();

		auto& mapHeader = ReadFromBytes<BinarySerializationHeaders::Map>();

		DIRE_ASSERT(valueType == mapHeader.ValueType && mapHeader.SizeofValueType == pMapHandler->SizeofValue()
			&& mapHeader.KeyType == pMapHandler->KeyMetaType() && mapHeader.SizeofKeyType == sizeofKeyType);

		for (size_t i = 0; i < mapHeader.MapSize; ++i)
		{
			const char* keyData = ReadBytes(sizeofKeyType);
			void* createdValue = pMapHandler->BinaryCreate(pPropPtr, keyData, nullptr);

			if (createdValue != nullptr)
			{
				DeserializeValue(valueType, createdValue, &valueHandler);
			}
		}
	}


	void BinaryReflectorDeserializer::DeserializeCompoundValue(void* pPropPtr) const
	{
		if (pPropPtr == nullptr)
			return;

		// should start with a header...
		const auto& header = ReadFromBytes<BinarySerializationHeaders::Object>();
		if (header.PropertiesCount == 0)
			return;

		Reflectable& reflectable = *static_cast<Reflectable*>(pPropPtr);
		const TypeInfo* objTypeInfo = TypeInfoDatabase::GetSingleton().GetTypeInfo(reflectable.GetReflectableClassID());

		const TypeInfo* deserializedTypeInfo = TypeInfoDatabase::GetSingleton().GetTypeInfo(header.ID);

		// cannot dump properties into incompatible reflectable type
		if (!deserializedTypeInfo->IsParentOf(objTypeInfo->GetID()))
			return;

		char* objectPtr = reinterpret_cast<char*>(pPropPtr);

		const auto* nextPropertyHeader = &ReadFromBytes<BinarySerializationHeaders::Property>();
		unsigned iProp = 0; // cppcheck-suppress variableScope

		deserializedTypeInfo->ForEachPropertyInHierarchy([&](const PropertyTypeInfo& pProperty)
		{
			// In theory, property will come in ascending order of offset so we should not be missing any.
			if (pProperty.GetOffset() == nextPropertyHeader->PropertyOffset)
			{
				void* propPtr = objectPtr + pProperty.GetOffset();
				DeserializeValue(nextPropertyHeader->PropertyType, propPtr, &pProperty.GetDataStructureHandler());
				iProp++;
				if (iProp < header.PropertiesCount)
				{
					// Only read a following property header if we're sure there are more properties coming
					nextPropertyHeader = &ReadFromBytes<BinarySerializationHeaders::Property>();
				}
			}
		});
	}

	void	BinaryReflectorDeserializer::DeserializeValue(MetaType pPropType, void* pPropPtr, const DataStructureHandler* pHandler) const
	{
		switch (pPropType.Value)
		{
			BINARY_DESERIALIZE_VALUE_CASE(Bool)
			BINARY_DESERIALIZE_VALUE_CASE(Char)
			BINARY_DESERIALIZE_VALUE_CASE(UChar)
			BINARY_DESERIALIZE_VALUE_CASE(Short)
			BINARY_DESERIALIZE_VALUE_CASE(UShort)
			BINARY_DESERIALIZE_VALUE_CASE(Int)
			BINARY_DESERIALIZE_VALUE_CASE(Uint)
			BINARY_DESERIALIZE_VALUE_CASE(Int64)
			BINARY_DESERIALIZE_VALUE_CASE(Uint64)
			BINARY_DESERIALIZE_VALUE_CASE(Float)
			BINARY_DESERIALIZE_VALUE_CASE(Double)
		case MetaType::Array:
			if (pHandler != nullptr)
			{
				DeserializeArrayValue(pPropPtr, pHandler->GetArrayHandler());
			}
			break;
		case MetaType::Map:
			if (pHandler != nullptr)
			{
				DeserializeMapValue(pPropPtr, pHandler->GetMapHandler());
			}
			break;
		case MetaType::Object:
			DeserializeCompoundValue(pPropPtr);
			break;
		case MetaType::Enum:
		{
			MetaType underlyingType = pHandler->GetEnumHandler()->EnumMetaType();
			DeserializeValue(underlyingType, pPropPtr);
		}
		break;
		default:
			// Unmanaged type in DeserializeValue!
			DIRE_ASSERT(false);
		}
	}
}
#endif
