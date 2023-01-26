
#include "DireJSONDeserializer.h"

#ifdef DIRE_COMPILE_JSON_SERIALIZATION

#include "dire/Types/DireTypeInfo.h"
#include "dire/Types/DireTypeInfoDatabase.h"
#include "dire/DireReflectable.h"
#include "dire/Handlers/DireTypeHandlers.h"

#include <rapidjson/error/en.h>
#include <rapidjson/document.h>

#include <cassert>

/* This macro does a cast on the right side of the equal sign to silence warnings about casting char to int for example */
#define JSON_DESERIALIZE_VALUE_CASE(TypeEnum, JsonFunc) \
	case MetaType::TypeEnum:\
	*(FromEnumTypeToActualType<MetaType::TypeEnum>::ActualType *)(pPropPtr) = (FromEnumTypeToActualType<MetaType::TypeEnum>::ActualType) jsonVal->Get##JsonFunc();\
	break;

namespace DIRE_NS
{
	IDeserializer::Result RapidJsonReflectorDeserializer::DeserializeInto(char const* pJson, Reflectable& pDeserializedObject)
	{
		rapidjson::Document doc;
		rapidjson::ParseResult ok = doc.Parse(pJson);
		if (doc.Parse(pJson).HasParseError())
		{
			auto neededSize = snprintf(nullptr, 0, "JSON parse error: %s (%llu)", GetParseError_En(ok.Code()), ok.Offset());
			DIRE_STRING error(neededSize+1, '\0');
			snprintf(error.data(), error.size(), "JSON parse error: %s (%llu)", GetParseError_En(ok.Code()), ok.Offset());

			return { error };
		}

		TypeInfoDatabase::GetSingleton().GetTypeInfo(pDeserializedObject.GetReflectableClassID())->ForEachPropertyInHierarchy([&pDeserializedObject, &doc, this](const PropertyTypeInfo& pProperty)
		{
			Reflectable::PropertyAccessor accessor = pDeserializedObject.GetProperty(pProperty.GetName());
			void* propPtr = const_cast<void*>(accessor.GetPointer());
			rapidjson::Value const& propValue = doc[pProperty.GetName().data()];
			DeserializeValue(&propValue, pProperty.GetMetatype(), propPtr, &pProperty.GetDataStructureHandler());
		});

		return { &pDeserializedObject };
	}

	void RapidJsonReflectorDeserializer::DeserializeArrayValue(const rapidjson::Value& pVal, void* pPropPtr, const IArrayDataStructureHandler * pArrayHandler) const
	{
		DIRE_ASSERT(pVal.IsArray());

		if (pArrayHandler != nullptr)
		{
			DataStructureHandler elemHandler = pArrayHandler->ElementHandler();

			MetaType elemType = pArrayHandler->ElementType();
			if (elemType != MetaType::Unknown)
			{
				for (auto iElem = 0u; iElem < pVal.Size(); ++iElem)
				{
					void* elemVal = const_cast<void*>(pArrayHandler->Read(pPropPtr, iElem));
					DeserializeValue(&pVal[iElem], elemType, elemVal, &elemHandler);
				}
			}
		}
	}


	void RapidJsonReflectorDeserializer::DeserializeMapValue(const rapidjson::Value& pVal, void* pPropPtr, const IMapDataStructureHandler * pMapHandler) const
	{
		if (pPropPtr == nullptr || pMapHandler == nullptr)
			return;

		DIRE_ASSERT(pVal.IsObject());

		const MetaType valueType = pMapHandler->ValueMetaType();
		const DataStructureHandler valueHandler = pMapHandler->ValueDataHandler();
		for (rapidjson::Value::ConstMemberIterator itr = pVal.MemberBegin(); itr != pVal.MemberEnd(); ++itr)
		{
			void* createdValue = pMapHandler->Create(pPropPtr, itr->name.GetString(), nullptr);
			DeserializeValue(&itr->value, valueType, createdValue, &valueHandler);
		}
	}


	void RapidJsonReflectorDeserializer::DeserializeCompoundValue(const rapidjson::Value& pVal, void* pPropPtr) const
	{
		DIRE_ASSERT(pVal.IsObject());
		auto* reflectableProp = static_cast<Reflectable*>(pPropPtr);
		const TypeInfo * compTypeInfo = TypeInfoDatabase::GetSingleton().GetTypeInfo(reflectableProp->GetReflectableClassID());
		DIRE_ASSERT(compTypeInfo != nullptr);

		compTypeInfo->ForEachPropertyInHierarchy([this, &pVal, reflectableProp](const PropertyTypeInfo & pProperty)
			{
				Reflectable::PropertyAccessor prop = reflectableProp->GetProperty(pProperty.GetName());
				void* propPtr = const_cast<void*>(prop.GetPointer());
				const rapidjson::Value & propValue = pVal[pProperty.GetName().data()];
				DeserializeValue(&propValue, pProperty.GetMetatype(), propPtr, &pProperty.GetDataStructureHandler());
			});
	}

	void	RapidJsonReflectorDeserializer::DeserializeValue(void const* pSerializedVal, MetaType pPropType, void* pPropPtr, const DataStructureHandler* pHandler) const
	{
		auto* jsonVal = static_cast<const rapidjson::Value*>(pSerializedVal);

		switch (pPropType.Value)
		{
			JSON_DESERIALIZE_VALUE_CASE(MetaType::Bool, Bool)
			// JSON doesn't have a concept of "small types" like char and short so just decode it as int.
			JSON_DESERIALIZE_VALUE_CASE(Char, Int)
			JSON_DESERIALIZE_VALUE_CASE(UChar, Int)
			JSON_DESERIALIZE_VALUE_CASE(Short, Int)
			JSON_DESERIALIZE_VALUE_CASE(UShort, Int)
			JSON_DESERIALIZE_VALUE_CASE(Int, Int)
			JSON_DESERIALIZE_VALUE_CASE(Uint, Uint)
			JSON_DESERIALIZE_VALUE_CASE(Int64, Int64)
			JSON_DESERIALIZE_VALUE_CASE(Uint64, Uint64)
			JSON_DESERIALIZE_VALUE_CASE(Float, Double)
			JSON_DESERIALIZE_VALUE_CASE(Double, Double)
		case MetaType::Array:
			if (pHandler != nullptr)
			{
				DeserializeArrayValue(*jsonVal, pPropPtr, pHandler->GetArrayHandler());
			}
			break;
		case MetaType::Map:
			if (pHandler != nullptr)
			{
				DeserializeMapValue(*jsonVal, pPropPtr, pHandler->GetMapHandler());
			}
			break;
		case MetaType::Object:
			DeserializeCompoundValue(*jsonVal, pPropPtr);
			break;
		case MetaType::Enum:
			pHandler->GetEnumHandler()->SetFromString(jsonVal->GetString(), pPropPtr);
			break;
		default:
			// Unmanaged type in DeserializeValue!
			DIRE_ASSERT(false); // for now
		}
	}
}
#endif
