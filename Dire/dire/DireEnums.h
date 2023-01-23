#pragma once

#include "Utils/DireMacros.h"
#include "Utils/DireString.h"

#include <type_traits> // enable_if

namespace DIRE_NS
{

	class Enum
	{};

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
		EnumName& operator=(const EnumName & pOther)\
		{\
			Value = pOther.Value;\
			return *this;\
		}\
		EnumName& operator=(const Values pVal)\
		{\
			Value = pVal;\
			return *this;\
		}\
		bool operator!=(const EnumName & pOther) const\
		{\
			return (Value != pOther.Value);\
		}\
		bool operator!=(const Values  pVal) const\
		{\
			return (Value != pVal);\
		}\
		bool operator==(const EnumName & pOther) const\
		{\
			return !(*this != pOther);\
		}\
		bool operator==(const Values pVal) const\
		{\
			return (Value == pVal);\
		}\
		explicit operator Values() const\
		{\
			return Value;\
		}\
		friend DIRE_STRING	to_string(const EnumName & pSelf)\
		{\
			return GetStringFromSafeEnum(pSelf.Value);\
		}\
		const char*	GetString() const\
		{\
			return GetStringFromSafeEnum(Value);\
		}\
		/* This one assumes that the parameter value is guaranteed to be within bounds and thus doesn't even bounds check. */\
		static const char *	GetStringFromSafeEnum(Values enumValue) \
		{ \
			return nameEnumPairs[(int)enumValue].first; \
		}\
		/* We have the guarantee that linear enums values start from 0 and increase 1 by 1 so we can make this function O(1).*/ \
		static const char *	GetStringFromEnum(Values enumValue) \
		{ \
			if( (int)enumValue >= DIRE_NARGS(__VA_ARGS__))\
			{\
				return nullptr;\
			}\
			return GetStringFromSafeEnum(enumValue);\
		}\
		static const Values *	GetValueFromString(const char * enumStr) \
		{ \
			for (auto const& pair : nameEnumPairs)\
			{\
				if (strcmp(pair.first, enumStr) == 0)\
					return &pair.second;\
			}\
			return nullptr;\
		}\
		static Values	GetValueFromSafeString(DIRE_STRING_VIEW enumStr) \
		{ \
			for (auto const& pair : nameEnumPairs)\
			{\
				if (enumStr == pair.first)\
					return pair.second;\
			}\
			/* should never happen!*/\
			DIRE_ASSERT(false);\
			return Values();\
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
		enum LocalCounter_t { COUNTER_BASE = __COUNTER__, OFFSET = (1) };\
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
		EnumName(Underlying pInitVal) :\
			Value((Values)pInitVal)\
		{}\
		EnumName& operator=(const EnumName & pOther)\
		{\
			Value = pOther.Value;\
			return *this;\
		}\
		EnumName& operator=(const Values  pVal)\
		{\
			Value = pVal;\
			return *this;\
		}\
		EnumName& operator|=(const Values pVal)\
		{\
			Value = (Values) ((Underlying)Value | (Underlying)pVal); \
			return *this; \
		}\
		EnumName operator|(const Values pVal) const\
		{\
			return (Values) ((Underlying)Value | (Underlying)pVal); \
		}\
		EnumName& operator&=(const Values  pVal)\
		{\
			Value = (Values) ((Underlying)Value & (Underlying)pVal); \
			return *this; \
		}\
		EnumName operator&(const Values pVal) const\
		{\
			return (Values) ((Underlying)Value & (Underlying)pVal); \
		}\
		EnumName& operator<<=(unsigned pLShift)\
		{\
			Value = (Values) ((Underlying)Value << pLShift); \
			return *this; \
		}\
		EnumName operator<<(unsigned pLShift) const\
		{\
			return (Values) ((Underlying)Value << pLShift); \
		}\
		EnumName& operator>>=(unsigned pRShift)\
		{\
			Value = (Values) ((Underlying)Value >> pRShift); \
			return *this; \
		}\
		EnumName operator>>(unsigned pRShift) const\
		{\
			return (Values) ((Underlying)Value >> pRShift); \
		}\
		EnumName& operator^=(const Values pVal)\
		{\
			Value = (Values) ((Underlying)Value ^ (Underlying)pVal); \
			return *this; \
		}\
		EnumName operator^(const Values pVal) const\
		{\
			return (Values) ((Underlying)Value ^ (Underlying)pVal); \
		}\
		EnumName operator~() const\
		{\
			return (Values) ((Underlying)~Value);\
		}\
		bool operator!=(const EnumName & pOther) const\
		{\
			return (Value != pOther.Value);\
		}\
		bool operator!=(const Values  pVal) const\
		{\
			return (Value != pVal);\
		}\
		bool operator==(const EnumName & pOther) const\
		{\
			return !(*this != pOther);\
		}\
		bool operator==(const Values  pVal) const\
		{\
			return (Value == pVal);\
		}\
		operator Values() const\
		{\
			return Value;\
		}\
		Values	Value{};\
		friend DIRE_STRING	to_string(const EnumName & pSelf)\
		{\
			return GetStringFromSafeEnum(pSelf.Value);\
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
		DIRE_STRING	GetString() const\
		{\
			return BuildStringFromEnum(Value);\
		}\
		/*This one assumes that the parameter value is guaranteed to be within bounds and thus doesn't even bounds check. */\
		static const char*	GetStringFromSafeEnum(Values pEnumValue) \
		{ \
			return nameEnumPairs[DIRE_NS::FindFirstSetBit((UnderlyingType)pEnumValue)].first; \
		}\
		/* We have the guarantee that bitflags enums values are powers of 2 so we can make this function O(1).*/ \
		static DIRE_STRING	GetStringFromEnum(Values pEnumValue)\
		{ \
			/* If value is 0, it cannot be valid */\
			if ((UnderlyingType)pEnumValue == 0)\
			{\
				return "invalid";\
			}\
			return GetStringFromSafeEnum(pEnumValue);\
		}\
		static const Values GetValueFromString(DIRE_STRING_VIEW enumStr) \
		{ \
			/* Happy path: there's no pipe in the string, directly search values */\
			if (enumStr.find('|') == enumStr.npos)\
			{\
				for (const auto& pair : nameEnumPairs)\
				{\
					if (enumStr == pair.first)\
						return pair.second;\
				}\
				return (Values)0;\
			}\
			else\
			{\
				/* Find each ' | ', then search each token separately*/\
				size_t tokenPos = 0;\
				Underlying val = 0;\
				while (tokenPos != enumStr.npos)\
				{\
					DIRE_STRING_VIEW token;\
					size_t pipePos = enumStr.find(" | ", tokenPos);\
					if (pipePos != enumStr.npos)\
						token = enumStr.substr(tokenPos, pipePos - tokenPos);\
					else\
						token = enumStr.substr(tokenPos, enumStr.length() - tokenPos);\
					Underlying oldVal = val;\
					for (const auto& pair : nameEnumPairs)\
					{\
						if (token == pair.first)\
						{\
							val = val | pair.second;\
							break;\
						}\
					}\
					if (oldVal == val) /* not found? error*/\
						return (Values)0;\
					if (pipePos != enumStr.npos)\
						tokenPos = pipePos + 3;\
					else\
						tokenPos = enumStr.npos;\
				}\
				return (Values)val;\
			}\
		}\
		static Values	GetValueFromSafeString(DIRE_STRING_VIEW enumStr) \
		{ \
			return GetValueFromString(enumStr);\
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
		static DIRE_STRING	BuildStringFromEnum(Values pEnumValue)\
		{ \
			/* If value is 0, it cannot be valid */\
			if ((UnderlyingType)pEnumValue == 0)\
			{\
				return "invalid";\
			}\
			DIRE_STRING oredValues;\
			for (unsigned i = DIRE_NS::FindFirstSetBit((UnderlyingType)pEnumValue); i < DIRE_NARGS(__VA_ARGS__); ++i)\
			{\
				const auto& enumNamePair = nameEnumPairs[i];\
				if ((UnderlyingType)pEnumValue & (UnderlyingType)enumNamePair.second)\
				{\
					if (!oredValues.empty())\
						oredValues += " | ";\
					oredValues += enumNamePair.first;\
				}\
			}\
			return oredValues;\
		}\
		inline static std::pair<const char*, Values> nameEnumPairs[] {DIRE_VA_MACRO(DIRE_BRACES_STRINGIZE_COMMA_VALUE, __VA_ARGS__)};\
	};
	//std::ostream& operator<<(std::ostream& pOs, const EnumName& pEnum)\
	//{\
	//	pOs << pEnum.GetString();\
	//	return pOs;\
	//}


#define DIRE_ENUM(EnumName, ...) DIRE_SEQUENTIAL_ENUM(EnumName, int, __VA_ARGS__)
