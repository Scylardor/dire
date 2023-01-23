#pragma once

#include "DireDefines.h"
#ifdef DIRE_COMPILE_BINARY_SERIALIZATION

#include "dire/Types/DireTypes.h"
#include "DireSerialization.h"
#include "dire/DireReflectable.h"

#include <string.h> // memcpy

namespace DIRE_NS
{
	class BinaryReflectorSerializer : public ISerializer
	{
	public:

		virtual Result	Dire_EXPORT Serialize(Reflectable const& serializedObject) override;

		virtual bool	Dire_EXPORT SerializesMetadata() const override { return false; }

		virtual void	Dire_EXPORT SerializeString(DIRE_STRING_VIEW pSerializedString) override;
		virtual void	Dire_EXPORT SerializeInt(int32_t pSerializedInt) override;
		virtual void	Dire_EXPORT SerializeFloat(float pSerializedFloat) override;
		virtual void	Dire_EXPORT SerializeBool(bool pSerializedBool) override;
		virtual void	Dire_EXPORT SerializeValuesForObject(DIRE_STRING_VIEW pObjectName, SerializedValueFiller pFillerFunction) override;

	private:

		template <typename T>
		class Handle
		{
		public:
			Handle(Result::ByteVector& pSerializedBytes, const size_t pOffset) :
				SerializedData(pSerializedBytes), Offset(pOffset)
			{}

			[[nodiscard]] T& Edit() const { return reinterpret_cast<T&>(SerializedData[Offset]); }

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

		void	SerializeArrayValue(void const* pPropPtr, IArrayDataStructureHandler const* pArrayHandler);

		void	SerializeMapValue(void const* pPropPtr, IMapDataStructureHandler const* pMapHandler);

		void	SerializeCompoundValue(void const* pPropPtr);

		Result::ByteVector	mySerializedBuffer;
	};
}
#endif