#pragma once
#include <iostream>

#if DIRE_USE_SERIALIZATION

#include <vector>
#include "DireString.h"
#include "DireTypeInfo.h"
#include <variant>
#include <optional>

namespace DIRE_NS
{
	class Reflectable2;

	using SerializationError = DIRE_STRING;

	class ISerializer
	{
	public:
		struct Result
		{
			static_assert(sizeof(std::byte) == sizeof(char) == 1);
			using ByteVector = std::vector<std::byte>;

			Result() = default;

			Result(const char* pBuffer, size_t pBufferSize) :
				Value(std::in_place_type_t<ByteVector>{}, (const std::byte*)pBuffer, (const std::byte*)pBuffer + pBufferSize)
			{}

			Result(const SerializationError& pError) :
				Value(pError)
			{}

			Result(ByteVector&& pMovedVec) :
				Value(std::move(pMovedVec))
			{}

			[[nodiscard]] DIRE_STRING AsString() const;

			operator DIRE_STRING() const { return AsString();}

			[[nodiscard]] bool	HasError() const { return std::holds_alternative<SerializationError>(Value); }

			[[nodiscard]] SerializationError	GetError() const;

		private:
			std::variant<ByteVector, SerializationError>	Value;
		};

		virtual ~ISerializer() = default;

		virtual Result	Serialize(Reflectable2 const& serializedObject) = 0;

		virtual bool	SerializesMetadata() const = 0;

		virtual void	SerializeString(DIRE_STRING_VIEW pSerializedString) = 0;
		virtual void	SerializeInt(int32_t pSerializedInt) = 0;
		virtual void	SerializeFloat(float pSerializedFloat) = 0;
		virtual void	SerializeBool(bool pSerializedBool) = 0;

		using SerializedValueFiller = void (*)(ISerializer& pSerializer);
		virtual void	SerializeValuesForObject(DIRE_STRING_VIEW pObjectName, SerializedValueFiller pFillerFunction) = 0;
	};

	class IDeserializer
	{
	public:
		struct Result
		{
			Result() = default;

			Result(const SerializationError& pError) :
				Value(pError)
			{}

			Result(Reflectable2* pDeserializedReflectable) :
				Value(pDeserializedReflectable)
			{}

			template <typename T = Reflectable2>
			[[nodiscard]] T* GetReflectable() const
			{
				static_assert(std::is_base_of_v<Reflectable2, T>);
				return (T*)std::get<Reflectable2*>(Value);
			}

			[[nodiscard]] bool	HasError() const { return std::holds_alternative<SerializationError>(Value); }

			[[nodiscard]] SerializationError	GetError() const;

		private:
			std::variant<Reflectable2*, SerializationError>	Value{nullptr};
		};

		virtual ~IDeserializer() = default;

		template <typename T, typename... Args>
		Result Deserialize(const char* pSerialized, Args&&... pArgs);

		template <typename... Args>
		Result Deserialize(const char* pSerialized, ReflectableID pReflectableClassID, Args&&... pArgs);

		virtual Result	DeserializeInto(const char* pSerialized, Reflectable2& pDeserializedObject) = 0;
	};

	template <typename T, typename ... Args>
	IDeserializer::Result IDeserializer::Deserialize(const char* pSerialized, Args&&... pArgs)
	{
		static_assert(std::is_base_of_v<Reflectable2, T>, "Deserialize is only able to process Reflectable-derived classes");
		T* deserializedReflectable = new T(std::forward<Args>(pArgs)...); // TODO: allow customization of new
		Result result = DeserializeInto(pSerialized, *deserializedReflectable);

		if (result.HasError())
			delete deserializedReflectable;

		return result;
	}

	template <typename ... Args>
	IDeserializer::Result  IDeserializer::Deserialize(const char* pSerialized, ReflectableID pReflectableClassID, Args&&... pArgs)
	{
		const TypeInfo * typeInfo = Reflector3::GetSingleton().GetTypeInfo(pReflectableClassID);
		if (typeInfo == nullptr) // bad ID
		{
			return nullptr;
		}

		Reflectable2* deserializedReflectable = Reflector3::GetSingleton().TryInstantiate(pReflectableClassID, std::tuple<Args...>(std::forward<Args>(pArgs)...));

		Result result;
		if (deserializedReflectable)
		{
			result = DeserializeInto(pSerialized, *deserializedReflectable);
		}

		if (result.HasError())
			delete deserializedReflectable;

		return result;
	}

	inline DIRE_STRING ISerializer::Result::AsString() const
	{
		const ByteVector* bytes = std::get_if<ByteVector>(&Value);
		if (bytes == nullptr)
			return "";

		DIRE_STRING serializedString((const char*)bytes->data(), (const char*)bytes->data() + bytes->size());
		return serializedString;
	}

	inline SerializationError ISerializer::Result::GetError() const
	{
		if (const SerializationError* error = std::get_if<SerializationError>(&Value))
			return *error;
		return "";
	}

}
#endif