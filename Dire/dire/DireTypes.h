#pragma once

#include "DireEnums.h"
#include "DireMacros.h"
#include "DireTypeTraits.h"

namespace DIRE_NS
{
	DIRE_SEQUENTIAL_ENUM(Type, uint8_t,
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
		UShort
	);

	template <Type::Values T>
	struct FromEnumTypeToActualType;

	template <typename T, typename Enable = void>
	struct FromActualTypeToEnumType
	{
		// If the type was not registered, by default it's unknown
		static const Type::Values EnumType = Type::Unknown;
	};

#define DECLARE_TYPE_TRANSLATOR(TheEnumType, TheActualType) \
	template <> \
	struct FromEnumTypeToActualType<TheEnumType> \
	{ \
		using ActualType = TheActualType; \
	}; \
	template <typename T>\
	struct FromActualTypeToEnumType<T, typename std::enable_if_t<std::is_same_v<T, TheActualType>>>\
	{\
		static const Type::Values EnumType = TheEnumType;\
	};

	DECLARE_TYPE_TRANSLATOR(Type::Void, void)
	DECLARE_TYPE_TRANSLATOR(Type::Bool, bool)
	DECLARE_TYPE_TRANSLATOR(Type::Int, int)
	DECLARE_TYPE_TRANSLATOR(Type::Uint, unsigned)
	DECLARE_TYPE_TRANSLATOR(Type::Int64, int64_t)
	DECLARE_TYPE_TRANSLATOR(Type::Uint64, uint64_t)
	DECLARE_TYPE_TRANSLATOR(Type::Float, float)
	DECLARE_TYPE_TRANSLATOR(Type::Double, double)
	DECLARE_TYPE_TRANSLATOR(Type::Char, char)
	DECLARE_TYPE_TRANSLATOR(Type::UChar, unsigned char)
	DECLARE_TYPE_TRANSLATOR(Type::Short, short)
	DECLARE_TYPE_TRANSLATOR(Type::UShort, unsigned short)

	template <typename T>
	constexpr Type	FromEnumToUnderlyingType()
	{
		static_assert(IsEnum<T>);

		switch (sizeof T)
		{
			case sizeof(int8_t):
				return Type::Char;
			case sizeof(int16_t):
				return Type::Short;
			case sizeof(int32_t):
				return Type::Int;
			case sizeof(int64_t):
				return Type::Int64;
			default:
				return Type::Unknown;
		}
	}

	template <typename T>
	struct FromActualTypeToEnumType<T, typename std::enable_if_t<std::is_enum_v<T>, void>>
	{
		static const Type::Values EnumType = FromEnumToUnderlyingType<T>();
	};

	template <typename T>
	struct FromActualTypeToEnumType<T, typename std::enable_if_t<std::is_base_of_v<Enum, T>, void>>
	{
		static const Type::Values EnumType = Type::Enum;
	};

}