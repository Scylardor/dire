#pragma once

#include "DireMacros.h"
#include "DireString.h"
#include <string.h> // strcmp

#define DIRE_SEQUENTIAL_ENUM(EnumName, UnderlyingType, ...) \
	class EnumName : public Enum \
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
		bool operator!=(EnumName const& pOther)\
		{\
			return (Value != pOther.Value);\
		}\
		bool operator!=(Values const pVal)\
		{\
			return (Value == pVal);\
		}\
		bool operator==(EnumName const& pOther)\
		{\
			return !(*this != pOther);\
		}\
		bool operator==(Values const pVal)\
		{\
			return (Value == pVal);\
		}\
		operator Values() const\
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

// This idea of storing a "COUNTER_BASE" has been stolen from https://stackoverflow.com/a/52770279/1987466
#define DIRE_BITMASK_ENUM(EnumName, UnderlyingType, ...) \
	class EnumName : public Enum { \
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
		bool operator!=(EnumName const& pOther)\
		{\
			return (Value != pOther.Value);\
		}\
		bool operator!=(Values const pVal)\
		{\
			return (Value == pVal);\
		}\
		bool operator==(EnumName const& pOther)\
		{\
			return !(*this != pOther);\
		}\
		bool operator==(Values const pVal)\
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
			return nameEnumPairs[FindFirstSetBit((UnderlyingType)pEnumValue)].first; \
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


#define DIRE_ENUM(EnumName, ...) DIRE_SEQUENTIAL_ENUM(EnumName, int, __VA_ARGS__)
