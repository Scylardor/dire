#pragma once

namespace DIRE_NS
{
	class DataStructureHandler;

	class ArrayPropertyCRUDHandler
	{
		using ArrayReadFptr = void const* (*)(void const*, size_t);
		using ArrayUpdateFptr = void (*)(void*, size_t, void const*);
		using ArrayCreateFptr = void (*)(void*, size_t, void const*);
		using ArrayEraseFptr = void	(*)(void*, size_t);
		using ArrayClearFptr = void	(*)(void*);
		using ArraySizeFptr = size_t(*)(void const*);
		using ArrayElementHandlerFptr = DataStructureHandler(*)();
		using ArrayElementReflectableIDFptr = unsigned (*)();
		using ArrayElementType = Type(*)();
		using ArrayElementSize = size_t(*)();

		ArrayReadFptr	Read = nullptr;
		ArrayUpdateFptr Update = nullptr;
		ArrayCreateFptr	Create = nullptr;
		ArrayEraseFptr	Erase = nullptr;
		ArrayClearFptr	Clear = nullptr;
		ArraySizeFptr	Size = nullptr;
		ArrayElementHandlerFptr	ElementHandler = nullptr;
		ArrayElementReflectableIDFptr ElementReflectableID = nullptr;
		ArrayElementType	ElementType = nullptr;
		ArrayElementSize	ElementSize = nullptr;
	};


	class DataStructureHandler
	{
		ArrayPropertyCRUDHandler const* ArrayHandler = nullptr;
		MapPropertyCRUDHandler const* MapHandler = nullptr;
		struct EnumDataHandler const* EnumHandler = nullptr;
	};
}