#if DIRE_USE_SERIALIZATION && DIRE_USE_JSON_SERIALIZATION

#include "DireTypeInfo.h"
#include "DireTypeInfoDatabase.h"
#include "DireReflectable.h"
#include "DireJSONDeserializer.h"
#include "DireTypeHandlers.h"

#include "DireAssert.h"

#include <rapidjson/error/en.h>
#include <rapidjson/document.h>

#include <cassert>

/* This macro does a cast on the right side of the equal sign to silence warnings about casting char to int for example */
#define JSON_DESERIALIZE_VALUE_CASE(TypeEnum, JsonFunc) \
	case Type::TypeEnum:\
	*(FromEnumTypeToActualType<Type::TypeEnum>::ActualType *)(pPropPtr) = (FromEnumTypeToActualType<Type::TypeEnum>::ActualType) jsonVal->Get##JsonFunc();\
	break;

namespace DIRE_NS
{
	void RapidJsonReflectorDeserializer::DeserializeInto(char const* pJson, Reflectable2& pDeserializedObject)
	{
		rapidjson::Document doc;
		rapidjson::ParseResult ok = doc.Parse(pJson);
		if (doc.Parse(pJson).HasParseError())
		{
			// TODO: encapsulate in REFLECTION_ERROR customizable macro
			fprintf(stderr, "JSON parse error: %s (%llu)",
				rapidjson::GetParseError_En(ok.Code()), ok.Offset());
			return;
		}

		Reflector3::GetSingleton().GetTypeInfo(pDeserializedObject.GetReflectableClassID())->ForEachPropertyInHierarchy([&pDeserializedObject, &doc, this](const PropertyTypeInfo& pProperty)
			{
				void* propPtr = const_cast<void*>(pDeserializedObject.GetProperty(pProperty.GetName()));
				rapidjson::Value const& propValue = doc[pProperty.GetName().data()];
				DeserializeValue(&propValue, pProperty.GetMetatype(), propPtr, &pProperty.GetDataStructureHandler());
			});
	}

	void RapidJsonReflectorDeserializer::DeserializeArrayValue(const rapidjson::Value& pVal, void* pPropPtr, const ArrayDataStructureHandler * pArrayHandler) const
	{
		DIRE_ASSERT(pVal.IsArray());

		if (pArrayHandler != nullptr)
		{
			DataStructureHandler elemHandler = pArrayHandler->ElementHandler();

			Type elemType = pArrayHandler->ElementType();
			if (elemType != Type::Unknown)
			{
				for (auto iElem = 0u; iElem < pVal.Size(); ++iElem)
				{
					void* elemVal = const_cast<void*>(pArrayHandler->Read(pPropPtr, iElem));
					DeserializeValue(&pVal[iElem], elemType, elemVal, &elemHandler);
				}
			}
		}
	}


	void RapidJsonReflectorDeserializer::DeserializeMapValue(const rapidjson::Value& pVal, void* pPropPtr, const MapDataStructureHandler * pMapHandler) const
	{
		if (pPropPtr == nullptr || pMapHandler == nullptr)
			return;

		DIRE_ASSERT(pVal.IsObject());

		const Type valueType = pMapHandler->GetValueType();
		const DataStructureHandler valueHandler = pMapHandler->ValueHandler();
		for (rapidjson::Value::ConstMemberIterator itr = pVal.MemberBegin(); itr != pVal.MemberEnd(); ++itr)
		{
			void* createdValue = pMapHandler->Create(pPropPtr, itr->name.GetString(), nullptr);
			DeserializeValue(&itr->value, valueType, createdValue, &valueHandler);
		}
	}


	void RapidJsonReflectorDeserializer::DeserializeCompoundValue(const rapidjson::Value& pVal, void* pPropPtr) const
	{
		DIRE_ASSERT(pVal.IsObject());
		auto* reflectableProp = static_cast<Reflectable2*>(pPropPtr);
		const TypeInfo * compTypeInfo = Reflector3::GetSingleton().GetTypeInfo(reflectableProp->GetReflectableClassID());
		DIRE_ASSERT(compTypeInfo != nullptr);

		compTypeInfo->ForEachPropertyInHierarchy([this, &pVal, reflectableProp](const PropertyTypeInfo & pProperty)
			{
				void* propPtr = const_cast<void*>(reflectableProp->GetProperty(pProperty.GetName()));
				rapidjson::Value const& propValue = pVal[pProperty.GetName().data()];
				DeserializeValue(&propValue, pProperty.GetMetatype(), propPtr, &pProperty.GetDataStructureHandler());
			});
	}

	void	RapidJsonReflectorDeserializer::DeserializeValue(void const* pSerializedVal, Type pPropType, void* pPropPtr, const DataStructureHandler* pHandler) const
	{
		auto* jsonVal = static_cast<const rapidjson::Value*>(pSerializedVal);

		switch (pPropType.Value)
		{
			JSON_DESERIALIZE_VALUE_CASE(Type::Bool, Bool)
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
		case Type::Array:
			if (pHandler != nullptr)
			{
				DeserializeArrayValue(*jsonVal, pPropPtr, pHandler->GetArrayHandler());
			}
			break;
		case Type::Map:
			if (pHandler != nullptr)
			{
				DeserializeMapValue(*jsonVal, pPropPtr, pHandler->GetMapHandler());
			}
			break;
		case Type::Object:
			DeserializeCompoundValue(*jsonVal, pPropPtr);
			break;
		case Type::Enum:
			pHandler->GetEnumHandler()->SetFromString(jsonVal->GetString(), pPropPtr);
			break;
		default:
			// Unmanaged type in DeserializeValue!
			DIRE_ASSERT(false); // for now
		}
	}
}
#endif
