#pragma once

#include "DireTypes.h"

namespace DIRE_NS
{
	struct EnumDataStructureHandler
	{
		using EnumToStringFptr = const char* (*)(const void*);
		using SetFromStringFptr = void (*)(const char*, void*);
		using EnumTypeFptr = Type::Values (*)();

		EnumToStringFptr	EnumToString = nullptr;
		SetFromStringFptr	SetFromString = nullptr;
		EnumTypeFptr		EnumType = nullptr;
	};

	template <class T>
	struct TypedEnumDataStructureHandler : EnumDataStructureHandler
	{
		TypedEnumDataStructureHandler()
		{
			EnumToString = [](const void* pVal) { return T::GetStringFromSafeEnum(*(const typename T::Values*)pVal); };
			SetFromString = &SetEnumFromString;
			EnumType = &FromEnumToUnderlyingType<T>;
		}

		static void	SetEnumFromString(const char* pEnumStr, void* pEnumAddr)
		{
			T* theEnum = (T*)pEnumAddr;
			if (theEnum != nullptr)
			{
				const typename T::Values enumValue = T::GetValueFromSafeString(pEnumStr);
				*theEnum = enumValue;
			}
		}

		static TypedEnumDataStructureHandler const& GetInstance()
		{
			static TypedEnumDataStructureHandler instance{};
			return instance;
		}
	};

}
