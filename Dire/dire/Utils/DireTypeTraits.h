#pragma once

#include <type_traits>
#include "DireMacros.h"

#ifdef _MSC_VER
namespace DIRE_NS
{
	template< class... Args>
	using void_t = std::void_t<Args...>;
}
#else
namespace DIRE_NS
{
	template< class... Args>
	using void_t = std::__void_t<Args...>;
}
#endif

// Inspired by https://stackoverflow.com/a/30848101/1987466
#include <type_traits>


// Inspired by https://stackoverflow.com/a/71574097/1987466
// Useful for member function names like "operator[]" that would break the build if used as is
#define MEMBER_FUNCTION_DETECTOR(FunctionName) \
	template <typename T>\
	using DIRE_##FunctionName##_t = decltype(std::declval<T>().FunctionName());\
	template <typename T>\
	inline constexpr bool has_##FunctionName##_v = detect<T, DIRE_##FunctionName##_t>{};

#define MEMBER_FUNCTION_ALIAS_DETECTOR_PARAM1(Alias, FunctionName, Param1) \
	template <typename T>\
	using Alias##_t = decltype(std::declval<T>().FunctionName(Param1));\
	template <typename T>\
	inline constexpr bool has_##Alias##_v = detect<T, Alias##_t>{};

#define MEMBER_FUNCTION_DETECTOR_PARAM1(FunctionName, Param1) \
	MEMBER_FUNCTION_ALIAS_DETECTOR_PARAM1(FunctionName, FunctionName, Param1)

#define MEMBER_FUNCTION_DETECTOR_PARAM2(FunctionName, Param1, Param2) \
	template <typename T>\
	using DIRE_##FunctionName##_t = decltype(std::declval<T>().FunctionName(Param1, Param2));\
	template <typename T>\
	inline constexpr bool has_##FunctionName##_v = detect<T, DIRE_##FunctionName##_t>{};

// Inspired by https://stackoverflow.com/a/62203640/1987466
#define DECLARE_HAS_TYPE_DETECTOR(DetectorName, DetectedType) \
template <class T, class = void>\
struct DetectorName\
{\
	using type = std::false_type;\
};\
template <class T>\
struct DetectorName <T, std::void_t<typename T::DetectedType >>\
{\
	using type = std::true_type;\
};\
template <class T>\
inline constexpr bool DetectorName##_v = DetectorName<T>::type::value;

namespace DIRE_NS
{
	// Primary template handles all types not supporting the operation.
	template <typename, template <typename> class, typename = DIRE_NS::void_t<>>
	struct detect : std::false_type {};

	// Specialization recognizes/validates only types supporting the archetype.
	template <typename T, template <typename> class Op>
	struct detect<T, Op, DIRE_NS::void_t<Op<T>>> : std::true_type {};

	MEMBER_FUNCTION_ALIAS_DETECTOR_PARAM1(ArrayBrackets, operator[], 0)
	MEMBER_FUNCTION_ALIAS_DETECTOR_PARAM1(MapBrackets, operator[], typename T::key_type())

	MEMBER_FUNCTION_DETECTOR_PARAM2(insert, std::declval<T>().begin(), std::declval<T>()[0])
	MEMBER_FUNCTION_DETECTOR_PARAM1(erase, std::declval<T>().begin())
	MEMBER_FUNCTION_ALIAS_DETECTOR_PARAM1(MapErase, erase, typename T::key_type())
	MEMBER_FUNCTION_DETECTOR(clear)
	MEMBER_FUNCTION_DETECTOR(size)

	DECLARE_HAS_TYPE_DETECTOR(HasValueType, value_type)
	DECLARE_HAS_TYPE_DETECTOR(HasKeyType, key_type)
	DECLARE_HAS_TYPE_DETECTOR(HasMappedType, mapped_type)

	template<typename T>
	inline constexpr bool HasMapSemantics_v = HasKeyType_v<T> && HasValueType_v<T> && HasMappedType_v<T> && has_MapBrackets_v<T>;

	template<typename T>
	using HasMapSemantics_t = std::enable_if_t<HasMapSemantics_v<T> >;

	template<typename T>
	inline constexpr bool HasArraySemantics_v = (std::is_array_v<T> || (has_ArrayBrackets_v<T> && HasValueType_v<T>)) && !HasMapSemantics_v<T>;

	template<typename T>
	using HasArraySemantics_t = std::enable_if_t<HasArraySemantics_v<T>>;

	// 
	/**
	 * \brief Holds a complicated ADL contraption in order to be able to declare the "Self" type in a Reflectable class.
	 *	From https://stackoverflow.com/a/70701479/1987466
	 */
	namespace SelfType
	{
		template <typename T>
		struct Reader
		{
			friend auto adl_GetSelfType(Reader<T>);
		};

		template <typename T, typename U>
		struct Writer
		{
			friend auto adl_GetSelfType(Reader<T>) { return U{}; }
		};

		inline void adl_GetSelfType() {}

		template <typename T>
		using Read = std::remove_pointer_t<decltype(adl_GetSelfType(Reader<T>{})) > ;
	}
}
