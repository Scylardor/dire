#pragma once

#include "DireDefines.h"
#ifdef DIRE_COMPILE_JSON_SERIALIZATION

#include "DireSerialization.h"
#include "DireTypes.h"

#include <rapidjson/document.h>

namespace DIRE_NS
{
	class DataStructureHandler;
	class IMapDataStructureHandler;
	class IArrayDataStructureHandler;

	class RapidJsonReflectorDeserializer : public IDeserializer
	{
	public:
		virtual Result	DeserializeInto(char const* pJson, Reflectable& pDeserializedObject) override;

	private:

		void	DeserializeArrayValue(const rapidjson::Value& pVal, void* pPropPtr, const IArrayDataStructureHandler * pArrayHandler) const;

		void	DeserializeMapValue(const rapidjson::Value& pVal, void* pPropPtr, const IMapDataStructureHandler * pMapHandler) const;

		void	DeserializeCompoundValue(const rapidjson::Value& pVal, void* pPropPtr) const;

		void	DeserializeValue(void const* pSerializedVal, Type pPropType, void* pPropPtr, const DataStructureHandler* pHandler = nullptr) const;
	};
}
#endif
