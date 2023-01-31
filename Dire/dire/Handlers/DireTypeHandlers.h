#pragma once

namespace DIRE_NS
{
	class IArrayDataStructureHandler;
	class IMapDataStructureHandler;
	class IEnumDataStructureHandler;

	/**
	 * \brief Generic storage class holding a pointer to a handler class allowing to interact with a property based on its type.
	 *	It holds a union pointer to either an array, map, or enum handler (or nullptr if none of these apply).
	 *	Do not use in your code: it is set and used internally by the Dire property system.
	 *	Usage of the MetaType allows the system to know which pointer of the union it should use.
	 */
	class Dire_EXPORT DataStructureHandler
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
		union Dire_EXPORT HandlersUnion
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
