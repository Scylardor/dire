#pragma once

#include "DireMacros.h"
#include "DireString.h"

#include <string.h> // strcmp
#include <type_traits> // enable_if

namespace DIRE_NS
{
	class Enum
	{

	};

	template <typename T>
	constexpr bool IsEnum = std::is_enum_v<T> || std::is_base_of_v<Enum, T>;
}

#define DIRE_SEQUENTIAL_ENUM(EnumName, UnderlyingType, ...) \
	class EnumName : public DIRE_NS::Enum \
	{\
	public:\
		using Underlying = UnderlyingType; \
		enum Values : UnderlyingType\
		{\
			__VA_ARGS__\
		};\
		Values Value{};\
		EnumName() = default;\
		EnumName(Values pInitVal) :\
			Value(pInitVal)\
		{}\
		EnumName& operator=(EnumName const& pOther)\
		{\
			Value = pOther.Value;\
			return *this;\
		}\
		EnumName& operator=(Values const pVal)\
		{\
			Value = pVal;\
			return *this;\
		}\
		bool operator!=(EnumName const& pOther) const\
		{\
			return (Value != pOther.Value);\
		}\
		bool operator!=(Values const pVal) const\
		{\
			return (Value == pVal);\
		}\
		bool operator==(EnumName const& pOther) const\
		{\
			return !(*this != pOther);\
		}\
		bool operator==(Values const pVal) const\
		{\
			return (Value == pVal);\
		}\
		explicit operator Values() const\
		{\
			return Value;\
		}\
		friend DIRE_STRING	to_string(EnumName const& pSelf)\
		{\
			return DIRE_STRING(GetStringFromSafeEnum(pSelf.Value));\
		}\
		const char*	GetString() const\
		{\
			return GetStringFromSafeEnum(Value);\
		}\
		/* This one assumes that the parameter value is guaranteed to be within bounds and thus doesn't even bounds check. */\
		static char const*	GetStringFromSafeEnum(Values enumValue) \
		{ \
			return nameEnumPairs[(int)enumValue].first; \
		}\
		/* We have the guarantee that linear enums values start from 0 and increase 1 by 1 so we can make this function O(1).*/ \
		static char const*	GetStringFromEnum(Values enumValue) \
		{ \
			if( (int)enumValue >= DIRE_NARGS(__VA_ARGS__))\
			{\
				return nullptr;\
			}\
			return GetStringFromSafeEnum(enumValue);\
		}\
		static Values const*	GetValueFromString(char const* enumStr) \
		{ \
			for (auto const& pair : nameEnumPairs)\
			{\
				if (strcmp(pair.first, enumStr) == 0)\
					return &pair.second;\
			}\
			return nullptr;\
		}\
		template <typename F>\
		static void	Enumerate(F&& pEnumerator)\
		{\
			for (UnderlyingType i = 0; i < DIRE_NARGS(__VA_ARGS__); ++i)\
				pEnumerator((Values) i);\
		}\
		static const char*	GetTypeName()\
		{\
			return DIRE_STRINGIZE(EnumName);\
		}\
	private:\
		inline static std::pair<const char*, Values> nameEnumPairs[]{DIRE_VA_MACRO(DIRE_BRACES_STRINGIZE_COMMA_VALUE, __VA_ARGS__)}; \
	};
	//std::ostream& operator<<(std::ostream& pOs, const EnumName& pEnum)\
	//{\
	//	pOs << pEnum.GetString();\
	//	return pOs;\
	//}

namespace DIRE_NS
{
#ifdef _MSC_VER // Microsoft compilers

#pragma intrinsic(_BitScanForward)

	// https://learn.microsoft.com/en-us/cpp/intrinsics/bitscanforward-bitscanforward64?view=msvc-170
	template <typename T>
	constexpr ::std::enable_if_t<::std::is_integral_v<T>, unsigned long>
	FindFirstSetBit(T value)
	{
		if (value == 0)
			return UINT32_MAX;

		unsigned long firstSetBitIndex = 0;
		if constexpr (sizeof(T) > sizeof(unsigned long))
		{
			_BitScanForward64(&firstSetBitIndex, (unsigned __int64)value);
		}
		else
		{
			_BitScanForward(&firstSetBitIndex, (unsigned long)value);
		}

		return firstSetBitIndex;
	}
#elif defined(__GNUG__) || defined(__clang__) // https://stackoverflow.com/questions/28166565/detect-gcc-as-opposed-to-msvc-clang-with-macro
	// https://gcc.gnu.org/onlinedocs/gcc/Other-Builtins.html
	// https://clang.llvm.org/docs/LanguageExtensions.html
	// TODO: untested
	template <typename T>
	constexpr ::std::enable_if_t<::std::is_integral_v<T>, unsigned long>
	FindFirstSetBit(T value)
	{
		if (value == 0)
			return UINT32_MAX;

		unsigned long firstSetBitIndex = 0;
		if constexpr (sizeof(T) <= sizeof(int))
		{
			firstSetBitIndex = (unsigned long)__builtin_ffs((int)value);
		}
		else if constexpr (sizeof(T) <= sizeof(long)) // probably useless nowadays
		{
			firstSetBitIndex = (unsigned long)__builtin_ffsl((long)value);
		}
		else
		{
			firstSetBitIndex = (unsigned long)__builtin_ffsll((long long)value);
		}

		return firstSetBitIndex;
	}
#endif
}




