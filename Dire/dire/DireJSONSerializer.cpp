
#if DIRE_USE_SERIALIZATION && DIRE_USE_JSON_SERIALIZATION

#include "DireJSONSerializer.h"

#include "DireTypeInfoDatabase.h"
#include "DireEnumDataStructureHandler.h"
#include "DireTypeInfo.h"
#include "DireReflectable.h"

namespace DIRE_NS
{

#define JSON_SERIALIZE_VALUE_CASE(TypeEnum, JsonFunc) \
case Type::TypeEnum:\
	myJsonWriter.JsonFunc(*static_cast<const FromEnumTypeToActualType<Type::TypeEnum>::ActualType *>(pPropPtr));\
	break;


	ISerializer::Result RapidJsonReflectorSerializer::Serialize(const Reflectable2& serializedObject)
	{
		myBuffer.Clear(); // to clean any previously written information
		myJsonWriter.Reset(myBuffer); // to wipe the root of a previously serialized object

		myJsonWriter.StartObject();

		Reflector3::GetSingleton().GetTypeInfo(serializedObject.GetReflectableClassID())->ForEachPropertyInHierarchy([&serializedObject, this](const PropertyTypeInfo & pProperty)
		{
			PropertyTypeInfo::SerializationState serializableState = pProperty.GetSerializableState();
			if (serializableState.IsSerializable == true)
			{
				void const* propPtr = serializedObject.GetProperty(pProperty.GetName());
				myJsonWriter.String(pProperty.GetName().data(), (rapidjson::SizeType)pProperty.GetName().size());
				this->SerializeValue(pProperty.GetMetatype(), propPtr, &pProperty.GetDataStructureHandler());

				if (SerializesMetadata() && serializableState.HasAttributesToSerialize)
				{
					const std::string metadataName = pProperty.GetName() + "_metadata";
					myJsonWriter.String(metadataName.data(), (rapidjson::SizeType)metadataName.size());

					myJsonWriter.StartObject();

					pProperty.SerializeAttributes(*this);

					myJsonWriter.EndObject();
				}
			}
		});

		myJsonWriter.EndObject();

		return Result(myBuffer.GetString(), myBuffer.GetSize());
	}


	void RapidJsonReflectorSerializer::SerializeArrayValue(const void * pPropPtr, const ArrayDataStructureHandler * pArrayHandler)
	{
		myJsonWriter.StartArray();

		if (pArrayHandler != nullptr)
		{
			Type elemType = pArrayHandler->ElementType();
			if (elemType != Type::Unknown)
			{
				size_t arraySize = pArrayHandler->Size(pPropPtr);
				DataStructureHandler elemHandler = pArrayHandler->ElementHandler();

				for (int iElem = 0; iElem < arraySize; ++iElem)
				{
					const void * elemVal = pArrayHandler->Read(pPropPtr, iElem);
					SerializeValue(elemType, elemVal, &elemHandler);
				}
			}
		}

		myJsonWriter.EndArray();
	}

	void RapidJsonReflectorSerializer::SerializeMapValue(const void * pPropPtr, const MapDataStructureHandler * pMapHandler)
	{
		myJsonWriter.StartObject();

		if (pPropPtr != nullptr && pMapHandler != nullptr)
		{
			pMapHandler->SerializeForEachPair(pPropPtr, this, [](void* pSerializer, const void* pKey, const void* pVal, const MapDataStructureHandler & pMapHandler,
				const DataStructureHandler & /*pKeyHandler*/, const DataStructureHandler & pValueHandler)
				{
					auto* myself = static_cast<RapidJsonReflectorSerializer*>(pSerializer);

					const std::string keyStr = pMapHandler.KeyToString(pKey);
					myself->myJsonWriter.String(keyStr.data(), (rapidjson::SizeType)keyStr.length());

					Type valueType = pMapHandler.GetValueType();
					myself->SerializeValue(valueType, pVal, &pValueHandler);
				});
		}

		myJsonWriter.EndObject();
	}

	void RapidJsonReflectorSerializer::SerializeCompoundValue(const void * pPropPtr)
	{
		// TODO: Casting to Reflectable feels easy here... Shouldn't there be a way to properly reflect structs without making them reflectable ?
		auto* reflectableProp = static_cast<const Reflectable2 *>(pPropPtr);
		const TypeInfo * compTypeInfo = Reflector3::GetSingleton().GetTypeInfo(reflectableProp->GetReflectableClassID());
		assert(compTypeInfo != nullptr); // TODO: customize assert

		myJsonWriter.StartObject();

		// TODO: Should be refactored with Serialize()
		compTypeInfo->ForEachPropertyInHierarchy([this, reflectableProp](const PropertyTypeInfo & pProperty)
			{
				PropertyTypeInfo::SerializationState serializableState = pProperty.GetSerializableState();
				if (serializableState.IsSerializable == true)
				{
					void const* compProp = reflectableProp->GetProperty(pProperty.GetName());
					myJsonWriter.String(pProperty.GetName().data(), (rapidjson::SizeType)pProperty.GetName().size());
					Type propType = pProperty.GetMetatype();
					SerializeValue(propType, compProp, &pProperty.GetDataStructureHandler());

					if (SerializesMetadata() && serializableState.HasAttributesToSerialize)
					{
						const std::string metadataName = pProperty.GetName() + "_metadata";
						myJsonWriter.String(metadataName.data(), (rapidjson::SizeType)metadataName.size());

						myJsonWriter.StartObject();

						pProperty.SerializeAttributes(*this);

						myJsonWriter.EndObject();
					}
				}
			});

		myJsonWriter.EndObject();
	}


	void RapidJsonReflectorSerializer::SerializeValue(Type pPropType, const void* pPropPtr, const DataStructureHandler* pHandler)
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
			JSON_SERIALIZE_VALUE_CASE(Float, Double)
			JSON_SERIALIZE_VALUE_CASE(Double, Double)
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
			const char* enumStr = pHandler->GetEnumHandler()->EnumToString(pPropPtr);
			myJsonWriter.String(enumStr);
		}
		break;
		default:
			// Unmanaged type!
			assert(false); // TODO: for now
		}
	}
}

#endif