#pragma once
#include "DireEnumDataStructureHandler.h"
#include "DireMapDataStructureHandler.h"

namespace DIRE_NS
{
	template <typename ElementValueType>
	ReflectableID AbstractArrayDataStructureHandler::GetElementReflectableID()
	{
		if constexpr (std::is_base_of_v<Reflectable, ElementValueType>)
		{
			return ElementValueType::GetTypeInfo().GetID();
		}
		else
		{
			return DIRE_NS::INVALID_REFLECTABLE_ID;
		}
	}

	template <typename ElementValueType>
	DataStructureHandler AbstractArrayDataStructureHandler::GetTypedElementHandler()
	{
		if constexpr (HasMapSemantics_v<ElementValueType>)
		{
			return DataStructureHandler(&TypedMapDataStructureHandler<ElementValueType>::GetInstance());
		}
		else if constexpr (HasArraySemantics_v<ElementValueType>)
		{
			return DataStructureHandler(&TypedArrayDataStructureHandler<ElementValueType>::GetInstance());
		}
		else if constexpr (std::is_base_of_v<Enum, ElementValueType>)
		{
			return DataStructureHandler(&TypedEnumDataStructureHandler<ElementValueType>::GetInstance());
		}
		else
			return {};
	}


	template <typename T>
	const void* TypedArrayDataStructureHandler<T, std::enable_if_t<HasArraySemantics_v<T> && !std::is_array_v<T>, void>>::Read(const void* pArray, size_t pIndex) const
	{
		if (pArray == nullptr)
		{
			return nullptr;
		}

		if (pIndex >= Size(pArray))
		{
			Create(const_cast<void*>(pArray), pIndex, nullptr);
		}

		T const* thisArray = static_cast<T const*>(pArray);
		return &(*thisArray)[pIndex];
	}

	template <typename T>
	void TypedArrayDataStructureHandler<T, std::enable_if_t<HasArraySemantics_v<T> && !std::is_array_v<T>, void>>::Update(void* pArray, size_t pIndex, const void* pUpdateValue) const
	{
		if (pArray == nullptr)
		{
			return;
		}

		T* thisArray = static_cast<T*>(pArray);
		if (pIndex >= Size(pArray))
		{
			thisArray->resize(pIndex + 1);
		}

		ElementValueType const* actualData = static_cast<ElementValueType const*>(pUpdateValue);
		(*thisArray)[pIndex] = actualData ? *actualData : ElementValueType();
	}

	template <typename T>
	bool TypedArrayDataStructureHandler<T, std::enable_if_t<HasArraySemantics_v<T> && !std::is_array_v<T>, void>>::Erase(void* pArray, size_t pIndex) const
	{
		T* thisArray = static_cast<T*>(pArray);

		if (thisArray != nullptr && thisArray->size() > pIndex)
		{
			thisArray->erase(thisArray->begin() + pIndex);
			return true;
		}

		return false;
	}

	template <typename T>
	void TypedArrayDataStructureHandler<T, std::enable_if_t<HasArraySemantics_v<T> && !std::is_array_v<T>, void>>::Clear(void* pArray) const
	{
		if (pArray != nullptr)
		{
			T* thisArray = static_cast<T*>(pArray);
			thisArray->clear();
		}
	}

	template <typename T>
	size_t TypedArrayDataStructureHandler<T, std::enable_if_t<HasArraySemantics_v<T> && !std::is_array_v<T>, void>>::Size(const void* pArray) const
	{
		if (pArray != nullptr)
		{
			T const* thisArray = static_cast<T const*>(pArray);
			return thisArray->size();
		}

		return 0;
	}


	template <typename T>
	const void* TypedArrayDataStructureHandler<T, std::enable_if_t<std::is_array_v<T>, void>>::Read(const void* pArray, size_t pIndex) const
	{
		if (pArray == nullptr)
		{
			return nullptr;
		}

		IndexCheck(pIndex);

		T const* thisArray = static_cast<T const*>(pArray);
		return &(*thisArray)[pIndex];
	}

	template <typename T>
	void TypedArrayDataStructureHandler<T, std::enable_if_t<std::is_array_v<T>, void>>::Update(void* pArray, size_t pIndex, const void* pUpdateValue) const
	{
		if (pArray == nullptr || pUpdateValue == nullptr)
		{
			return;
		}

		IndexCheck(pIndex);

		// Check if the element is assignable to make arrays of arrays work
		if constexpr (std::is_assignable_v<ElementValueType&, ElementValueType>)
		{
			T* thisArray = static_cast<T*>(pArray);
			ElementValueType const* actualData = static_cast<ElementValueType const*>(pUpdateValue);
			(*thisArray)[pIndex] = *actualData;
		}
	}

	template <typename T>
	void TypedArrayDataStructureHandler<T, std::enable_if_t<std::is_array_v<T>, void>>::Create(void* pArray, size_t pIndex, const void* pCreateValue) const
	{
		if (pArray == nullptr)
		{
			return;
		}

		IndexCheck(pIndex);

		// Check if the element is assignable to make arrays of arrays work
		if constexpr (std::is_assignable_v<ElementValueType&, ElementValueType>)
		{
			T* thisArray = static_cast<T*>(pArray);
			ElementValueType const* actualInitData = (pCreateValue ? static_cast<ElementValueType const*>(pCreateValue) : nullptr);
			(*thisArray)[pIndex] = actualInitData ? *actualInitData : ElementValueType();
		}
	}

	template <typename T>
	bool TypedArrayDataStructureHandler<T, std::enable_if_t<std::is_array_v<T>, void>>::Erase(void* pArray, size_t pIndex) const
	{
		if (pArray == nullptr || pIndex >= ARRAY_SIZE)
		{
			return false;
		}

		// Check if the element is assignable to make arrays of arrays work
		if constexpr (std::is_assignable_v<ElementValueType&, ElementValueType>)
		{
			T* thisArray = static_cast<T*>(pArray);
			(*thisArray)[pIndex] = ElementValueType();
			return true;
		}
		else
			return false;
	}

	template <typename T>
	void TypedArrayDataStructureHandler<T, std::enable_if_t<std::is_array_v<T>, void>>::Clear(void* pArray) const
	{
		if (pArray == nullptr)
		{
			return;
		}

		// I don't know if we can do something smarter in this case...
		// Check if the element is assignable to make arrays of arrays work
		if constexpr (std::is_assignable_v<ElementValueType&, ElementValueType>)
		{
			T* thisArray = static_cast<T*>(pArray);
			for (auto i = 0u; i < ARRAY_SIZE; ++i)
			{
				(*thisArray)[i] = ElementValueType();
			}
		}
	}

	template <typename T>
	void TypedArrayDataStructureHandler<T, std::enable_if_t<std::is_array_v<T>, void>>::IndexCheck(size_t pIndex)
	{
		if (ARRAY_SIZE <= pIndex)
		{
			char buffer[256]{ 0 };
			std::snprintf(buffer, sizeof(buffer), "Static Array handler was used with an index out of range (index: %llu vs. array size: %llu)", pIndex, ARRAY_SIZE);
			throw std::out_of_range(buffer);
		}
	}
}
