#include "DireBinarySerializer.h"

#ifdef DIRE_COMPILE_BINARY_SERIALIZATION

#include "DireBinaryHeaders.h"

#define BINARY_SERIALIZE_VALUE_CASE(TypeEnum) \
case MetaType::TypeEnum:\
{\
	using ActualType = FromEnumTypeToActualType<MetaType::TypeEnum>::ActualType;\
	WriteAsBytes<ActualType>(*static_cast<const ActualType*>(pPropPtr)); \
	break;\
}

namespace DIRE_NS
{
	ISerializer::Result BinaryReflectorSerializer::Serialize(const Reflectable & serializedObject)
	{
		// write the header last because we don't know in advance the total number of props
		auto objectHandle = WriteAsBytes<BinarySerializationHeaders::Object>(serializedObject.GetReflectableClassID());

		const std::byte * reflectableAddr = reinterpret_cast<const std::byte *>(&serializedObject);

		TypeInfoDatabase::GetSingleton().GetTypeInfo(serializedObject.GetReflectableClassID())->ForEachPropertyInHierarchy([this, &objectHandle, reflectableAddr](const PropertyTypeInfo & pProperty)
		{
			const std::byte * propertyAddr = reflectableAddr + pProperty.GetOffset();

			// Uniquely identify props by their offset. Not "change proof", will need a reconcile method it case something changed location (TODO)
			WriteAsBytes<BinarySerializationHeaders::Property>(pProperty.GetMetatype(), (uint32_t)pProperty.GetOffset());
			this->SerializeValue(pProperty.GetMetatype(), propertyAddr, &pProperty.GetDataStructureHandler());

			// Dont forget to count properties
			objectHandle.Edit().PropertiesCount++;
		});

		return Result(std::move(mySerializedBuffer));
	}

	void BinaryReflectorSerializer::SerializeValue(MetaType pPropType, const void * pPropPtr, const DataStructureHandler * pHandler)
	{
		switch (pPropType.Value)
		{
			BINARY_SERIALIZE_VALUE_CASE(Bool)
			BINARY_SERIALIZE_VALUE_CASE(Char)
			BINARY_SERIALIZE_VALUE_CASE(UChar)
			BINARY_SERIALIZE_VALUE_CASE(Short)
			BINARY_SERIALIZE_VALUE_CASE(UShort)
			BINARY_SERIALIZE_VALUE_CASE(Int)
			BINARY_SERIALIZE_VALUE_CASE(Uint)
			BINARY_SERIALIZE_VALUE_CASE(Int64)
			BINARY_SERIALIZE_VALUE_CASE(Uint64)
			BINARY_SERIALIZE_VALUE_CASE(Float)
			BINARY_SERIALIZE_VALUE_CASE(Double)
		case MetaType::Array:
			if (pHandler != nullptr)
			{
				SerializeArrayValue(pPropPtr, pHandler->GetArrayHandler());
			}
			break;
		case MetaType::Map:
			if (pHandler != nullptr)
			{
				SerializeMapValue(pPropPtr, pHandler->GetMapHandler());
			}
			break;
		case MetaType::Object:
			SerializeCompoundValue(pPropPtr);
			break;
		case MetaType::Enum:
		{
			MetaType underlyingType = pHandler->GetEnumHandler()->EnumMetaType();
			SerializeValue(underlyingType, pPropPtr);
		}
		break;
		default:
			//Unmanaged type in SerializeValue!
			DIRE_ASSERT(false);
		}
	}

	void BinaryReflectorSerializer::SerializeArrayValue(const void * pPropPtr, const IArrayDataStructureHandler * pArrayHandler)
	{
		if (pArrayHandler == nullptr)
			return;

		MetaType elemType = pArrayHandler->ElementType();

		if (elemType != MetaType::Unknown)
		{
			size_t arraySize = pArrayHandler->Size(pPropPtr);
			size_t elemSize = pArrayHandler->ElementSize();
			WriteAsBytes<BinarySerializationHeaders::Array>(elemType, elemSize, arraySize);
			DataStructureHandler elemHandler = pArrayHandler->ElementHandler();

			for (int iElem = 0; iElem < arraySize; ++iElem)
			{
				void const* elemVal = pArrayHandler->Read(pPropPtr, iElem);
				SerializeValue(elemType, elemVal, &elemHandler);
			}
		}
	}

