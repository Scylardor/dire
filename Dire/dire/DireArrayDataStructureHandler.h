#pragma once

#include "DireTypeHandlers.h"
#include "DireEnumDataStructureHandler.h"
#include "DireMapDataStructureHandler.h"
#include "DireTypes.h"

namespace DIRE_NS
{
	class DataStructureHandler;

	struct ArrayDataStructureHandler
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

	// CRUD = Create, Read, Update, Delete
	template <typename T, typename = void>
	struct TypedArrayDataStructureHandler
	{};

	// Base class for reflectable properties that have brackets operator to be able to make operations on the underlying array
	template <typename T>
	struct TypedArrayDataStructureHandler<T,
		typename std::enable_if_t<HasArraySemantics_v<T> && !std::is_array_v<T>>> : ArrayDataStructureHandler
	{
		using RawElementType = decltype(std::declval<T>()[0]);
		// Decaying removes the references and fixes the compile error "pointer to reference is illegal"
		using ElementValueType = std::decay_t<RawElementType>;

		TypedArrayDataStructureHandler()
		{
			Read = &ArrayRead;
			Update = &ArrayUpdate;
			Create = &ArrayCreate;
			Erase = &ArrayErase;
			Clear = &ArrayClear;
			Size = &ArraySize;
			ElementHandler = &ArrayElementHandler;
			ElementReflectableID = &ArrayElementReflectableID;
			ElementType = [] { return Type(FromActualTypeToEnumType<ElementValueType>::EnumType); };
			ElementSize = [] { return sizeof(ElementValueType); };
		}

		static_assert(has_ArrayBrackets_v<T>
			&& has_insert_v<T>
			&& has_erase_v<T>
			&& has_clear_v<T>
			&& has_size_v<T>,
			"In order to use Array-style reflection indexing, your type has to implement the following member functions with the same signature-style as std::vector:"
			" operator[], begin, insert(iterator, value), erase, clear, size, resize.");

		static void const* ArrayRead(void const* pArray, size_t pIndex)
		{
			if (pArray == nullptr)
			{
				return nullptr;
			}

			if (pIndex >= ArraySize(pArray))
			{
				ArrayCreate(const_cast<void*>(pArray), pIndex, nullptr);
			}

			T const* thisArray = static_cast<T const*>(pArray);
			return &(*thisArray)[pIndex];
		}

		static void	ArrayUpdate(void* pArray, size_t pIndex, void const* pNewData)
		{
			if (pArray == nullptr)
			{
				return;
			}

			T* thisArray = static_cast<T*>(pArray);
			if (pIndex >= ArraySize(pArray))
			{
				thisArray->resize(pIndex + 1);
			}

			ElementValueType const* actualData = static_cast<ElementValueType const*>(pNewData);
			(*thisArray)[pIndex] = actualData ? *actualData : ElementValueType();
		}

		static void	ArrayCreate(void* pArray, size_t pAtIndex, void const* pInitData)
		{
			return ArrayUpdate(pArray, pAtIndex, pInitData);
		}

		static void	ArrayErase(void* pArray, size_t pAtIndex)
		{
			if (pArray != nullptr)
			{
				T* thisArray = static_cast<T*>(pArray);
				thisArray->erase(thisArray->begin() + pAtIndex);
			}
		}

		static void	ArrayClear(void* pArray)
		{
			if (pArray != nullptr)
			{
				T* thisArray = static_cast<T*>(pArray);
				thisArray->clear();
			}
		}

		static size_t	ArraySize(void const* pArray)
		{
			if (pArray != nullptr)
			{
				T const* thisArray = static_cast<T const*>(pArray);
				return thisArray->size();
			}

			return 0;
		}

		static DataStructureHandler	ArrayElementHandler()
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

		static unsigned ArrayElementReflectableID() // TODO: ReflectableID
		{
			if constexpr (std::is_base_of_v<Reflectable2, T>)
			{
				return T::GetClassReflectableTypeInfo().ReflectableID;
			}
			else
			{
				return INVALID_REFLECTABLE_ID;
			}
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
		typename std::enable_if_t<std::is_array_v<T>>> : ArrayDataStructureHandler
	{
		using RawElementType = decltype(std::declval<T>()[0]);
		// remove_reference_t removes the references and fixes the compile error "pointer to reference is illegal"
		using ElementValueType = std::remove_reference_t<RawElementType>;

		inline static const size_t ARRAY_SIZE = std::extent_v<T>;

		TypedArrayDataStructureHandler()
		{
			Read = &ArrayRead;
			Update = &ArrayUpdate;
			Create = &ArrayCreate;
			Erase = &ArrayErase;
			Clear = &ArrayClear;
			Size = &ArraySize;
			ElementHandler = &ArrayElementHandler;
			ElementReflectableID = &ArrayElementReflectableID;
			ElementType = [] { return Type(FromActualTypeToEnumType<ElementValueType>::EnumType); };
			ElementSize = [] { return sizeof(ElementValueType); };
		}

		static void const* ArrayRead(void const* pArray, size_t pIndex)
		{
			if (pArray == nullptr)
			{
				return nullptr;
			}
			T const* thisArray = static_cast<T const*>(pArray);
			return &(*thisArray)[pIndex];
		}

		static void	ArrayUpdate(void* pArray, size_t pIndex, void const* pNewData)
		{
			if (pArray == nullptr || pNewData == nullptr)
			{
				return;
			}

			// Check if the element is assignable to make arrays of arrays work
			if constexpr (std::is_assignable_v<ElementValueType, ElementValueType>)
			{
				T* thisArray = static_cast<T*>(pArray);
				ElementValueType const* actualData = static_cast<ElementValueType const*>(pNewData);
				(*thisArray)[pIndex] = *actualData;
			}
		}

		static void	ArrayCreate(void* pArray, size_t pAtIndex, void const* pInitData)
		{
			if (pArray == nullptr)
			{
				return;
			}

			// TODO: throw exception if exceptions are enabled
			assert(pAtIndex < ARRAY_SIZE);

			// Check if the element is assignable to make arrays of arrays work
			if constexpr (std::is_assignable_v<ElementValueType, ElementValueType>)
			{
				T* thisArray = static_cast<T*>(pArray);
				ElementValueType const* actualInitData = (pInitData ? static_cast<ElementValueType const*>(pInitData) : nullptr);
				(*thisArray)[pAtIndex] = actualInitData ? *actualInitData : ElementValueType();
			}
		}

		static void	ArrayErase(void* pArray, size_t pAtIndex)
		{
			if (pArray == nullptr || pAtIndex >= ARRAY_SIZE)
			{
				return;
			}

			// Check if the element is assignable to make arrays of arrays work
			if constexpr (std::is_assignable_v<ElementValueType&, ElementValueType>)
			{
				T* thisArray = static_cast<T*>(pArray);
				(*thisArray)[pAtIndex] = ElementValueType();
			}
		}

		static void	ArrayClear(void* pArray)
		{
			if (pArray == nullptr)
			{
				return;
			}

			// I don't know if we can do something smarter in this case...
			// Check if the element is assignable to make arrays of arrays work
			if constexpr (std::is_assignable_v<ElementValueType, ElementValueType>)
			{
				T* thisArray = static_cast<T*>(pArray);
				for (auto i = 0u; i < ARRAY_SIZE; ++i)
				{
					(*thisArray)[i] = ElementValueType();
				}
			}
		}

		static size_t	ArraySize(void const* /*pArray*/)
		{
			return ARRAY_SIZE;
		}

		// TODO: Refactor because there is code duplication between the two handler types
		static DataStructureHandler	ArrayElementHandler()
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

		static unsigned ArrayElementReflectableID() // TODO: ReflectableID
		{
			if constexpr (std::is_base_of_v<Reflectable2, ElementValueType>)
			{
				return ElementValueType::GetClassReflectableTypeInfo().GetID();
			}
			else
			{
				return INVALID_REFLECTABLE_ID;
			}
		}

		static TypedArrayDataStructureHandler const& GetInstance()
		{
			static TypedArrayDataStructureHandler instance{};
			return instance;
		}
	};
}