#pragma once

#include "DireTypes.h"
#include "DireString.h"
#include "DireTypeTraits.h"
#include "DireReflectableID.h"

namespace DIRE_NS
{
	class DataStructureHandler;

	struct MapDataStructureHandler
	{
		virtual const void*				Read(const void*, const DIRE_STRING_VIEW&) const = 0;
		virtual void					Update(void*, const DIRE_STRING_VIEW&, const void*) const = 0;
		virtual void*					Create(void*, const DIRE_STRING_VIEW&, const void*) const = 0;
		virtual void*					BinaryCreate(void*, void const*, const void*) const = 0;
		virtual bool					Erase(void*, const DIRE_STRING_VIEW&) const = 0;
		virtual void					Clear(void*) const = 0;
		virtual size_t					Size(const void*) const = 0;
		virtual DataStructureHandler	ValueDataHandler() const = 0;
		virtual ReflectableID			ValueReflectableID() const = 0;
		virtual Type					KeyMetaType() const = 0;
		virtual Type					ValueMetaType() const = 0;
		virtual size_t					SizeofKey() const = 0;
		virtual size_t					SizeofValue() const = 0;
		virtual DataStructureHandler	KeyDataHandler() const = 0;

		virtual ~MapDataStructureHandler() = default;


#if DIRE_USE_SERIALIZATION
		using OpaqueSerializerType = void*;
		using OpaqueMapType = const void *;
		using OpaqueKeyType = const void *;
		using OpaqueValueType = OpaqueKeyType;
		using KeyValuePairSerializeFptr = void (*)(OpaqueSerializerType, OpaqueKeyType, OpaqueValueType, const MapDataStructureHandler &, const DataStructureHandler &, const DataStructureHandler &);
		virtual void		SerializeForEachPair(OpaqueMapType, OpaqueSerializerType, KeyValuePairSerializeFptr) const = 0;
		virtual DIRE_STRING	KeyToString(OpaqueKeyType) const = 0;
#endif
	};

	template <typename T, typename = void>
	struct TypedMapDataStructureHandler
	{};


