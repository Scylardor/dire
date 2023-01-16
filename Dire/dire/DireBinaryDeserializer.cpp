#if DIRE_USE_SERIALIZATION && DIRE_USE_BINARY_SERIALIZATION
#include "DireBinaryDeserializer.h"
#include "DireReflectable.h"
#include "DireTypeInfoDatabase.h"
#include "BinaryHeaders.h"
#include "DireAssert.h"

#define BINARY_DESERIALIZE_VALUE_CASE(TypeEnum) \
case Type::TypeEnum:\
{\
	using ActualType = FromEnumTypeToActualType<Type::TypeEnum>::ActualType;\
	*static_cast<ActualType*>(pPropPtr) = ReadFromBytes<ActualType>();\
	break;\
}

namespace DIRE_NS
{

	void BinaryReflectorDeserializer::DeserializeInto(const char * pSerialized, Reflectable2& pDeserializedObject)
	{
		if (pSerialized == nullptr)
			return;

		mySerializedBytes = pSerialized;
		myReadingOffset = 0;

		// should start with a header...
		auto& header = ReadFromBytes<BinarySerializationHeaders::Object>();
		if (header.PropertiesCount == 0)
			return;

		const TypeInfo* deserializedTypeInfo = Reflector3::GetSingleton().GetTypeInfo(header.ReflectableID);
		const TypeInfo* objTypeInfo = Reflector3::GetSingleton().GetTypeInfo(pDeserializedObject.GetReflectableClassID());

		// cannot dump properties into incompatible reflectable type
		if (!deserializedTypeInfo->IsParentOf(objTypeInfo->GetID()))
			return;

		const auto* nextPropertyHeader = &ReadFromBytes<BinarySerializationHeaders::Property>();

		char* objectPtr = (char*)&pDeserializedObject;
		unsigned iProp = 0;

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
	}

	void BinaryReflectorDeserializer::DeserializeArrayValue(void* pPropPtr, const ArrayDataStructureHandler * pArrayHandler) const
	{
		if (pArrayHandler != nullptr)
		{
			DataStructureHandler elemHandler = pArrayHandler->ElementHandler();
			auto& arrayHeader = ReadFromBytes<BinarySerializationHeaders::Array>();

			DIRE_ASSERT(pArrayHandler->ElementType() == arrayHeader.ElementType
			&& pArrayHandler->ElementSize() == arrayHeader.SizeofElement);
			if (arrayHeader.ElementType != Type::Unknown)
			{
				for (auto iElem = 0; iElem < arrayHeader.ArraySize; ++iElem)
				{
					void* elemVal = const_cast<void*>(pArrayHandler->Read(pPropPtr, iElem));
					DeserializeValue(arrayHeader.ElementType, elemVal, &elemHandler);
				}
			}
		}
	}

	void BinaryReflectorDeserializer::DeserializeMapValue(void* pPropPtr, const MapDataStructureHandler * pMapHandler) const
	{
		if (pPropPtr == nullptr || pMapHandler == nullptr)
			return;

		const DataStructureHandler valueHandler = pMapHandler->ValueHandler();
		const Type valueType = pMapHandler->GetValueType();
		const size_t sizeofKeyType = pMapHandler->SizeofKey();

		auto& mapHeader = ReadFromBytes<BinarySerializationHeaders::Map>();

		DIRE_ASSERT(valueType == mapHeader.ValueType && mapHeader.SizeofValueType == pMapHandler->SizeofValue()
			&& mapHeader.KeyType == pMapHandler->GetKeyType() && mapHeader.SizeofKeyType == sizeofKeyType);

		for (int i = 0; i < mapHeader.MapSize; ++i)
		{
			const char* keyData = ReadBytes(sizeofKeyType);
			void* createdValue = pMapHandler->BinaryKeyCreate(pPropPtr, keyData, nullptr);

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

		Reflectable2& reflectable = *static_cast<Reflectable2*>(pPropPtr);
		const TypeInfo* objTypeInfo = Reflector3::GetSingleton().GetTypeInfo(reflectable.GetReflectableClassID());

		const TypeInfo* deserializedTypeInfo = Reflector3::GetSingleton().GetTypeInfo(header.ReflectableID);

		// cannot dump properties into incompatible reflectable type
		if (!deserializedTypeInfo->IsParentOf(objTypeInfo->GetID()))
			return;

		char* objectPtr = (char*)pPropPtr;

		const auto* nextPropertyHeader = &ReadFromBytes<BinarySerializationHeaders::Property>();
		unsigned iProp = 0;

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

	void	BinaryReflectorDeserializer::DeserializeValue(Type pPropType, void* pPropPtr, const DataStructureHandler* pHandler) const
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
		case Type::Array:
			if (pHandler != nullptr)
			{
				DeserializeArrayValue(pPropPtr, pHandler->GetArrayHandler());
			}
			break;
		case Type::Map:
			if (pHandler != nullptr)
			{
				DeserializeMapValue(pPropPtr, pHandler->GetMapHandler());
			}
			break;
		case Type::Object:
			DeserializeCompoundValue(pPropPtr);
			break;
		case Type::Enum:
		{
			Type underlyingType = pHandler->GetEnumHandler()->EnumType();
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