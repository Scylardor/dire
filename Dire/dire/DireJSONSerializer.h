#pragma once

#if DIRE_USE_SERIALIZATION && DIRE_SERIALIZATION_JSON_SERIALIZER_ENABLED

namespace DIRE_NS
{
# include <rapidjson/rapidjson.h>
# include <rapidjson/stringbuffer.h>	// wrapper of C stream for prettywriter as output
# include <rapidjson/prettywriter.h>	// for stringify JSON
# include "rapidjson/document.h"		// rapidjson's DOM-style API
# include "rapidjson/error/en.h"		// rapidjson's error encoding into messages

#define JSON_SERIALIZE_VALUE_CASE(TypeEnum, JsonFunc) \
case Type::TypeEnum:\
	myJsonWriter.JsonFunc(*static_cast<FromEnumTypeToActualType<Type::TypeEnum>::ActualType const*>(pPropPtr));\
	break;

	class RapidJsonReflectorSerializer : public ISerializer
	{
		virtual Result Serialize(Reflectable2 const& serializedObject) override;

		virtual bool	SerializesMetadata() const override
		{
			return true;
		}

		virtual void	SerializeString(std::string_view pSerializedString) override;
		virtual void	SerializeInt(int32_t pSerializedInt) override;
		virtual void	SerializeFloat(float pSerializedFloat) override;
		virtual void	SerializeBool(bool pSerializedBool) override;
		virtual void	SerializeValuesForObject(std::string_view pObjectName, SerializedValueFiller pFillerFunction) override;

		void	SerializeValue(Type pPropType, void const* pPropPtr, AbstractPropertyHandler const* pHandler = nullptr)
		{
			switch (pPropType)
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
					SerializeArrayValue(pPropPtr, pHandler->ArrayHandler);
				}
				break;
			case Type::Map:
				if (pHandler != nullptr)
				{
					SerializeMapValue(pPropPtr, pHandler->MapHandler);
				}
				break;
			case Type::Object:
				SerializeCompoundValue(pPropPtr);
				break;
			case Type::Enum:
			{
				const char* enumStr = pHandler->EnumHandler->EnumToString(pPropPtr);
				myJsonWriter.String(enumStr);
			}
			break;
			default:
				std::cerr << "Unmanaged type in SerializeValue!";
				assert(false); // TODO: for now
			}
		}

		void	SerializeArrayValue(void const* pPropPtr, ArrayPropertyCRUDHandler const* pArrayHandler);

		void	SerializeMapValue(void const* pPropPtr, MapPropertyCRUDHandler const* pMapHandler);

		void	SerializeCompoundValue(void const* pPropPtr);

		// TODO: allow to provide a custom allocator for StringBuffer and Writer
		rapidjson::StringBuffer	myBuffer;
		rapidjson::Writer<rapidjson::StringBuffer>	myJsonWriter;
	};

	void RapidJsonReflectorSerializer::SerializeString(std::string_view pSerializedString)
	{
		myJsonWriter.String(pSerializedString.data(), (rapidjson::SizeType)pSerializedString.size());
	}

	void RapidJsonReflectorSerializer::SerializeInt(int32_t pSerializedInt)
	{
		myJsonWriter.Int(pSerializedInt);
	}

	void RapidJsonReflectorSerializer::SerializeFloat(float pSerializedFloat)
	{
		myJsonWriter.Double(pSerializedFloat);
	}

	void RapidJsonReflectorSerializer::SerializeBool(bool pSerializedBool)
	{
		myJsonWriter.Bool(pSerializedBool);
	}

	void RapidJsonReflectorSerializer::SerializeValuesForObject(std::string_view pObjectName, SerializedValueFiller pFillerFunction)
	{
		myJsonWriter.String(pObjectName.data(), (rapidjson::SizeType)pObjectName.size());

		myJsonWriter.StartObject();

		pFillerFunction(*this);

		myJsonWriter.EndObject();
	}
}

#endif