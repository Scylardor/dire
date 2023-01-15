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

		void	SerializeValue(Type pPropType, const void * pPropPtr, const DataStructureHandler * pHandler = nullptr);

		void	SerializeArrayValue(const void * pPropPtr, const ArrayDataStructureHandler * pArrayHandler);

		void	SerializeMapValue(const void * pPropPtr, const MapDataStructureHandler * pMapHandler);

		void	SerializeCompoundValue(const void * pPropPtr);

		// TODO: allow to provide a custom allocator for StringBuffer and Writer
		rapidjson::StringBuffer	myBuffer;
		rapidjson::Writer<rapidjson::StringBuffer>	myJsonWriter;
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