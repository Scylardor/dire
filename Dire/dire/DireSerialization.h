#pragma once

#if DIRE_USE_SERIALIZATION

#include <vector>
#include "DireString.h"
#include "DireTypeInfo.h"

namespace DIRE_NS
{
	class Reflectable2;

	class ISerializer
	{
	public:
		struct Result
		{
			using ByteVector = std::vector<std::byte>;

			Result() = default;

			Result(const char* pBuffer, size_t pBufferSize)
			{
				SerializedBuffer.resize(pBufferSize);
				memcpy(SerializedBuffer.data(), pBuffer, sizeof(char) * pBufferSize);
			}

			Result(ByteVector&& pMovedVec) :
				SerializedBuffer(std::move(pMovedVec))
			{}

			[[nodiscard]] DIRE_STRING AsString() const // TODO: customize string
			{
				DIRE_STRING serializedString;
				serializedString.resize(SerializedBuffer.size());
				memcpy(serializedString.data(), SerializedBuffer.data(), sizeof(std::byte) * SerializedBuffer.size());
				return serializedString;
			}

			operator DIRE_STRING() const
			{
				return AsString();
			}

		private:
			ByteVector	SerializedBuffer;
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
		virtual ~IDeserializer() = default;

		template <typename T, typename... Args>
		T* Deserialize(const char* pSerialized, Args&&... pArgs)
		{
			static_assert(std::is_base_of_v<Reflectable2, T>, "Deserialize is only able to process Reflectable-derived classes");
			T* deserializedReflectable = new T(std::forward<Args>(pArgs)...); // TODO: allow customization of new
			DeserializeInto(pSerialized, *deserializedReflectable);
			return deserializedReflectable;
		}

		template <typename... Args>
		Reflectable2* Deserialize(const char* pSerialized, unsigned pReflectableClassID, Args&&... pArgs)
		{
			const TypeInfo * typeInfo = Reflector3::GetSingleton().GetTypeInfo(pReflectableClassID);
			if (typeInfo == nullptr) // bad ID
			{
				return nullptr;
			}

			Reflectable2* deserializedReflectable =
				Reflector3::GetSingleton().TryInstantiate(pReflectableClassID, std::tuple<Args...>(std::forward<Args>(pArgs)...));

			if (deserializedReflectable)
			{
				DeserializeInto(pSerialized, *deserializedReflectable);
			}

			return deserializedReflectable;
		}

		virtual void	DeserializeInto(const char* pSerialized, Reflectable2& pDeserializedObject) = 0;
	};
}
#endif