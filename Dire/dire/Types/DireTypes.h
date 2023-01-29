#pragma once

#include "dire/DireEnums.h"
#include "dire/Utils/DireMacros.h"
#include "dire/Utils/DireTypeTraits.h"

namespace DIRE_NS
{
	DIRE_SEQUENTIAL_ENUM(MetaType, uint8_t,
		Unknown,
		Void,
		Bool,
		Int,
		Uint,
		Int64,
		Uint64,
		Float,
		Double,
		Array,
		Map,
		Enum,
		Object,
		Char,
		UChar,
		Short,
		UShort,
		Reference
	);

	template <MetaType::Values T>
	struct FromEnumTypeToActualType;

	template <typename T, typename Enable = void>
	struct FromActualTypeToEnumType
	{
		// If the type was not registered, by default it's unknown
		static const MetaType::Values EnumType = MetaType::Unknown;
	};

	template <typename T>
	constexpr MetaType::Values	FromEnumToUnderlyingType()
	{
		static_assert(IsEnum<T>);

		switch (sizeof(T))
		{
		case sizeof(int8_t):
			return MetaType::Char;
		case sizeof(int16_t):
			return MetaType::Short;
		case sizeof(int32_t):
			return MetaType::Int;
		case sizeof(int64_t):
			return MetaType::Int64;
		default:
			return MetaType::Unknown;
		}
	}

#define DECLARE_TYPE_TRANSLATOR(TheEnumType, TheActualType) \
	template <> \
	struct FromEnumTypeToActualType<TheEnumType> \
	{ \
		using ActualType = TheActualType; \
	}; \
	template <typename T>\
	struct FromActualTypeToEnumType<T, typename ::std::enable_if_t<std::is_same_v<T, TheActualType>>>\
	{\
		static const MetaType::Values EnumType = TheEnumType;\
	};

#define DECLARE_ENABLE_IF_TRANSLATOR(TheEnumType, Condition) \
	template <typename T>\
	struct FromActualTypeToEnumType<T, typename ::std::enable_if_t<Condition>>\
	{\
		static const MetaType::Values EnumType = TheEnumType;\
	};

	DECLARE_TYPE_TRANSLATOR(MetaType::Void, void)
	DECLARE_TYPE_TRANSLATOR(MetaType::Bool, bool)
	DECLARE_TYPE_TRANSLATOR(MetaType::Int, int)
	DECLARE_TYPE_TRANSLATOR(MetaType::Uint, unsigned)
	DECLARE_TYPE_TRANSLATOR(MetaType::Int64, int64_t)
	DECLARE_TYPE_TRANSLATOR(MetaType::Uint64, uint64_t)
	DECLARE_TYPE_TRANSLATOR(MetaType::Float, float)
	DECLARE_TYPE_TRANSLATOR(MetaType::Double, double)
	DECLARE_TYPE_TRANSLATOR(MetaType::Char, char)
	DECLARE_TYPE_TRANSLATOR(MetaType::UChar, unsigned char)
	DECLARE_TYPE_TRANSLATOR(MetaType::Short, short)
	DECLARE_TYPE_TRANSLATOR(MetaType::UShort, unsigned short)

	DECLARE_ENABLE_IF_TRANSLATOR(MetaType::Array, HasArraySemantics_v<T>)
	DECLARE_ENABLE_IF_TRANSLATOR(MetaType::Map, HasMapSemantics_v<T>)
	DECLARE_ENABLE_IF_TRANSLATOR(MetaType::Object, std::is_class_v<T> && !IsEnum<T> && !HasArraySemantics_v<T> && !HasMapSemantics_v<T>)
	DECLARE_ENABLE_IF_TRANSLATOR(FromEnumToUnderlyingType<T>(), std::is_enum_v<T>)
	DECLARE_ENABLE_IF_TRANSLATOR(MetaType::Enum, (std::is_base_of_v<Enum, T>))
	DECLARE_ENABLE_IF_TRANSLATOR(MetaType::Reference, std::is_reference_v<T>)

}
