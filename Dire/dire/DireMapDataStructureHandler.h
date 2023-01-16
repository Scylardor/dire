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
		using MapReadFptr = const void * (*)(const void *, DIRE_STRING_VIEW);
		using MapUpdateFptr = void (*)(void*, DIRE_STRING_VIEW, const void *);
		using MapCreateFptr = void* (*)(void*, DIRE_STRING_VIEW, const void *);
		using MapBinaryCreateFptr = void* (*)(void*, void const*, const void *);
		using MapEraseFptr = bool	(*)(void*, DIRE_STRING_VIEW);
		using MapClearFptr = void	(*)(void*);
		using MapSizeFptr = size_t(*)(const void *);
		using MapValueHandlerFptr = DataStructureHandler(*)();
		using MapElementReflectableIDFptr = ReflectableID (*)();
		using MapKeyTypeFptr = Type(*)();
		using MapValueTypeFptr = MapKeyTypeFptr;
		using MapSizeofKeyFptr = size_t(*)();
		using MapSizeofValueFptr = MapSizeofKeyFptr;
		using MapKeyDataHandlerFptr = DataStructureHandler(*)();

		MapReadFptr		Read = nullptr;
		MapUpdateFptr	Update = nullptr;
		MapCreateFptr	Create = nullptr;
		MapBinaryCreateFptr	BinaryKeyCreate = nullptr;
		MapEraseFptr	Erase = nullptr;
		MapClearFptr	Clear = nullptr;
		MapSizeFptr		Size = nullptr;
		MapValueHandlerFptr			ValueHandler = nullptr;
		MapElementReflectableIDFptr	ValueReflectableID = nullptr;
		MapKeyTypeFptr		GetKeyType = nullptr;
		MapValueTypeFptr	GetValueType = nullptr;
		MapSizeofKeyFptr	SizeofKey = nullptr;
		MapSizeofValueFptr	SizeofValue = nullptr;
		MapKeyDataHandlerFptr	GetKeyDataHandler = nullptr;

#if DIRE_USE_SERIALIZATION
		using OpaqueSerializerType = void*;
		using OpaqueMapType = const void *;
		using OpaqueKeyType = const void *;
		using OpaqueValueType = OpaqueKeyType;
		using KeyValuePairSerializeFptr = void (*)(OpaqueSerializerType, OpaqueKeyType, OpaqueValueType, const MapDataStructureHandler &, const DataStructureHandler &, const DataStructureHandler &);
		using MapSerializeForeachPairFptr = void (*)(OpaqueMapType, OpaqueSerializerType, KeyValuePairSerializeFptr);
		using MapSerializeKeyToStringFptr = DIRE_STRING (*)(OpaqueKeyType);
		MapSerializeForeachPairFptr	SerializeForEachPair = nullptr;
		MapSerializeKeyToStringFptr	KeyToString = nullptr;
