#pragma once

#include "DireTypeHandlers.h"
#include "dire/Types/DireTypes.h"
#include "dire/DireReflectableID.h"
#include "dire/Utils/DireTypeTraits.h"

namespace DIRE_NS
{
	class DataStructureHandler;

	template <typename T, typename = void>
	class TypedArrayDataStructureHandler
	{};

	/**
	 * \brief Generic interface for type-erased array handlers.
	 * This class will be derived by handlers for both static arrays and array-like data structures (like std::vector).
	 */
	class Dire_EXPORT IArrayDataStructureHandler
	{
	public:
		IArrayDataStructureHandler() = default;
		virtual ~IArrayDataStructureHandler() = default;
		IArrayDataStructureHandler(const IArrayDataStructureHandler&) = default;
		IArrayDataStructureHandler& operator=(const IArrayDataStructureHandler&) = default;

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


	/**
	 * \brief An intermediate wrapper for functions common to both types of array handlers.
	 */
	class Dire_EXPORT AbstractArrayDataStructureHandler : public IArrayDataStructureHandler
	{
	protected:
		/**
		 * \brief Returns the reflectable ID of the array's element type, or INVALID_REFLECTABLE_ID if this is not a Reflectable.
		 * \tparam ElementValueType The element type
		 */
		template <typename ElementValueType>
		static ReflectableID	GetElementReflectableID();

		/**
		 * \brief Populates a DataStructureHandler that will hold the handler matching the data structure of the element type.
		 * The handler pointer will be null if the type doesn't have any known handler.
		 * \tparam ElementValueType The element type
		 */
		template <typename ElementValueType>
		static DataStructureHandler	GetTypedElementHandler();
	};
	

	/**
	 * \brief Type-erased array handler for reflectable properties that have all the traits of an Array (e.g. std::vector), but who are not static arrays.
	 * \tparam T The property type
	 */
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

	/**
	 * \brief Type-erased array handler for reflectable properties that are static arrays.
	 * \tparam T The property type
	 */
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
