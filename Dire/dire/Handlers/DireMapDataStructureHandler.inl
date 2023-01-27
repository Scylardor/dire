#pragma once

namespace DIRE_NS
{


	template <typename T>
	const void* TypedMapDataStructureHandler<T, std::enable_if_t<HasMapSemantics_v<T>, void>>::Read(const void* pMap, const std::string_view& pKey) const
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

	template <typename T>
	void TypedMapDataStructureHandler<T, std::enable_if_t<HasMapSemantics_v<T>, void>>::Update(void* pMap, const std::string_view& pKey, const void* pNewData) const
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

	template <typename T>
	void* TypedMapDataStructureHandler<T, std::enable_if_t<HasMapSemantics_v<T>, void>>::Create(void* pMap, const std::string_view& pKey, const void* pInitData) const
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

	template <typename T>
	void* TypedMapDataStructureHandler<T, std::enable_if_t<HasMapSemantics_v<T>, void>>::BinaryCreate(void* pMap, void const* pKey, const void* pInitData) const
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

	template <typename T>
	bool TypedMapDataStructureHandler<T, std::enable_if_t<HasMapSemantics_v<T>, void>>::Erase(void* pMap, const std::string_view& pKey) const
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

	template <typename T>
	void TypedMapDataStructureHandler<T, std::enable_if_t<HasMapSemantics_v<T>, void>>::Clear(void* pMap) const
	{
		if (pMap != nullptr)
		{
			T* thisMap = static_cast<T*>(pMap);
			thisMap->clear();
		}
	}

	template <typename T>
	size_t TypedMapDataStructureHandler<T, std::enable_if_t<HasMapSemantics_v<T>, void>>::Size(const void* pMap) const
	{
		if (pMap != nullptr)
		{
			const T* thisMap = static_cast<const T*>(pMap);
			return thisMap->size();
		}

		return 0;
	}

	template <typename T>
	DataStructureHandler TypedMapDataStructureHandler<T, std::enable_if_t<HasMapSemantics_v<T>, void>>::ValueDataHandler() const
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
		else
			return {};
	}

	template <typename T>
	ReflectableID TypedMapDataStructureHandler<T, std::enable_if_t<HasMapSemantics_v<T>, void>>::ValueReflectableID() const
	{
		if constexpr (std::is_base_of_v<Reflectable, ValueType>)
		{
			return ValueType::GetTypeInfo().GetID();
		}
		else
		{
			return DIRE_NS::INVALID_REFLECTABLE_ID;
		}
	}

	template <typename T>
	DataStructureHandler TypedMapDataStructureHandler<T, std::enable_if_t<HasMapSemantics_v<T>, void>>::KeyDataHandler() const
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
		else
			return {};
	}
}
