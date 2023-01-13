#pragma once

#if DIRE_USE_SERIALIZATION && DIRE_SERIALIZATION_BINARY_SERIALIZER_ENABLED

#define BINARY_DESERIALIZE_VALUE_CASE(TypeEnum) \
case Type::TypeEnum:\
{\
	using ActualType = FromEnumTypeToActualType<Type::TypeEnum>::ActualType;\
	*static_cast<ActualType*>(pPropPtr) = ReadFromBytes<ActualType>();\
	break;\
}

namespace DIRE_NS
{
	class BinaryReflectorDeserializer : public IDeserializer
	{
	public:
		virtual void	DeserializeInto(char const* pSerialized, Reflectable2& pDeserializedObject) override;

	private:

		template <typename T>
		const T& ReadFromBytes() const
		{
			auto Tptr = reinterpret_cast<const T*>(mySerializedBytes + myReadingOffset);
			myReadingOffset += sizeof T;
			return *Tptr;
		}

		const char* ReadBytes(size_t pNbBytesRead) const
		{
			const char* dataPtr = mySerializedBytes + myReadingOffset;
			myReadingOffset += pNbBytesRead;
			return dataPtr;
		}

		void	DeserializeArrayValue(void* pPropPtr, ArrayDataStructureHandler const* pArrayHandler) const;

		void	DeserializeMapValue(void* pPropPtr, MapPropertyCRUDHandler const* pMapHandler) const;

		void	DeserializeCompoundValue(void* pPropPtr) const;

		void	DeserializeValue(Type pPropType, void* pPropPtr, AbstractPropertyHandler const* pHandler = nullptr) const
		{
			switch (pPropType)
			{
				BINARY_DESERIALIZE_VALUE_CASE(Bool)
					BINARY_DESERIALIZE_VALUE_CASE(Char)
					BINARY_DESERIALIZE_VALUE_CASE(UChar)
					BINARY_DESERIALIZE_VALUE_CASE(Short)
					BINARY_DESERIALIZE_VALUE_CASE(UShort)
					BINARY_DESERIALIZE_VALUE_CASE(Int)
					BINARY_DESERIALIZE_VALUE_CASE(Uint)
					BINARY_DESERIALIZE_VALUE_CASE(Int64)
					BINARY_DESERIALIZE_VALUE_CASE(Uint64)
					BINARY_DESERIALIZE_VALUE_CASE(Float)
					BINARY_DESERIALIZE_VALUE_CASE(Double)
			case Type::Array:
				if (pHandler != nullptr)
				{
					DeserializeArrayValue(pPropPtr, pHandler->ArrayHandler);
				}
				break;
			case Type::Map:
				if (pHandler != nullptr)
				{
					DeserializeMapValue(pPropPtr, pHandler->MapHandler);
				}
				break;
			case Type::Object:
				DeserializeCompoundValue(pPropPtr);
				break;
			case Type::Enum:
			{
				Type underlyingType = pHandler->EnumHandler->EnumType();
				DeserializeValue(underlyingType, pPropPtr);
			}
			break;
			default:
				std::cerr << "Unmanaged type in SerializeValue!";
				assert(false); // TODO: for now
			}
		}

		const char* mySerializedBytes = nullptr;
		mutable size_t	myReadingOffset = 0;
	};
}
#endif