#if DIRE_USE_SERIALIZATION && DIRE_USE_BINARY_SERIALIZATION
#include "DireBinarySerializer.h"
#include "BinaryHeaders.h"
#include "DireAssert.h"

#define BINARY_SERIALIZE_VALUE_CASE(TypeEnum) \
case Type::TypeEnum:\
{\
	using ActualType = FromEnumTypeToActualType<Type::TypeEnum>::ActualType;\
	WriteAsBytes<ActualType>(*static_cast<const ActualType*>(pPropPtr)); \
	break;\
}

namespace DIRE_NS
{
	ISerializer::Result BinaryReflectorSerializer::Serialize(const Reflectable2 & serializedObject)
	{
		// write the header last because we don't know in advance the total number of props
		auto objectHandle = WriteAsBytes<BinarySerializationHeaders::Object>(serializedObject.GetReflectableClassID());

		// Account for the vtable pointer offset in case our type is polymorphic (aka virtual)
		const std::byte * reflectableAddr = reinterpret_cast<const std::byte *>(&serializedObject);

		Reflector3::GetSingleton().GetTypeInfo(serializedObject.GetReflectableClassID())->ForEachPropertyInHierarchy([this, &objectHandle, reflectableAddr](const PropertyTypeInfo & pProperty)
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

	void BinaryReflectorSerializer::SerializeValue(Type pPropType, const void * pPropPtr, const DataStructureHandler * pHandler)
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
		case Type::Array:
			if (pHandler != nullptr)
			{
				SerializeArrayValue(pPropPtr, pHandler->GetArrayHandler());
			}
			break;
		case Type::Map:
			if (pHandler != nullptr)
			{
				SerializeMapValue(pPropPtr, pHandler->GetMapHandler());
			}
			break;
		case Type::Object:
			SerializeCompoundValue(pPropPtr);
			break;
		case Type::Enum:
		{
			Type underlyingType = pHandler->GetEnumHandler()->EnumType();
			SerializeValue(underlyingType, pPropPtr);
		}
		break;
		default:
			//Unmanaged type in SerializeValue!
			DIRE_ASSERT(false);
		}
	}

	void BinaryReflectorSerializer::SerializeArrayValue(const void * pPropPtr, const ArrayDataStructureHandler * pArrayHandler)
	{
		if (pArrayHandler == nullptr)
			return;

		Type elemType = pArrayHandler->ElementType();

		if (elemType != Type::Unknown)
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

	void BinaryReflectorSerializer::SerializeMapValue(const void * pPropPtr, const MapDataStructureHandler * pMapHandler)
	{
		if (pPropPtr == nullptr || pMapHandler == nullptr)
			return;

		const Type keyType = pMapHandler->GetKeyType();
		const Type valueType = pMapHandler->GetValueType();
		const size_t mapSize = pMapHandler->Size(pPropPtr);
		const size_t keySize = pMapHandler->SizeofKey();
		const size_t valueSize = pMapHandler->SizeofValue();
		WriteAsBytes<BinarySerializationHeaders::Map>(keyType, keySize, valueType, valueSize, mapSize);

		pMapHandler->SerializeForEachPair(pPropPtr, this, [](void* pSerializer, const void* pKey, const void* pVal, MapDataStructureHandler const& pMapHandler,
			DataStructureHandler const& pKeyHandler, DataStructureHandler const& pValueHandler)
		{
			auto* myself = static_cast<BinaryReflectorSerializer*>(pSerializer);

			const Type keyType = pMapHandler.GetKeyType();
			myself->SerializeValue(keyType, pKey, &pKeyHandler);

			Type valueType = pMapHandler.GetValueType();
			myself->SerializeValue(valueType, pVal, &pValueHandler);
		});
	}

	void BinaryReflectorSerializer::SerializeCompoundValue(const void * pPropPtr)
	{
		auto* reflectableProp = static_cast<const Reflectable2 *>(pPropPtr);
		const TypeInfo * compTypeInfo = Reflector3::GetSingleton().GetTypeInfo(reflectableProp->GetReflectableClassID());
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


	void BinaryReflectorSerializer::SerializeString(std::string_view pSerializedString)
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

	void BinaryReflectorSerializer::SerializeValuesForObject(std::string_view /*pObjectName*/, SerializedValueFiller /*pFillerFunction*/)
	{
		// not implemented for now (mostly used for JSON metadata)
	}

}
#endif