	// Base class for reflectable properties that have brackets operator to be able to make operations on the underlying array
	template <typename T>
	struct TypedMapDataStructureHandler<T,
		typename std::enable_if_t<HasMapSemantics_v<T>>> final : MapDataStructureHandler
	{
		using KeyType = typename T::key_type;
		using ValueType = typename T::mapped_type;

		static_assert(has_MapErase_v<T>
			&& has_clear_v<T>
			&& has_size_v<T>,
			"In order to use Map-style reflection access, your type has to implement the following members with the same signature-style as std::map:"
			"a key_type typedef, a mapped_type typedef, operator[], begin, end, find, an std::pair-like iterator type, erase, clear, size.");

		virtual const void* Read(const void* pMap, const DIRE_STRING_VIEW& pKey) const override
		{
			const T* thisMap = static_cast<const T*>(pMap);
			if (thisMap == nullptr)
			{
				return nullptr;
			}
			const ConvertResult<KeyType> key = DIRE_NS::FromCharsConverter<KeyType>::Convert(pKey);
			if (key.HasError())
				return nullptr;

			auto it = thisMap->find(key.GetValue());
			if (it == thisMap->end())
			{
				return nullptr;
			}
			return &it->second;
		}

		virtual void					Update(void* pMap, const DIRE_STRING_VIEW& pKey, const void* pNewData) const override
		{
			if (pMap == nullptr)
			{
				return;
			}

			ValueType* valuePtr = const_cast<ValueType*>((const ValueType*)Read(pMap, pKey));
			if (valuePtr == nullptr) // key not found
			{
				return;
			}

			auto* newValue = static_cast<const ValueType*>(pNewData);
			(*valuePtr) = *newValue;
		}

		virtual void* Create(void* pMap, const DIRE_STRING_VIEW& pKey, const void* pInitData) const override
		{
			if (pMap == nullptr)
			{
				return nullptr;
			}

			T* thisMap = static_cast<T*>(pMap);
			const ConvertResult<KeyType> key = DIRE_NS::FromCharsConverter<KeyType>::Convert(pKey);
			if (key.HasError())
				return nullptr;

			if (pInitData == nullptr)
			{
				auto [it, inserted] = thisMap->insert({ key.GetValue(), ValueType() });
				return &it->second;
			}

			const ValueType& initValue = *static_cast<const ValueType*>(pInitData);
			auto [it, inserted] = thisMap->insert({ key.GetValue(), initValue });
			return &it->second;
		}

		virtual void* BinaryCreate(void* pMap, void const* pKey, const void* pInitData) const override
		{
			if (pMap == nullptr)
			{
				return nullptr;
			}

			T* thisMap = static_cast<T*>(pMap);
			KeyType const& key = *static_cast<const KeyType*>(pKey);

			if (pInitData == nullptr)
			{
				auto [it, inserted] = thisMap->insert({ key, ValueType() });
				return &it->second;
			}

			ValueType const& initValue = *static_cast<ValueType const*>(pInitData);
			auto [it, inserted] = thisMap->insert({ key, initValue });
			return &it->second;
		}

		virtual bool					Erase(void* pMap, const DIRE_STRING_VIEW& pKey) const override
		{
			if (pMap != nullptr)
			{
				T* thisMap = static_cast<T*>(pMap);
				const ConvertResult<KeyType> key = FromCharsConverter<KeyType>::Convert(pKey);
				if (key.HasError())
					return false;

				return thisMap->erase(key.GetValue());
			}

			return false;
		}

		virtual void					Clear(void* pMap) const override
		{
			if (pMap != nullptr)
			{
				T* thisMap = static_cast<T*>(pMap);
				thisMap->clear();
			}
		}

		virtual size_t					Size(const void* pMap) const override
		{
			if (pMap != nullptr)
			{
				const T* thisMap = static_cast<const T*>(pMap);
				return thisMap->size();
			}

			return 0;
		}

		virtual DataStructureHandler	ValueDataHandler() const override
		{
			if constexpr (HasMapSemantics_v<ValueType>)
			{
				return DataStructureHandler(&TypedMapDataStructureHandler<ValueType>::GetInstance());
			}
			else if constexpr (HasArraySemantics_v<ValueType>)
			{
				return DataStructureHandler(&TypedArrayDataStructureHandler<ValueType>::GetInstance());
			}
			else if constexpr (std::is_base_of_v<Enum, ValueType>)
			{
				return DataStructureHandler(&TypedEnumDataStructureHandler<ValueType>::GetInstance());
			}

			return {};
		}

		virtual ReflectableID			ValueReflectableID() const override
		{
			if constexpr (std::is_base_of_v<Reflectable2, ValueType>)
			{
				return ValueType::GetClassReflectableTypeInfo().GetID();
			}
			else
			{
				return DIRE_NS::INVALID_REFLECTABLE_ID;
			}
		}

		virtual Type					KeyMetaType() const override
		{
			return FromActualTypeToEnumType<KeyType>::EnumType;
		}

		virtual Type					ValueMetaType() const override
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

		virtual DataStructureHandler	KeyDataHandler() const override
		{
			if constexpr (HasMapSemantics_v<KeyType>)
			{
				return DataStructureHandler(&TypedMapDataStructureHandler<KeyType>::GetInstance());
			}
			else if constexpr (HasArraySemantics_v<KeyType>)
			{
				return DataStructureHandler(&TypedArrayDataStructureHandler<KeyType>::GetInstance());
			}
			else if constexpr (std::is_base_of_v<Enum, KeyType>)
			{
				return DataStructureHandler(&TypedEnumDataStructureHandler<KeyType>::GetInstance());
			}

			return {};
		}

#if DIRE_USE_SERIALIZATION
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
