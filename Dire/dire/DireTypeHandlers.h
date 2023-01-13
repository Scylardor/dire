#pragma once

namespace DIRE_NS
{
	struct ArrayDataStructureHandler;
	struct MapDataStructureHandler;
	struct EnumDataStructureHandler;

	class DataStructureHandler
	{
	public:
		DataStructureHandler() = default;
		DataStructureHandler(const ArrayDataStructureHandler* pArrayHandler, const MapDataStructureHandler* pMapHandler, const EnumDataStructureHandler* pEnumHandler) :
			ArrayHandler(pArrayHandler), MapHandler(pMapHandler), EnumHandler(pEnumHandler)
		{}
		DataStructureHandler(const ArrayDataStructureHandler * pArrayHandler) :
			ArrayHandler(pArrayHandler)
		{}

		DataStructureHandler(const MapDataStructureHandler* pMapHandler) :
			MapHandler(pMapHandler)
		{}

		DataStructureHandler(const EnumDataStructureHandler* pEnumHandler) :
			EnumHandler(pEnumHandler)
		{}


		[[nodiscard]] const ArrayDataStructureHandler*	GetArrayHandler() const { return ArrayHandler; }
		[[nodiscard]] const MapDataStructureHandler  *	GetMapHandler() const { return MapHandler; }
		[[nodiscard]] const EnumDataStructureHandler *	GetEnumHandler() const { return EnumHandler; }

	private:
		const ArrayDataStructureHandler *	ArrayHandler = nullptr;
		const MapDataStructureHandler *		MapHandler = nullptr;
		const EnumDataStructureHandler *	EnumHandler = nullptr;
	};
}
