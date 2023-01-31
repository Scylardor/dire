#pragma once

#include "dire/Types/DireTypes.h"
#include "dire/Utils/DireString.h"
#include "dire/Utils/DireTypeTraits.h"
#include "dire/DireReflectableID.h"

namespace DIRE_NS
{
	class DataStructureHandler;

	/**
	 * \brief Generic interface for type-erased map handlers.
	 * This class will be derived by the TypedMapDataStructureHandler, which handles all properties that behave like an associative array (like std::map or std::unordered_map).
	 */
	class Dire_EXPORT IMapDataStructureHandler
	{
	public:

		IMapDataStructureHandler() = default;
		virtual ~IMapDataStructureHandler() = default;
		IMapDataStructureHandler(const IMapDataStructureHandler&) = default;
		IMapDataStructureHandler& operator=(const IMapDataStructureHandler&) = default;

		/**
		 * \brief Allows to read from the map, without creating the key if it doesn't exist.
		 * \param pMap Pointer to the map
		 * \param pKey The key to read (will be string-converted to the actual key type
		 * \return A pointer to the associated value inside the map, or nullptr if the key does not exist
		 */
		virtual const void*				Read(const void* pMap, const DIRE_STRING_VIEW& pKey) const = 0;

		/**
		 * \brief Updates the entry in the map at the provided key with the provided value. If it doesn't exist, will create it.
		 * \param pMap Pointer to the map
		 * \param pKey The accessed key
		 * \param pValue The value we should update the key with
		 */
		virtual void					Update(void* pMap, const DIRE_STRING_VIEW& pKey, const void* pValue) const = 0;

		/**
		 * \brief Creates an entry associated with the provided key and an optional value to initialize it with.
		 *	Will update an existing entry with the provided value.
		 * \param pMap Pointer to the map
		 * \param pKey The key to create the entry under.
		 * \param pValue An optional value to initialize the entry with. If nullptr, will default-construct.
		 * \return The new value stored in the map
		 */
		virtual void*					Create(void* pMap, const DIRE_STRING_VIEW& pKey, const void* pValue) const = 0;

		/**
		 * \brief Same as create, except that the key is already in binary format (can be cast directly into the key type).
		 * \param pMap Pointer to the map
		 * \param pKey The key to create the entry under.
		 * \param pValue An optional value to initialize the entry with. If nullptr, will default-construct.
		 * \return The new value stored in the map
		 */
		virtual void*					BinaryCreate(void* pMap, void const* pBinaryKey, const void* pValue) const = 0;

		/**
		 * \brief Erases the given key from the map. If it doesn't exist, does nothing.
		 * \param pMap Pointer to the map
		 * \param pKey Key to erase
		 * \return True if the key was erased.
		 */
		virtual bool					Erase(void* pMap, const DIRE_STRING_VIEW& pKey) const = 0;

		/**
		 * \brief Wipes everything in the map.
		 * \param pMap Pointer to the map
		 */
		virtual void					Clear(void* pMap) const = 0;

		/**
		 * \brief Returns the number of entries in the map
		 * \param pMap Pointer to the map
		 */
		virtual size_t					Size(const void* pMap) const = 0;

		/**
		 * \brief Returns the data structure handler associated to the value type of the map.
		 */
		virtual DataStructureHandler	ValueDataHandler() const = 0;

		/**
		 * \brief Returns the reflectable ID of the value type of the map, or INVALID_REFLECTABLE_ID if it isn't a Reflectable.
		 */
		virtual ReflectableID			ValueReflectableID() const = 0;

		virtual MetaType				KeyMetaType() const = 0;
		virtual MetaType				ValueMetaType() const = 0;
		virtual size_t					SizeofKey() const = 0;
		virtual size_t					SizeofValue() const = 0;
		virtual DataStructureHandler	KeyDataHandler() const = 0;

#ifdef DIRE_SERIALIZATION_ENABLED
		using OpaqueSerializerType = void*;
		using OpaqueMapType = const void *;
		using OpaqueKeyType = const void *;
		using OpaqueValueType = OpaqueKeyType;
		using KeyValuePairSerializeFptr = void (*)(OpaqueSerializerType, OpaqueKeyType, OpaqueValueType, const IMapDataStructureHandler &, const DataStructureHandler &, const DataStructureHandler &);

