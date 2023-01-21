#pragma once

#include "DireTypes.h"

namespace DIRE_NS
{
	class IEnumDataStructureHandler
	{
	public:
		virtual const char*		EnumToString(const void*) const = 0;
		virtual void			SetFromString(const char*, void*) const = 0;
		virtual Type::Values	EnumMetaType() const = 0;

		virtual ~IEnumDataStructureHandler() = default;
	};

	template <class T>
	class TypedEnumDataStructureHandler final : public IEnumDataStructureHandler
	{
	public:
		virtual const char* EnumToString(const void* pVal) const override
		{
			return T::GetStringFromSafeEnum(*(const typename T::Values*)pVal);
		}

		virtual void	SetFromString(const char* pEnumStr, void* pEnumAddr) const override
		{
			T* theEnum = (T*)pEnumAddr;
			if (theEnum != nullptr)
			{
				const typename T::Values enumValue = T::GetValueFromSafeString(pEnumStr);
				*theEnum = enumValue;
			}
		}

		virtual Type::Values	EnumMetaType() const override
		{
			return FromEnumToUnderlyingType<T>();
		}

		static TypedEnumDataStructureHandler const& GetInstance()
		{
			static TypedEnumDataStructureHandler instance{};
			return instance;
		}
	};

}
