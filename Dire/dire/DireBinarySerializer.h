#pragma once

#if DIRE_USE_SERIALIZATION && DIRE_SERIALIZATION_BINARY_SERIALIZER_ENABLED

#include "DireTypes.h"
#include "DireSerialization.h"
#include "DireReflectable.h"

#include <string.h> // memcpy

#define BINARY_SERIALIZE_VALUE_CASE(TypeEnum) \
case Type::TypeEnum:\
{\
	using ActualType = FromEnumTypeToActualType<Type::TypeEnum>::ActualType;\
	WriteAsBytes<ActualType>(*static_cast<const ActualType*>(pPropPtr)); \
	break;\
}

namespace DIRE_NS
{
	class BinarySerializationHeaders
	{
		friend class BinaryReflectorSerializer;
		friend class BinaryReflectorDeserializer;

		struct Object
		{
			Object(const uint32_t pID) :
				ReflectableID(pID)
			{}

			uint32_t	ReflectableID = 0; // TODO: Reflectable typedef
			uint32_t	PropertiesCount = 0; // TODO: customizable property count
		};

		struct Property
		{
			Property(const Type pType, const uint32_t pOffset) :
				PropertyType(pType), PropertyOffset(pOffset)
			{}

			Type		PropertyType = Type::Unknown;
			uint32_t	PropertyOffset = 0; // TODO: customizable offset size
		};

		struct Array
		{
			Array(const Type pType, const size_t pSizeofElem, const size_t pArraySize) :
				ElementType(pType), ElementSize(pSizeofElem), ArraySize(pArraySize)
			{}

			Type		ElementType = Type::Unknown;  // TODO: maybe to remove because we actually have it in the array handler
			size_t		ElementSize = 0; // TODO: maybe to remove because we actually have it in the array handler
			size_t		ArraySize = 0;
		};

		struct Map
		{
			Map(const Type pKType, const size_t pSizeofKey, const Type pVType, const size_t pSizeofValue, const size_t pMapSize) :
				KeyType(pKType), SizeofKeyType(pSizeofKey), ValueType(pVType), SizeofValueType(pSizeofValue), MapSize(pMapSize)
			{}

			Type		KeyType = Type::Unknown;  // TODO: maybe to remove because we actually have it in the map handler
			size_t		SizeofKeyType = 0;	// TODO: maybe to remove because we actually have it in the map handler
			Type		ValueType = Type::Unknown;  // TODO: maybe to remove because we actually have it in the map handler
			size_t		SizeofValueType = 0;	// TODO: maybe to remove because we actually have it in the map handler
			size_t		MapSize = 0;
		};
	};

	class BinaryReflectorSerializer : public ISerializer
	{
	public:

		virtual Result	Serialize(Reflectable2 const& serializedObject) override;

		virtual bool	SerializesMetadata() const override
		{
			return false;
		}

		virtual void	SerializeString(DIRE_STRING_VIEW pSerializedString) override;
		virtual void	SerializeInt(int32_t pSerializedInt) override;
		virtual void	SerializeFloat(float pSerializedFloat) override;
		virtual void	SerializeBool(bool pSerializedBool) override;
		virtual void	SerializeValuesForObject(DIRE_STRING_VIEW pObjectName, SerializedValueFiller pFillerFunction) override;

	private:

		template <typename T>
		class Handle
		{
		public:
			Handle(Result::ByteVector& pSerializedBytes, const size_t pOffset) :
				SerializedData(pSerializedBytes), Offset(pOffset)
			{}

			[[nodiscard]] T& Edit() const
			{
				return reinterpret_cast<T&>(SerializedData[Offset]);
			}

		private:
			Result::ByteVector& SerializedData;
			size_t	Offset = 0;
		};

		template <typename T, typename... Args>
		Handle<T>	WriteAsBytes(Args&&... pArgs)
		{
			auto oldSize = mySerializedBuffer.size();
			mySerializedBuffer.resize(mySerializedBuffer.size() + sizeof T);
			new (&mySerializedBuffer[oldSize]) T(std::forward<Args>(pArgs)...);
			return Handle<T>(mySerializedBuffer, oldSize);
		}

		void	WriteRawBytes(const char* pBytes, const size_t pNbBytes)
		{
			auto oldSize = mySerializedBuffer.size();
			mySerializedBuffer.resize(mySerializedBuffer.size() + pNbBytes);
			memcpy(&mySerializedBuffer[oldSize], pBytes, pNbBytes);
		}

		void	SerializeValue(Type pPropType, void const* pPropPtr, DataStructureHandler const* pHandler = nullptr);

		void	SerializeArrayValue(void const* pPropPtr, ArrayDataStructureHandler const* pArrayHandler);

		void	SerializeMapValue(void const* pPropPtr, MapDataStructureHandler const* pMapHandler);

		void	SerializeCompoundValue(void const* pPropPtr);

		Result::ByteVector	mySerializedBuffer;
	};
}
#endif