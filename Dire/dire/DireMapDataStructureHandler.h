#pragma once

#include "DireMacros.h"
#include "DireTypes.h"
#include "DireString.h"
#include "DireTypeTraits.h"

namespace DIRE_NS
{
	class DataStructureHandler;

	struct MapDataStructureHandler
	{
		using MapReadFptr = void const* (*)(void const*, DIRE_STRING_VIEW);
		using MapUpdateFptr = void (*)(void*, DIRE_STRING_VIEW, void const*);
		using MapCreateFptr = void* (*)(void*, DIRE_STRING_VIEW, void const*);
		using MapBinaryCreateFptr = void* (*)(void*, void const*, void const*);
		using MapEraseFptr = void	(*)(void*, DIRE_STRING_VIEW);
		using MapClearFptr = void	(*)(void*);
		using MapSizeFptr = size_t(*)(void const*);
		using MapValueHandlerFptr = DataStructureHandler(*)();
		using MapElementReflectableIDFptr = unsigned (*)();
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
		using OpaqueMapType = void const*;
		using OpaqueKeyType = void const*;
		using OpaqueValueType = OpaqueKeyType;
		using KeyValuePairSerializeFptr = void (*)(OpaqueSerializerType, OpaqueKeyType, OpaqueValueType, MapDataStructureHandler const&, DataStructureHandler const&, DataStructureHandler const&);
		using MapSerializeForeachPairFptr = void (*)(OpaqueMapType, OpaqueSerializerType, KeyValuePairSerializeFptr);
		using MapSerializeKeyToStringFptr = std::string(*)(OpaqueKeyType); // TODO: customize string type
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

		static void const* MapRead(void const* pMap, std::string_view pKey)
		{
			T const* thisMap = static_cast<T const*>(pMap);
			if (thisMap == nullptr)
			{
				return nullptr;
			}

			KeyType const key = ReflectorConversions::from_chars<KeyType>(pKey);
			auto it = thisMap->find(key);
			if (it == thisMap->end())
			{
				return nullptr;
			}
			return &it->second;
		}

		static void	MapUpdate(void* pMap, std::string_view pKey, void const* pNewData)
		{
			if (pMap == nullptr)
			{
				return;
			}

			ValueType* valuePtr = const_cast<ValueType*>((ValueType const*)MapRead(pMap, pKey));
			if (valuePtr == nullptr) // key not found
			{
				return;
			}

			auto* newValue = static_cast<ValueType const*>(pNewData);
			(*valuePtr) = *newValue;
		}

		static void* MapCreate(void* pMap, std::string_view pKey, void const* pInitData)
		{
			if (pMap == nullptr)
			{
				return nullptr;
			}

			T* thisMap = static_cast<T*>(pMap);
			KeyType const key = ReflectorConversions::from_chars<KeyType>(pKey);

			if (pInitData == nullptr)
			{
				auto [it, inserted] = thisMap->insert({ key, ValueType() });
				return &it->second;
			}

			ValueType const& initValue = *static_cast<ValueType const*>(pInitData);
			auto [it, inserted] = thisMap->insert({ key, initValue });
			return &it->second;
		}

		static void* MapBinaryKeyCreate(void* pMap, void const* pKey, void const* pInitData)
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

		static void	MapErase(void* pMap, std::string_view pKey)
		{
			if (pMap != nullptr)
			{
				T* thisMap = static_cast<T*>(pMap);
				KeyType const key = ReflectorConversions::from_chars<KeyType>(pKey);

				thisMap->erase(key);
			}
		}

		static void	MapClear(void* pMap)
		{
			if (pMap != nullptr)
			{
				T* thisMap = static_cast<T*>(pMap);
				thisMap->clear();
			}
		}

		static size_t	MapSize(void const* pMap)
		{
			if (pMap != nullptr)
			{
				T const* thisMap = static_cast<T const*>(pMap);
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

		static unsigned MapElementReflectableID() // TODO: ReflectableID
		{
			if constexpr (std::is_base_of_v<Reflectable2, ValueType>)
			{
				return ValueType::GetClassReflectableTypeInfo().GetID();
			}
			else
			{
				return INVALID_REFLECTABLE_ID;
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
				T const* thisMap = static_cast<T const*>(pMap);
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
			return ReflectorConversions::to_string(keyRef);
		}
#endif

		static TypedMapDataStructureHandler const& GetInstance()
		{
			static TypedMapDataStructureHandler instance{};
			return instance;
		}
	};

}