#endif
	};

	template <typename T, typename = void>
	struct TypedMapDataStructureHandler
	{};


	// Base class for reflectable properties that have brackets operator to be able to make operations on the underlying array
	template <typename T>
	struct TypedMapDataStructureHandler<T,
		typename std::enable_if_t<HasMapSemantics_v<T>>> : MapDataStructureHandler
	{
		using KeyType = typename T::key_type;
		using ValueType = typename T::mapped_type;

		TypedMapDataStructureHandler()
		{
			Read = &MapRead;
			Update = &MapUpdate;
			Create = &MapCreate;
			BinaryKeyCreate = &MapBinaryKeyCreate;
			Erase = &MapErase;
			Clear = &MapClear;
			Size = &MapSize;
			ValueHandler = &MapValueHandler;
			ValueReflectableID = &MapElementReflectableID;
			GetKeyType = &MapGetKeyType;
			GetValueType = &MapGetValueType;
			SizeofKey = [] { return sizeof(KeyType); };
			SizeofValue = [] { return sizeof(ValueType); };
			GetKeyDataHandler = &MapKeyHandler;

#if DIRE_USE_SERIALIZATION
			SerializeForEachPair = &MapSerializeForEachPair;
			KeyToString = &MapKeyToString;
#endif
		}

		static_assert(has_MapErase_v<T>
			&& has_clear_v<T>
			&& has_size_v<T>,
			"In order to use Map-style reflection access, your type has to implement the following members with the same signature-style as std::map:"
			"a key_type typedef, a mapped_type typedef, operator[], begin, end, find, an std::pair-like iterator type, erase, clear, size.");

		static const void * MapRead(const void * pMap, DIRE_STRING_VIEW pKey)
		{
			const T * thisMap = static_cast<const T *>(pMap);
			if (thisMap == nullptr)
			{
				return nullptr;
			}
			const KeyType key = DIRE_NS::FromCharsConverter<KeyType>::Convert(pKey);
			auto it = thisMap->find(key);
			if (it == thisMap->end())
			{
				return nullptr;
			}
			return &it->second;
		}

		static void	MapUpdate(void* pMap, DIRE_STRING_VIEW pKey, const void * pNewData)
		{
			if (pMap == nullptr)
			{
				return;
			}

			ValueType* valuePtr = const_cast<ValueType*>((const ValueType *)MapRead(pMap, pKey));
			if (valuePtr == nullptr) // key not found
			{
				return;
			}

			auto* newValue = static_cast<const ValueType *>(pNewData);
			(*valuePtr) = *newValue;
		}

		static void* MapCreate(void* pMap, DIRE_STRING_VIEW pKey, const void * pInitData)
		{
			if (pMap == nullptr)
			{
				return nullptr;
			}

			T* thisMap = static_cast<T*>(pMap);
			const KeyType key = DIRE_NS::FromCharsConverter<KeyType>::Convert(pKey);

			if (pInitData == nullptr)
			{
				auto [it, inserted] = thisMap->insert({ key, ValueType() });
				return &it->second;
			}

			const ValueType & initValue = *static_cast<const ValueType *>(pInitData);
			auto [it, inserted] = thisMap->insert({ key, initValue });
			return &it->second;
		}

		static void* MapBinaryKeyCreate(void* pMap, const void * pKey, const void * pInitData)
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

		static bool	MapErase(void* pMap, DIRE_STRING_VIEW pKey)
		{
			if (pMap != nullptr)
			{
				T* thisMap = static_cast<T*>(pMap);
				const KeyType key = FromCharsConverter<KeyType>::Convert(pKey);

				return thisMap->erase(key);
			}

			return false;
		}

		static void	MapClear(void* pMap)
		{
			if (pMap != nullptr)
			{
				T* thisMap = static_cast<T*>(pMap);
				thisMap->clear();
			}
		}

		static size_t	MapSize(const void * pMap)
		{
			if (pMap != nullptr)
			{
				const T * thisMap = static_cast<const T *>(pMap);
				return thisMap->size();
			}

			return 0;
		}

		static DataStructureHandler MapKeyHandler()
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

		static DataStructureHandler MapValueHandler()
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

		static ReflectableID MapElementReflectableID()
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

		static Type	MapGetKeyType()
		{
			return FromActualTypeToEnumType<KeyType>::EnumType;
		}

		static Type	MapGetValueType()
		{
			return FromActualTypeToEnumType<ValueType>::EnumType;
		}

#if DIRE_USE_SERIALIZATION
		static void	MapSerializeForEachPair(const OpaqueMapType pMap, OpaqueSerializerType pSerializer, KeyValuePairSerializeFptr pForEachPairCallback)
		{
			if (pMap != nullptr)
			{
				const T * thisMap = static_cast<const T *>(pMap);
				DataStructureHandler mapKeyHandler = MapKeyHandler();
				DataStructureHandler mapValueHandler = MapValueHandler();
				for (auto it = thisMap->begin(); it != thisMap->end(); ++it)
				{
					pForEachPairCallback(pSerializer, &it->first, &it->second, GetInstance(), mapKeyHandler, mapValueHandler);
				}
			}
		}

		static std::string	MapKeyToString(OpaqueKeyType pKey)
		{
			auto& keyRef = *static_cast<const KeyType*>(pKey);
			return DIRE_NS::to_string(keyRef);
		}
#endif

		static const TypedMapDataStructureHandler & GetInstance()
		{
			static TypedMapDataStructureHandler instance{};
			return instance;
		}
	};

}