		/**
		 * \brief Used by serialization code. Visitor function that allows the serializer to have an opaque serialization callback called for each key/value pair in the map.
		 */
		virtual void		SerializeForEachPair(OpaqueMapType, OpaqueSerializerType, KeyValuePairSerializeFptr) const = 0;

		/**
		 * \brief Used by serialization code. Will return the string representation of the given key in binary format.
		 */
		virtual DIRE_STRING	KeyToString(OpaqueKeyType) const = 0;
#endif
	};

	template <typename T, typename = void>
	class TypedMapDataStructureHandler
	{};

	/**
	 * \brief Template class for reflectable properties that have all the traits of an associative array (e.g. std::map).
	 * \tparam T The property type
	 */
	template <typename T>
	class TypedMapDataStructureHandler<T,
		typename std::enable_if_t<HasMapSemantics_v<T>>> final : public IMapDataStructureHandler
	{
		using KeyType = typename T::key_type;
		using ValueType = typename T::mapped_type;

		static_assert(has_MapErase_v<T>
			&& has_clear_v<T>
			&& has_size_v<T>,
			"In order to use Map-style reflection access, your type has to implement the following members with the same signature-style as std::map:"
			"a key_type typedef, a mapped_type typedef, operator[], begin, end, find, an std::pair-like iterator type, erase, clear, size.");

	public:
		virtual const void* Read(const void* pMap, const DIRE_STRING_VIEW& pKey) const override;

		virtual void		Update(void* pMap, const DIRE_STRING_VIEW& pKey, const void* pNewData) const override;

		virtual void*		Create(void* pMap, const DIRE_STRING_VIEW& pKey, const void* pInitData) const override;

		virtual void*		BinaryCreate(void* pMap, void const* pKey, const void* pInitData) const override;

		virtual bool		Erase(void* pMap, const DIRE_STRING_VIEW& pKey) const override;

		virtual void		Clear(void* pMap) const override;

		virtual size_t		Size(const void* pMap) const override;

		virtual DataStructureHandler	ValueDataHandler() const override;

		virtual ReflectableID			ValueReflectableID() const override;

		virtual MetaType				KeyMetaType() const override
		{
			return FromActualTypeToEnumType<KeyType>::EnumType;
		}

		virtual MetaType				ValueMetaType() const override
		{
			return FromActualTypeToEnumType<ValueType>::EnumType;
		}

		virtual size_t					SizeofKey() const override
		{
			return sizeof(KeyType);
		}

		virtual size_t					SizeofValue() const override
		{
			return sizeof(ValueType);
		}

		virtual DataStructureHandler	KeyDataHandler() const override;

#ifdef DIRE_SERIALIZATION_ENABLED
		virtual void		SerializeForEachPair(OpaqueMapType pMap, OpaqueSerializerType pSerializer, KeyValuePairSerializeFptr pForEachPairCallback) const override
		{
			if (pMap != nullptr)
			{
				const T* thisMap = static_cast<const T*>(pMap);
				DataStructureHandler mapKeyHandler = KeyDataHandler();
				DataStructureHandler mapValueHandler = ValueDataHandler();
				for (auto it = thisMap->begin(); it != thisMap->end(); ++it)
				{
					pForEachPairCallback(pSerializer, &it->first, &it->second, GetInstance(), mapKeyHandler, mapValueHandler);
				}
			}
		}
		virtual DIRE_STRING	KeyToString(OpaqueKeyType pKey) const override
		{
			auto& keyRef = *static_cast<const KeyType*>(pKey);
			return DIRE_NS::String::to_string(keyRef);
		}
#endif

		static const TypedMapDataStructureHandler & GetInstance()
		{
			static TypedMapDataStructureHandler instance{};
			return instance;
		}
	};
}

#include "DireMapDataStructureHandler.inl"
