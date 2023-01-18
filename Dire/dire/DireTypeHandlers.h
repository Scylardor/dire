#pragma once

namespace DIRE_NS
{
	struct IArrayDataStructureHandler;
	struct MapDataStructureHandler;
	struct EnumDataStructureHandler;

	class DataStructureHandler
	{
	public:
		DataStructureHandler() = default;
		DataStructureHandler(const IArrayDataStructureHandler* pArrayHandler, const MapDataStructureHandler* pMapHandler, const EnumDataStructureHandler* pEnumHandler) :
			ArrayHandler(pArrayHandler), MapHandler(pMapHandler), EnumHandler(pEnumHandler)
		{}
		DataStructureHandler(const IArrayDataStructureHandler * pArrayHandler) :
			ArrayHandler(pArrayHandler)
		{}

		DataStructureHandler(const MapDataStructureHandler* pMapHandler) :
			MapHandler(pMapHandler)
		{}

		DataStructureHandler(const EnumDataStructureHandler* pEnumHandler) :
			EnumHandler(pEnumHandler)
		{}


		[[nodiscard]] const IArrayDataStructureHandler*	GetArrayHandler() const { return ArrayHandler; }
		[[nodiscard]] const MapDataStructureHandler  *	GetMapHandler() const { return MapHandler; }
		[[nodiscard]] const EnumDataStructureHandler *	GetEnumHandler() const { return EnumHandler; }

	private:
		const IArrayDataStructureHandler *	ArrayHandler = nullptr;
		const MapDataStructureHandler *		MapHandler = nullptr;
		const EnumDataStructureHandler *	EnumHandler = nullptr;
	};
}
