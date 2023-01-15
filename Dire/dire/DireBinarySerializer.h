#pragma once

#if DIRE_USE_SERIALIZATION && DIRE_USE_BINARY_SERIALIZATION

#include "DireTypes.h"
#include "DireSerialization.h"
#include "DireReflectable.h"

#include <string.h> // memcpy


namespace DIRE_NS
{
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