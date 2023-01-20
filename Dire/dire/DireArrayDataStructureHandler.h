#pragma once

#include "DireTypeHandlers.h"
#include "DireEnumDataStructureHandler.h"
#include "DireMapDataStructureHandler.h"
#include "DireTypes.h"
#include "DireReflectableID.h"

namespace DIRE_NS
{
	class DataStructureHandler;

	template <typename T, typename = void>
	struct TypedArrayDataStructureHandler
	{};

	struct IArrayDataStructureHandler
	{
		virtual ~IArrayDataStructureHandler() = default;

		virtual const void*				Read(const void* pArray, size_t pIndex) const = 0;
		virtual void					Update(void* pArray, size_t pIndex, const void* pUpdateValue) const = 0;
		virtual void					Create(void* pArray, size_t pIndex, const void* pCreateValue) const = 0;
		virtual bool					Erase(void* pArray, size_t pIndex) const = 0;
		virtual void					Clear(void* pArray) const = 0;
		virtual size_t					Size(const void* pArray) const = 0;
		virtual DataStructureHandler	ElementHandler() const = 0;
		virtual ReflectableID			ElementReflectableID() const = 0;
		virtual Type					ElementType() const = 0;
		virtual size_t					ElementSize() const = 0;
	};


	class AbstractArrayDataStructureHandler : public IArrayDataStructureHandler
	{
	protected:
		template <typename ElementValueType>
		static ReflectableID			GetElementReflectableID()
		{
			if constexpr (std::is_base_of_v<Reflectable2, ElementValueType>)
			{
				return ElementValueType::GetTypeInfo().GetID();
			}
			else
			{
				return DIRE_NS::INVALID_REFLECTABLE_ID;
			}
		}

		template <typename ElementValueType>
		static DataStructureHandler	GetTypedElementHandler()
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

			return {};
		}
	};


	// Base class for reflectable properties that have brackets operator to be able to make operations on the underlying array
	template <typename T>
	struct TypedArrayDataStructureHandler<T,
		typename std::enable_if_t<HasArraySemantics_v<T> && !std::is_array_v<T>>> final : public AbstractArrayDataStructureHandler
	{
		using RawElementType = decltype(std::declval<T>()[0]);
		// Decaying removes the references and fixes the compile error "pointer to reference is illegal"
		using ElementValueType = std::decay_t<RawElementType>;

		TypedArrayDataStructureHandler() = default;

		static_assert(has_ArrayBrackets_v<T>
			&& has_insert_v<T>
			&& has_erase_v<T>
			&& has_clear_v<T>
			&& has_size_v<T>,
			"In order to use Array-style reflection indexing, your type has to implement the following member functions with the same signature-style as std::vector:"
			" operator[], begin, insert(iterator, value), erase, clear, size, resize.");

		virtual const void* Read(const void* pArray, size_t pIndex) const override
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

		virtual void					Update(void* pArray, size_t pIndex, const void* pUpdateValue) const override
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

		virtual void					Create(void* pArray, size_t pIndex, const void* pCreateValue) const override
		{
			return Update(pArray, pIndex, pCreateValue);
		}

		virtual bool					Erase(void* pArray, size_t pIndex) const override
		{
			T* thisArray = static_cast<T*>(pArray);

			if (thisArray != nullptr && thisArray->size() > pIndex)
			{
				thisArray->erase(thisArray->begin() + pIndex);
				return true;
			}

			return false;
		}

		virtual void					Clear(void* pArray) const override
		{
			if (pArray != nullptr)
			{
				T* thisArray = static_cast<T*>(pArray);
				thisArray->clear();
			}
		}

		virtual size_t					Size(const void* pArray) const override
		{
			if (pArray != nullptr)
			{
				T const* thisArray = static_cast<T const*>(pArray);
				return thisArray->size();
			}

			return 0;
		}

		virtual DataStructureHandler	ElementHandler() const override
		{
			return GetTypedElementHandler<ElementValueType>();
		}

		virtual ReflectableID			ElementReflectableID() const override
		{
			return GetElementReflectableID<ElementValueType>();
		}

		virtual Type					ElementType() const override
		{
			return Type(FromActualTypeToEnumType<ElementValueType>::EnumType);
		}

		virtual size_t					ElementSize() const override
		{
			return sizeof(ElementValueType);
		}

		static TypedArrayDataStructureHandler const& GetInstance()
		{
			static TypedArrayDataStructureHandler instance{};
			return instance;
		}
	};

	// Base class for reflectable properties that are C-arrays to be able to make operations on the underlying array
	template <typename T>
	struct TypedArrayDataStructureHandler<T,
		typename std::enable_if_t<std::is_array_v<T>>> final : AbstractArrayDataStructureHandler
	{
		using RawElementType = decltype(std::declval<T>()[0]);
		// remove_reference_t removes the references and fixes the compile error "pointer to reference is illegal"
		using ElementValueType = std::remove_reference_t<RawElementType>;

		inline static const size_t ARRAY_SIZE = std::extent_v<T>;

		TypedArrayDataStructureHandler() = default;

		virtual const void*		Read(const void* pArray, size_t pIndex) const override
		{
			if (pArray == nullptr)
			{
				return nullptr;
			}

			IndexCheck(pIndex);

			T const* thisArray = static_cast<T const*>(pArray);
			return &(*thisArray)[pIndex];
		}

		virtual void					Update(void* pArray, size_t pIndex, const void* pUpdateValue) const override
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

		virtual void					Create(void* pArray, size_t pIndex, const void* pCreateValue) const override
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

		virtual bool					Erase(void* pArray, size_t pIndex) const override
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

			return false;
		}

		virtual void					Clear(void* pArray) const override
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

		virtual size_t					Size(const void* /*pArray*/) const override
		{
			return ARRAY_SIZE;
		}

		virtual DataStructureHandler	ElementHandler() const override
		{
			return GetTypedElementHandler<ElementValueType>();
		}

		virtual ReflectableID			ElementReflectableID() const override
		{
			return GetElementReflectableID<ElementValueType>();
		}

		virtual Type					ElementType() const override
		{
			return Type(FromActualTypeToEnumType<ElementValueType>::EnumType);
		}

		virtual size_t					ElementSize() const override
		{
			return sizeof(ElementValueType);
		}

		static TypedArrayDataStructureHandler const& GetInstance()
		{
			static TypedArrayDataStructureHandler instance{};
			return instance;
		}

	private:
		static void	IndexCheck(size_t pIndex)
		{
			if (ARRAY_SIZE <= pIndex)
			{
				char buffer[256]{ 0 };
				std::snprintf(buffer, sizeof(buffer), "Static Array handler was used with an index out of range (index: %llu vs. array size: %llu)", pIndex, ARRAY_SIZE);
				throw std::out_of_range(buffer);
			}
		}
	};
}