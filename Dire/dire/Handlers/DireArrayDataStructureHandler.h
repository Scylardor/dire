#pragma once

#include "DireTypeHandlers.h"
#include "DireEnumDataStructureHandler.h"
#include "DireMapDataStructureHandler.h"
#include "dire/Types/DireTypes.h"
#include "dire/DireReflectableID.h"

namespace DIRE_NS
{
	class DataStructureHandler;

	template <typename T, typename = void>
	class TypedArrayDataStructureHandler
	{};

	class Dire_EXPORT IArrayDataStructureHandler
	{
	public:
		virtual ~IArrayDataStructureHandler() = default;

		virtual const void*				Read(const void* pArray, size_t pIndex) const = 0;
		virtual void					Update(void* pArray, size_t pIndex, const void* pUpdateValue) const = 0;
		virtual void					Create(void* pArray, size_t pIndex, const void* pCreateValue) const = 0;
		virtual bool					Erase(void* pArray, size_t pIndex) const = 0;
		virtual void					Clear(void* pArray) const = 0;
		virtual size_t					Size(const void* pArray) const = 0;
		virtual DataStructureHandler	ElementHandler() const = 0;
		virtual ReflectableID			ElementReflectableID() const = 0;
		virtual MetaType				ElementType() const = 0;
		virtual size_t					ElementSize() const = 0;
	};


	class Dire_EXPORT AbstractArrayDataStructureHandler : public IArrayDataStructureHandler
	{
	protected:
		template <typename ElementValueType>
		static ReflectableID	GetElementReflectableID();

		template <typename ElementValueType>
		static DataStructureHandler	GetTypedElementHandler();
	};


	// Base class for reflectable properties that have brackets operator to be able to make operations on the underlying array
	template <typename T>
	class TypedArrayDataStructureHandler<T,
		typename std::enable_if_t<HasArraySemantics_v<T> && !std::is_array_v<T>>> final : public AbstractArrayDataStructureHandler
	{
		using RawElementType = decltype(std::declval<T>()[0]);
		// Decaying removes the references and fixes the compile error "pointer to reference is illegal"
		using ElementValueType = std::decay_t<RawElementType>;

	public:
		TypedArrayDataStructureHandler() = default;

		static_assert(has_ArrayBrackets_v<T>
			&& has_insert_v<T>
			&& has_erase_v<T>
			&& has_clear_v<T>
			&& has_size_v<T>,
			"In order to use Array-style reflection indexing, your type has to implement the following member functions with the same signature-style as std::vector:"
			" operator[], begin, insert(iterator, value), erase, clear, size, resize.");

		virtual const void* Read(const void* pArray, size_t pIndex) const override;

		virtual void					Update(void* pArray, size_t pIndex, const void* pUpdateValue) const override;

		virtual void					Create(void* pArray, size_t pIndex, const void* pCreateValue) const override
		{
			return Update(pArray, pIndex, pCreateValue);
		}

		virtual bool					Erase(void* pArray, size_t pIndex) const override;

		virtual void					Clear(void* pArray) const override;

		virtual size_t					Size(const void* pArray) const override;

		virtual DataStructureHandler	ElementHandler() const override
		{
			return GetTypedElementHandler<ElementValueType>();
		}

		virtual ReflectableID			ElementReflectableID() const override
		{
			return GetElementReflectableID<ElementValueType>();
		}

		virtual MetaType				ElementType() const override
		{
			return MetaType(FromActualTypeToEnumType<ElementValueType>::EnumType);
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
	class TypedArrayDataStructureHandler<T,
		typename std::enable_if_t<std::is_array_v<T>>> final : public AbstractArrayDataStructureHandler
	{
		using RawElementType = decltype(std::declval<T>()[0]);
		// remove_reference_t removes the references and fixes the compile error "pointer to reference is illegal"
		using ElementValueType = std::remove_reference_t<RawElementType>;

		inline static const size_t ARRAY_SIZE = std::extent_v<T>;

	public:

		TypedArrayDataStructureHandler() = default;

		virtual const void*	Read(const void* pArray, size_t pIndex) const override;

		virtual void		Update(void* pArray, size_t pIndex, const void* pUpdateValue) const override;

		virtual void		Create(void* pArray, size_t pIndex, const void* pCreateValue) const override;

		virtual bool		Erase(void* pArray, size_t pIndex) const override;

		virtual void		Clear(void* pArray) const override;

		virtual size_t		Size(const void* /*pArray*/) const override
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

		virtual MetaType				ElementType() const override
		{
			return MetaType(FromActualTypeToEnumType<ElementValueType>::EnumType);
		}

		virtual size_t					ElementSize() const override
		{
			return sizeof(ElementValueType);
		}

		static const TypedArrayDataStructureHandler & GetInstance()
		{
			static TypedArrayDataStructureHandler instance{};
			return instance;
		}

	private:
		static void	IndexCheck(size_t pIndex);
	};

}

#include "DireArrayDataStructureHandler.inl"