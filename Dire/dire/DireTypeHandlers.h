#pragma once

namespace DIRE_NS
{
	class ArrayDataStructureHandler;
	class MapDataStructureHandler;
	class EnumDataStructureHandler;

	class DataStructureHandler
	{
	public:
		const ArrayDataStructureHandler *	GetArrayHandler() const { return ArrayHandler; }
		const MapDataStructureHandler *		GetMapHandler() const { return MapHandler; }
		const EnumDataStructureHandler *	GetEnumHandler() const { return EnumHandler; }

	private:
		const ArrayDataStructureHandler *	ArrayHandler = nullptr;
		const MapDataStructureHandler *		MapHandler = nullptr;
		const EnumDataStructureHandler *	EnumHandler = nullptr;
	};
}
