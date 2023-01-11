#pragma once

#if DIRE_USE_SERIALIZATION && DIRE_SERIALIZATION_JSON_SERIALIZER_ENABLED
/* This macro does a cast on the right side of the equal sign to silence warnings about casting char to int for example */
#define JSON_DESERIALIZE_VALUE_CASE(TypeEnum, JsonFunc) \
	case Type::TypeEnum:\
	*(FromEnumTypeToActualType<Type::TypeEnum>::ActualType *)(pPropPtr) = (FromEnumTypeToActualType<Type::TypeEnum>::ActualType) jsonVal->Get##JsonFunc();\
	break;

namespace DIRE_NS
{
	class RapidJsonReflectorDeserializer : public IDeserializer
	{
	public:
		virtual void	DeserializeInto(char const* pJson, Reflectable2& pDeserializedObject) override;

	private:

		void	DeserializeArrayValue(const rapidjson::Value& pVal, void* pPropPtr, ArrayPropertyCRUDHandler const* pArrayHandler) const;

		void	DeserializeMapValue(const rapidjson::Value& pVal, void* pPropPtr, MapPropertyCRUDHandler const* pMapHandler) const;

		void	DeserializeCompoundValue(const rapidjson::Value& pVal, void* pPropPtr) const;

		void	DeserializeValue(void const* pSerializedVal, Type pPropType, void* pPropPtr, AbstractPropertyHandler const* pHandler = nullptr) const
		{
			auto* jsonVal = static_cast<const rapidjson::Value*>(pSerializedVal);

			switch (pPropType)
			{
				JSON_DESERIALIZE_VALUE_CASE(Bool, Bool)
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
					DeserializeArrayValue(*jsonVal, pPropPtr, pHandler->ArrayHandler);
				}
				break;
			case Type::Map:
				if (pHandler != nullptr)
				{
					DeserializeMapValue(*jsonVal, pPropPtr, pHandler->MapHandler);
				}
				break;
			case Type::Object:
				DeserializeCompoundValue(*jsonVal, pPropPtr);
				break;
			case Type::Enum:
				pHandler->EnumHandler->SetFromString(jsonVal->GetString(), pPropPtr);
				break;
			default:
				std::cerr << "Unmanaged type in SerializeValue!";
				assert(false); // TODO: for now
			}
		}
	};
}
#endif
