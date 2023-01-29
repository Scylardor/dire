#pragma once

#include "dire/Types/DireTypes.h"
#include "dire/Utils/DireString.h"
#include "dire/Utils/DireTypeTraits.h"
#include "dire/DireReflectableID.h"

namespace DIRE_NS
{
	class DataStructureHandler;

	class Dire_EXPORT IMapDataStructureHandler
	{
	public:

		IMapDataStructureHandler() = default;
		virtual ~IMapDataStructureHandler() = default;
		IMapDataStructureHandler(const IMapDataStructureHandler&) = default;
		IMapDataStructureHandler& operator=(const IMapDataStructureHandler&) = default;

		virtual const void*				Read(const void*, const DIRE_STRING_VIEW&) const = 0;
		virtual void					Update(void*, const DIRE_STRING_VIEW&, const void*) const = 0;
		virtual void*					Create(void*, const DIRE_STRING_VIEW&, const void*) const = 0;
		virtual void*					BinaryCreate(void*, void const*, const void*) const = 0;
		virtual bool					Erase(void*, const DIRE_STRING_VIEW&) const = 0;
		virtual void					Clear(void*) const = 0;
		virtual size_t					Size(const void*) const = 0;
		virtual DataStructureHandler	ValueDataHandler() const = 0;
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
		virtual void		SerializeForEachPair(OpaqueMapType, OpaqueSerializerType, KeyValuePairSerializeFptr) const = 0;
		virtual DIRE_STRING	KeyToString(OpaqueKeyType) const = 0;
#endif
	};

	template <typename T, typename = void>
	class TypedMapDataStructureHandler
	{};


	// Base class for reflectable properties that have brackets operator to be able to make operations on the underlying array
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
