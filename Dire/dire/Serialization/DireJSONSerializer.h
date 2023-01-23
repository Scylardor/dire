#pragma once

#include "DireDefines.h"

#ifdef DIRE_COMPILE_JSON_SERIALIZATION

# include <rapidjson/rapidjson.h>
# include <rapidjson/stringbuffer.h>	// wrapper of C stream for prettywriter as output
# include <rapidjson/prettywriter.h>	// for stringify JSON

#include "DireSerialization.h"

#include "dire/Types/DireTypes.h"

namespace DIRE_NS
{
	class RapidJsonReflectorSerializer : public ISerializer
	{
	public:

		Dire_EXPORT virtual Result Serialize(const Reflectable & serializedObject) override;

		virtual bool	SerializesMetadata() const override
		{
			return true;
		}

		inline virtual void	SerializeString(DIRE_STRING_VIEW pSerializedString) override;
		inline virtual void	SerializeInt(int32_t pSerializedInt) override;
		inline virtual void	SerializeFloat(float pSerializedFloat) override;
		inline virtual void	SerializeBool(bool pSerializedBool) override;
		inline virtual void	SerializeValuesForObject(DIRE_STRING_VIEW pObjectName, SerializedValueFiller pFillerFunction) override;

		void	Dire_EXPORT SerializeValue(Type pPropType, const void * pPropPtr, const DataStructureHandler * pHandler = nullptr);

		void	Dire_EXPORT SerializeArrayValue(const void * pPropPtr, const IArrayDataStructureHandler * pArrayHandler);

		void	Dire_EXPORT SerializeMapValue(const void * pPropPtr, const IMapDataStructureHandler * pMapHandler);

		void	Dire_EXPORT SerializeCompoundValue(const void * pPropPtr);

	private:

		void	SerializeReflectable(const Reflectable& pReflectable);

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