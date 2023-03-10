
#include "DireJSONSerializer.h"

#ifdef DIRE_COMPILE_JSON_SERIALIZATION

#include "dire/Handlers/DireEnumDataStructureHandler.h"
#include "dire/Types/DireTypeInfo.h"
#include "dire/DireReflectable.h"

namespace DIRE_NS
{

#define JSON_SERIALIZE_VALUE_CASE(TypeEnum, JsonFunc) \
case MetaType::TypeEnum:\
	myJsonWriter.JsonFunc(*static_cast<const FromEnumTypeToActualType<MetaType::TypeEnum>::ActualType *>(pPropPtr));\
	break;


	ISerializer::Result JsonReflectorSerializer::Serialize(const Reflectable& serializedObject)
	{
		myBuffer.Clear(); // to clean any previously written information
		myJsonWriter.Reset(myBuffer); // to wipe the root of a previously serialized object

		SerializeReflectable(serializedObject);

		return { myBuffer.GetString(), myBuffer.GetSize() };
	}


	void JsonReflectorSerializer::SerializeArrayValue(const void * pPropPtr, const IArrayDataStructureHandler * pArrayHandler)
	{
		myJsonWriter.StartArray();

		if (pArrayHandler != nullptr)
		{
			MetaType elemType = pArrayHandler->ElementType();
			if (elemType != MetaType::Unknown)
			{
				size_t arraySize = pArrayHandler->Size(pPropPtr);
				DataStructureHandler elemHandler = pArrayHandler->ElementHandler();

				for (size_t iElem = 0; iElem < arraySize; ++iElem)
				{
					const void * elemVal = pArrayHandler->Read(pPropPtr, iElem);
					SerializeValue(elemType, elemVal, &elemHandler);
				}
			}
		}

		myJsonWriter.EndArray();
	}

	void JsonReflectorSerializer::SerializeMapValue(const void * pPropPtr, const IMapDataStructureHandler * pMapHandler)
	{
		myJsonWriter.StartObject();

		if (pPropPtr != nullptr && pMapHandler != nullptr)
		{
			pMapHandler->SerializeForEachPair(pPropPtr, this, [](void* pSerializer, const void* pKey, const void* pVal, const IMapDataStructureHandler & pMap,
				const DataStructureHandler & /*pKeyHandler*/, const DataStructureHandler & pValueHandler)
				{
					auto* myself = static_cast<JsonReflectorSerializer*>(pSerializer);

					const DIRE_STRING keyStr = pMap.KeyToString(pKey);
					myself->myJsonWriter.String(keyStr.data(), rapidjson::SizeType(keyStr.length()));

					MetaType valueType = pMap.ValueMetaType();
					myself->SerializeValue(valueType, pVal, &pValueHandler);
				});
		}

		myJsonWriter.EndObject();
	}

	void JsonReflectorSerializer::SerializeCompoundValue(const void * pPropPtr)
	{
		DIRE_ASSERT(pPropPtr != nullptr);
		auto* reflectableProp = static_cast<const Reflectable *>(pPropPtr);
		SerializeReflectable(*reflectableProp);
	}

	void JsonReflectorSerializer::SerializeReflectable(const Reflectable& pReflectable)
	{
		const TypeInfo* typeInfo = pReflectable.GetReflectableTypeInfo();

		DIRE_ASSERT(typeInfo != nullptr);

		myJsonWriter.StartObject();

		typeInfo->ForEachPropertyInHierarchy([this, &pReflectable](const PropertyTypeInfo& pProperty)
		{
			PropertyTypeInfo::SerializationState serializableState = pProperty.GetSerializableState();
			if (serializableState.IsSerializable == true)
			{
				void const* propPtr = pReflectable.GetProperty(pProperty.GetName());
				myJsonWriter.String(pProperty.GetName().data(), rapidjson::SizeType(pProperty.GetName().size()));
				this->SerializeValue(pProperty.GetMetatype(), propPtr, &pProperty.GetDataStructureHandler());

				if (SerializesMetadata() && serializableState.HasAttributesToSerialize)
				{
					const DIRE_STRING metadataName = DIRE_STRING(pProperty.GetName()) + "_metadata";
					myJsonWriter.String(metadataName.data(), rapidjson::SizeType(metadataName.size()));

					myJsonWriter.StartObject();

					pProperty.SerializeAttributes(*this);

					myJsonWriter.EndObject();
				}
			}
		});

		myJsonWriter.EndObject();
	}


	void JsonReflectorSerializer::SerializeValue(MetaType pPropType, const void* pPropPtr, const DataStructureHandler* pHandler)
	{
		switch (pPropType.Value)
		{
			JSON_SERIALIZE_VALUE_CASE(Bool, Bool)
			// JSON doesn't have a concept of "small types" like char and short so just encode them as int.
			JSON_SERIALIZE_VALUE_CASE(Char, Int)
			JSON_SERIALIZE_VALUE_CASE(UChar, Int)
			JSON_SERIALIZE_VALUE_CASE(Short, Int)
			JSON_SERIALIZE_VALUE_CASE(UShort, Int)
			JSON_SERIALIZE_VALUE_CASE(Int, Int)
			JSON_SERIALIZE_VALUE_CASE(Uint, Uint)
			JSON_SERIALIZE_VALUE_CASE(Int64, Int64)
			JSON_SERIALIZE_VALUE_CASE(Uint64, Uint64)
			case MetaType::Float: // Have to handle them without the macro, otherwise have the warning that float implicitly converts to double
				myJsonWriter.Double(double(*static_cast<const float*>(pPropPtr)));
				break;
			case MetaType::Double:
				myJsonWriter.Double(*static_cast<const double*>(pPropPtr));
				break;
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
			const char* enumStr = pHandler->GetEnumHandler()->EnumToString(pPropPtr);
			myJsonWriter.String(enumStr);
		}
		break;
		default:
			// Unmanaged type!
			DIRE_ASSERT(false); // for now
		}
	}
}

#endif