	void BinaryReflectorSerializer::SerializeMapValue(const void * pPropPtr, const IMapDataStructureHandler * pMapHandler)
	{
		if (pPropPtr == nullptr || pMapHandler == nullptr)
			return;

		const MetaType keyType = pMapHandler->KeyMetaType();
		const MetaType valueType = pMapHandler->ValueMetaType();
		const size_t mapSize = pMapHandler->Size(pPropPtr);
		const size_t keySize = pMapHandler->SizeofKey();
		const size_t valueSize = pMapHandler->SizeofValue();
		WriteAsBytes<BinarySerializationHeaders::Map>(keyType, keySize, valueType, valueSize, mapSize);

		pMapHandler->SerializeForEachPair(pPropPtr, this, [](void* pSerializer, const void* pKey, const void* pVal, IMapDataStructureHandler const& pMapHandler,
			DataStructureHandler const& pKeyHandler, DataStructureHandler const& pValueHandler)
		{
			auto* myself = static_cast<BinaryReflectorSerializer*>(pSerializer);

			const MetaType keyType = pMapHandler.KeyMetaType();
			myself->SerializeValue(keyType, pKey, &pKeyHandler);

			MetaType valueType = pMapHandler.ValueMetaType();
			myself->SerializeValue(valueType, pVal, &pValueHandler);
		});
	}

	void BinaryReflectorSerializer::SerializeCompoundValue(const void * pPropPtr)
	{
		auto* reflectableProp = static_cast<const Reflectable *>(pPropPtr);
		const TypeInfo * compTypeInfo = TypeInfoDatabase::GetSingleton().GetTypeInfo(reflectableProp->GetReflectableClassID());
		DIRE_ASSERT(compTypeInfo != nullptr);

		const std::byte * reflectableAddr = reinterpret_cast<const std::byte *>(reflectableProp);

		auto objectHandle = WriteAsBytes<BinarySerializationHeaders::Object>(reflectableProp->GetReflectableClassID());
		compTypeInfo->ForEachPropertyInHierarchy([&](const PropertyTypeInfo & pProperty)
		{
			const std::byte * propertyAddr = reflectableAddr + pProperty.GetOffset();

			// Uniquely identify props by their offset. TODO: Not "change proof", will need a reconcile method it case something changed location
			WriteAsBytes<BinarySerializationHeaders::Property>(pProperty.GetMetatype(), (uint32_t)pProperty.GetOffset());
			this->SerializeValue(pProperty.GetMetatype(), propertyAddr, &pProperty.GetDataStructureHandler());

			// Dont forget to count properties
			objectHandle.Edit().PropertiesCount++;
		});
	}


	void BinaryReflectorSerializer::SerializeString(DIRE_STRING_VIEW pSerializedString)
	{
		WriteAsBytes<size_t>(pSerializedString.size());
		WriteRawBytes(pSerializedString.data(), pSerializedString.size());
	}

	void BinaryReflectorSerializer::SerializeInt(int32_t pSerializedInt)
	{
		WriteAsBytes<int32_t>(pSerializedInt);
	}

	void BinaryReflectorSerializer::SerializeFloat(float pSerializedFloat)
	{
		WriteAsBytes<float>(pSerializedFloat);
	}

	void BinaryReflectorSerializer::SerializeBool(bool pSerializedBool)
	{
		WriteAsBytes<bool>(pSerializedBool);
	}

	void BinaryReflectorSerializer::SerializeValuesForObject(DIRE_STRING_VIEW /*pObjectName*/, SerializedValueFiller /*pFillerFunction*/)
	{
		// not implemented for now (mostly used for JSON metadata)
	}

}
#endif