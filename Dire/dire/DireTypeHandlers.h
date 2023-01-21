#pragma once

namespace DIRE_NS
{
	class IArrayDataStructureHandler;
	class IMapDataStructureHandler;
	class IEnumDataStructureHandler;

	class DataStructureHandler
	{
	public:
		DataStructureHandler() = default;
		DataStructureHandler(const IArrayDataStructureHandler * pArrayHandler) :
			myHandlers(pArrayHandler)
		{}

		DataStructureHandler(const IMapDataStructureHandler* pMapHandler) :
			myHandlers(pMapHandler)
		{}

		DataStructureHandler(const IEnumDataStructureHandler* pEnumHandler) :
			myHandlers(pEnumHandler)
		{}

		[[nodiscard]] const IArrayDataStructureHandler*	GetArrayHandler() const { return myHandlers.ArrayHandler; }
		[[nodiscard]] const IMapDataStructureHandler  *	GetMapHandler() const { return myHandlers.MapHandler; }
		[[nodiscard]] const IEnumDataStructureHandler *	GetEnumHandler() const { return myHandlers.EnumHandler; }

	private:
		union HandlersUnion
		{
			HandlersUnion() = default;

			HandlersUnion(const IArrayDataStructureHandler* pArrayHandler) :
				ArrayHandler(pArrayHandler)
			{}

			HandlersUnion(const IMapDataStructureHandler* pMapHandler) :
				MapHandler(pMapHandler)
			{}

			HandlersUnion(const IEnumDataStructureHandler* pEnumHandler) :
				EnumHandler(pEnumHandler)
			{}

			const IArrayDataStructureHandler*	ArrayHandler = nullptr;
			const IMapDataStructureHandler*		MapHandler;
			const IEnumDataStructureHandler*	EnumHandler;
		} ;

		HandlersUnion	myHandlers;
	};
}
