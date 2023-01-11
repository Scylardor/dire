#pragma once

#include "DireTypes.h"

namespace DIRE_NS
{
	class EnumDataStructureHandler
	{
		using EnumToStringFptr = const char* (*)(const void*);
		using SetFromStringFptr = void (*)(const char*, void*);
		using EnumTypeFptr = Type(*)();

	protected:
		EnumToStringFptr	EnumToString = nullptr;
		SetFromStringFptr	SetFromString = nullptr;
		EnumTypeFptr		EnumType = nullptr;
	};

	template <class T>
	struct TypedEnumDataStructureHandler : EnumDataStructureHandler
	{
		TypedEnumDataStructureHandler()
		{
			EnumToString = [](const void* pVal) { return T::GetStringFromSafeEnum(*(const typename T::Type*)pVal); };
			SetFromString = &SetEnumFromString;
			EnumType = &FromEnumToUnderlyingType<T>;
		}

		static void	SetEnumFromString(const char* pEnumStr, void* pEnumAddr)
		{
			T* theEnum = (T*)pEnumAddr;
			const typename T::Type* enumValue = T::GetValueFromString(pEnumStr);
			if (theEnum && enumValue != nullptr)
			{
				*theEnum = *enumValue;
			}
		}

		static TypedEnumDataStructureHandler const& GetInstance()
		{
			static TypedEnumDataStructureHandler instance{};
			return instance;
		}
	};

}
