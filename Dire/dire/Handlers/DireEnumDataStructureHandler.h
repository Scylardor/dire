#pragma once

#include "dire/Types/DireTypes.h"

namespace DIRE_NS
{
	/**
	 * \brief Generic type-erased handler for properties that inherit Dire::Enum like SequentialEnum or BitmaskEnum.
	 * In practice we always manipulate instances of its child class, TypedEnumDataStructureHandler.
	 */
	class Dire_EXPORT IEnumDataStructureHandler
	{
	public:
		virtual const char*			EnumToString(const void*) const = 0;
		virtual void				SetFromString(const char*, void*) const = 0;
		virtual MetaType::Values	EnumMetaType() const = 0;

		IEnumDataStructureHandler() = default;
		virtual ~IEnumDataStructureHandler() = default;
		IEnumDataStructureHandler(const IEnumDataStructureHandler&) = default;
		IEnumDataStructureHandler& operator=(const IEnumDataStructureHandler&) = default;

	};

	template <class T>
	class TypedEnumDataStructureHandler final : public IEnumDataStructureHandler
	{
		static_assert(std::is_base_of_v<Enum, T>);

	public:
		virtual const char* EnumToString(const void* pVal) const override
		{
			return T::GetStringFromSafeEnum(*(const typename T::Values*)pVal);
		}

		virtual void	SetFromString(const char* pEnumStr, void* pEnumAddr) const override
		{
			T* theEnum = static_cast<T*>(pEnumAddr);
			if (theEnum != nullptr)
			{
				const typename T::Values enumValue = T::GetValueFromSafeString(pEnumStr);
				*theEnum = enumValue;
			}
		}

		virtual MetaType::Values	EnumMetaType() const override
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
