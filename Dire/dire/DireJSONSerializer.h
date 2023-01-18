#pragma once

#if DIRE_USE_SERIALIZATION && DIRE_USE_JSON_SERIALIZATION

# include <rapidjson/rapidjson.h>
# include <rapidjson/stringbuffer.h>	// wrapper of C stream for prettywriter as output
# include <rapidjson/prettywriter.h>	// for stringify JSON

#include "DireSerialization.h"

#include "DireTypes.h"
#include "DireTypeHandlers.h"



namespace DIRE_NS
{
	class RapidJsonReflectorSerializer : public ISerializer
	{
	public:

		virtual Result Serialize(const Reflectable2 & serializedObject) override;

		virtual bool	SerializesMetadata() const override
		{
			return true;
		}

		inline virtual void	SerializeString(DIRE_STRING_VIEW pSerializedString) override;
		inline virtual void	SerializeInt(int32_t pSerializedInt) override;
		inline virtual void	SerializeFloat(float pSerializedFloat) override;
		inline virtual void	SerializeBool(bool pSerializedBool) override;
		inline virtual void	SerializeValuesForObject(DIRE_STRING_VIEW pObjectName, SerializedValueFiller pFillerFunction) override;

		inline void	SerializeValue(Type pPropType, const void * pPropPtr, const DataStructureHandler * pHandler = nullptr);

		inline void	SerializeArrayValue(const void * pPropPtr, const IArrayDataStructureHandler * pArrayHandler);

		inline void	SerializeMapValue(const void * pPropPtr, const MapDataStructureHandler * pMapHandler);

		inline void	SerializeCompoundValue(const void * pPropPtr);

	private:

		void	SerializeReflectable(const Reflectable2& pReflectable);

		using StringBuffer = rapidjson::GenericStringBuffer<rapidjson::UTF8<>, DIRE_RAPIDJSON_ALLOCATOR>;
		using Writer = rapidjson::Writer<StringBuffer>;

		StringBuffer	myBuffer;
		Writer			myJsonWriter;
	};

	void RapidJsonReflectorSerializer::SerializeString(DIRE_STRING_VIEW pSerializedString)
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

	void RapidJsonReflectorSerializer::SerializeValuesForObject(DIRE_STRING_VIEW pObjectName, SerializedValueFiller pFillerFunction)
	{
		myJsonWriter.String(pObjectName.data(), (rapidjson::SizeType)pObjectName.size());

		myJsonWriter.StartObject();

		pFillerFunction(*this);

		myJsonWriter.EndObject();
	}
}

#endif