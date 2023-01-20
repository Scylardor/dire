#pragma once

namespace DIRE_NS
{
	struct IArrayDataStructureHandler;
	struct MapDataStructureHandler;
	struct IEnumDataStructureHandler;

	class DataStructureHandler
	{
	public:
		DataStructureHandler() = default;
		DataStructureHandler(const IArrayDataStructureHandler * pArrayHandler) :
			myHandlers(pArrayHandler)
		{}

		DataStructureHandler(const MapDataStructureHandler* pMapHandler) :
			myHandlers(pMapHandler)
		{}

		DataStructureHandler(const IEnumDataStructureHandler* pEnumHandler) :
			myHandlers(pEnumHandler)
		{}

		[[nodiscard]] const IArrayDataStructureHandler*	GetArrayHandler() const { return myHandlers.ArrayHandler; }
		[[nodiscard]] const MapDataStructureHandler  *	GetMapHandler() const { return myHandlers.MapHandler; }
		[[nodiscard]] const IEnumDataStructureHandler *	GetEnumHandler() const { return myHandlers.EnumHandler; }

	private:
		union HandlersUnion
		{
			HandlersUnion() = default;

			HandlersUnion(const IArrayDataStructureHandler* pArrayHandler) :
				ArrayHandler(pArrayHandler)
			{}

			HandlersUnion(const MapDataStructureHandler* pMapHandler) :
				MapHandler(pMapHandler)
			{}

			HandlersUnion(const IEnumDataStructureHandler* pEnumHandler) :
				EnumHandler(pEnumHandler)
			{}

			const IArrayDataStructureHandler*	ArrayHandler = nullptr;
			const MapDataStructureHandler*		MapHandler;
			const IEnumDataStructureHandler*	EnumHandler;
		} ;

		HandlersUnion	myHandlers;
	};
}
