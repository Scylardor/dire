#pragma once
#include "DireDefines.h"

#ifdef DIRE_SERIALIZATION_ENABLED
#include "dire/Utils/DireString.h"
#include "dire/Types/DireTypeInfo.h"
#include "dire/DireReflectableID.h"
#include <vector>
#include <variant>

namespace DIRE_NS
{
	class Reflectable;

	using SerializationError = DIRE_STRING;

	class Dire_EXPORT ISerializer
	{
	public:
		struct Result
		{
			static_assert(sizeof(std::byte) == sizeof(char) == 1);
			using ByteVector = std::vector<std::byte, DIRE_ALLOCATOR<std::byte>>;

			Dire_EXPORT Result() = default;

			Dire_EXPORT Result(const char* pBuffer, size_t pBufferSize) :
				Value(std::in_place_type_t<ByteVector>{}, (const std::byte*)pBuffer, (const std::byte*)pBuffer + pBufferSize)
			{}

			Dire_EXPORT Result(const SerializationError& pError) :
				Value(pError)
			{}

			Result(ByteVector&& pMovedVec) :
				Value(std::move(pMovedVec))
			{}

			Dire_EXPORT [[nodiscard]] DIRE_STRING AsString() const;

			Dire_EXPORT operator DIRE_STRING() const { return AsString();}

			const ByteVector& GetBytes() const { return std::get<ByteVector>(Value); }

			[[nodiscard]] bool	HasError() const { return std::holds_alternative<SerializationError>(Value); }

			[[nodiscard]] SerializationError	GetError() const;

		private:
			std::variant<ByteVector, SerializationError>	Value;
		};

		virtual ~ISerializer() = default;

		virtual Result	Serialize(Reflectable const& serializedObject) = 0;

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
			Dire_EXPORT Result() = default;

			Dire_EXPORT Result(const SerializationError& pError) :
				Value(pError)
			{}

			Dire_EXPORT Result(Reflectable* pDeserializedReflectable) :
				Value(pDeserializedReflectable)
			{}

			template <typename T = Reflectable>
			[[nodiscard]] T* GetReflectable() const
			{
				static_assert(std::is_base_of_v<Reflectable, T>);
				return (T*)std::get<Reflectable*>(Value);
			}

			[[nodiscard]] bool	HasError() const { return std::holds_alternative<SerializationError>(Value); }

			[[nodiscard]] SerializationError	GetError() const;

		private:
			std::variant<Reflectable*, SerializationError>	Value{nullptr};
		};

		Dire_EXPORT virtual ~IDeserializer() = default;

		template <typename T, typename... Args>
		Result Deserialize(const char* pSerialized, Args&&... pArgs);

		template <typename... Args>
		Result Deserialize(const char* pSerialized, ReflectableID pReflectableClassID, Args&&... pArgs);

		Dire_EXPORT virtual Result	DeserializeInto(const char* pSerialized, Reflectable& pDeserializedObject) = 0;
	};

	template <typename T, typename ... Args>
	IDeserializer::Result IDeserializer::Deserialize(const char* pSerialized, Args&&... pArgs)
	{
		static_assert(std::is_base_of_v<Reflectable, T>, "Deserialize is only able to process Reflectable-derived classes");
		T* deserializedReflectable = AllocateReflectable<T>(std::forward<Args>(pArgs)...);
		Result result = DeserializeInto(pSerialized, *deserializedReflectable);

		if (result.HasError())
			delete deserializedReflectable;

		return result;
	}

	template <typename ... Args>
	IDeserializer::Result  IDeserializer::Deserialize(const char* pSerialized, ReflectableID pReflectableClassID, Args&&... pArgs)
	{
		const TypeInfo * typeInfo = TypeInfoDatabase::GetSingleton().GetTypeInfo(pReflectableClassID);
		if (typeInfo == nullptr) // bad ID
		{
			return nullptr;
		}

		Reflectable* deserializedReflectable = TypeInfoDatabase::GetSingleton().TryInstantiate(pReflectableClassID, std::tuple<Args...>(std::forward<Args>(pArgs)...));

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