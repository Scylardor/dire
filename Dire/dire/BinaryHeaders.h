#pragma once

#if DIRE_USE_SERIALIZATION && DIRE_USE_BINARY_SERIALIZATION

#include <cstdint>
#include "DireTypes.h"

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
}

#endif