// This idea of storing a "COUNTER_BASE" has been stolen from https://stackoverflow.com/a/52770279/1987466
#define DIRE_BITMASK_ENUM(EnumName, UnderlyingType, ...) \
	class EnumName : public DIRE_NS::Enum\
	{ \
		private:\
		enum LocalCounter_t { COUNTER_BASE = __COUNTER__, OFFSET = (COUNTER_BASE == 0 ? 0 : 1) };\
	public:\
		using Underlying = UnderlyingType; \
		enum Values : UnderlyingType\
		{\
			DIRE_VA_MACRO(DIRE_COMMA_ARGS_LSHIFT_COUNTER, __VA_ARGS__)\
		};\
		EnumName() = default;\
		EnumName(Values pInitVal) :\
			Value(pInitVal)\
		{}\
		EnumName& operator=(EnumName const& pOther)\
		{\
			Value = pOther.Value;\
			return *this;\
		}\
		EnumName& operator=(Values const pVal)\
		{\
			Value = pVal;\
			return *this;\
		}\
		bool operator!=(EnumName const& pOther) const\
		{\
			return (Value != pOther.Value);\
		}\
		bool operator!=(Values const pVal) const\
		{\
			return (Value == pVal);\
		}\
		bool operator==(EnumName const& pOther) const\
		{\
			return !(*this != pOther);\
		}\
		bool operator==(Values const pVal) const\
		{\
			return (Value == pVal);\
		}\
		operator Values() const\
		{\
			return Value;\
		}\
		Values	Value{};\
		friend DIRE_STRING	to_string(EnumName const& pSelf)\
		{\
			return DIRE_STRING(GetStringFromSafeEnum(pSelf.Value));\
		}\
		void SetBit(Values pSetBit)\
		{\
			Value = (Values)((Underlying)Value | (Underlying)pSetBit);\
		}\
		void ClearBit(Values pClearedBit)\
		{\
			Value = (Values)((Underlying)Value & ~((Underlying)pClearedBit));\
		}\
		void Clear()\
		{\
			Value = (Values)(Underlying)0;\
		}\
		bool IsBitSet(Values pBit) const\
		{\
			return ((Underlying)Value & (Underlying)pBit) != 0;\
		}\
		const char*	GetString() const\
		{\
			return GetStringFromSafeEnum(Value);\
		}\
		/*This one assumes that the parameter value is guaranteed to be within bounds and thus doesn't even bounds check. */\
		static char const*	GetStringFromSafeEnum(Values pEnumValue) \
		{ \
			return nameEnumPairs[DIRE_NS::FindFirstSetBit((UnderlyingType)pEnumValue)].first; \
		}\
		/* We have the guarantee that bitflags enums values are powers of 2 so we can make this function O(1).*/ \
		static char const*	GetStringFromEnum(Values pEnumValue)\
		{ \
			/* If value is 0 OR not a power of 2 OR bigger than our biggest power of 2, it cannot be valid */\
			const bool isPowerOf2 = ((UnderlyingType)pEnumValue & ((UnderlyingType)pEnumValue - 1)) == 0;\
			if ((UnderlyingType)pEnumValue == 0 || !isPowerOf2 || (UnderlyingType)pEnumValue >= (1 << DIRE_NARGS(__VA_ARGS__)))\
			{\
				return nullptr;\
			}\
			return GetStringFromSafeEnum(pEnumValue);\
		}\
		static Values const*	GetValueFromString(char const* enumStr) \
		{ \
			for (auto const& pair : nameEnumPairs)\
			{\
				if (strcmp(pair.first, enumStr) == 0)\
					return &pair.second;\
			}\
			return nullptr;\
		}\
		template <typename F>\
		static void	Enumerate(F&& pEnumerator)\
		{\
			for (UnderlyingType i = 0; i < DIRE_NARGS(__VA_ARGS__); ++i)\
				pEnumerator((Values)(1 << i));\
		}\
		static const char*	GetTypeName()\
		{\
			return DIRE_STRINGIZE(EnumName);\
		}\
	private:\
		inline static std::pair<const char*, Values> nameEnumPairs[] {DIRE_VA_MACRO(DIRE_BRACES_STRINGIZE_COMMA_VALUE, __VA_ARGS__)};\
	};
	//std::ostream& operator<<(std::ostream& pOs, const EnumName& pEnum)\
	//{\
	//	pOs << pEnum.GetString();\
	//	return pOs;\
	//}


#define DIRE_ENUM(EnumName, ...) DIRE_SEQUENTIAL_ENUM(EnumName, int, __VA_ARGS__)
