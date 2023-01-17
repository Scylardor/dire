#pragma once
#if DIRE_USE_SERIALIZATION && DIRE_USE_JSON_SERIALIZATION

#include "DireSerialization.h"
#include "DireTypes.h"

#include <rapidjson/document.h>

namespace DIRE_NS
{
	class DataStructureHandler;
	struct MapDataStructureHandler;
	struct ArrayDataStructureHandler;

	class RapidJsonReflectorDeserializer : public IDeserializer
	{
	public:
		virtual Result	DeserializeInto(char const* pJson, Reflectable2& pDeserializedObject) override;

	private:

		void	DeserializeArrayValue(const rapidjson::Value& pVal, void* pPropPtr, const ArrayDataStructureHandler * pArrayHandler) const;

		void	DeserializeMapValue(const rapidjson::Value& pVal, void* pPropPtr, const MapDataStructureHandler * pMapHandler) const;

		void	DeserializeCompoundValue(const rapidjson::Value& pVal, void* pPropPtr) const;

		void	DeserializeValue(void const* pSerializedVal, Type pPropType, void* pPropPtr, const DataStructureHandler* pHandler = nullptr) const;
	};
}
#endif
