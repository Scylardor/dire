#pragma once

#ifdef DIRE_COMPILE_BINARY_SERIALIZATION

#include <cstdint>
#include "Dire/Types/DireTypes.h"

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

			ReflectableID	ReflectableID = 0;
			uint32_t		PropertiesCount = 0;
		};

		struct Property
		{
			Property(const MetaType pType, const uint32_t pOffset) :
				PropertyType(pType), PropertyOffset(pOffset)
			{}

			MetaType	PropertyType = MetaType::Unknown;
			uint32_t	PropertyOffset = 0;
		};

		struct Array
		{
			Array(const MetaType pType, const size_t pSizeofElem, const size_t pArraySize) :
				ElementType(pType), SizeofElement(pSizeofElem), ArraySize(pArraySize)
			{}

			// ElementType and SizeofElement are not strictly necessary for now, but let's keep them here for debug purposes
			MetaType	ElementType = MetaType::Unknown;
			size_t		SizeofElement = 0;
			size_t		ArraySize = 0;
		};

		struct Map
		{
			Map(const MetaType pKType, const size_t pSizeofKey, const MetaType pVType, const size_t pSizeofValue, const size_t pMapSize) :
				KeyType(pKType), SizeofKeyType(pSizeofKey), ValueType(pVType), SizeofValueType(pSizeofValue), MapSize(pMapSize)
			{}

			// Key/ValueType and Sizeofs are not strictly necessary for now, but let's keep them here for debug purposes
			// Maybe think of an "optimized" mode that omits them.
			MetaType	KeyType = MetaType::Unknown;
			size_t		SizeofKeyType = 0;
			MetaType	ValueType = MetaType::Unknown;
			size_t		SizeofValueType = 0;
			size_t		MapSize = 0;
		};
	};
}

#endif
