#pragma once

#include "DireDefines.h"

#ifdef DIRE_COMPILE_BINARY_SERIALIZATION
#include "DireSerialization.h"
#include "DireTypes.h"


namespace DIRE_NS
{
	class DataStructureHandler;
	class IMapDataStructureHandler;
	class IArrayDataStructureHandler;

	class BinaryReflectorDeserializer : public IDeserializer
	{
	public:
		virtual Result	DeserializeInto(char const* pSerialized, Reflectable& pDeserializedObject) override;

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

		void	DeserializeArrayValue(void* pPropPtr, const IArrayDataStructureHandler * pArrayHandler) const;

		void	DeserializeMapValue(void* pPropPtr, const IMapDataStructureHandler * pMapHandler) const;

		void	DeserializeCompoundValue(void* pPropPtr) const;

		void	DeserializeValue(Type pPropType, void* pPropPtr, const DataStructureHandler* pHandler = nullptr) const;

		const char*		mySerializedBytes = nullptr;
		mutable size_t	myReadingOffset = 0;
	};
}
#endif