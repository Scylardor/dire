#pragma once

#include <type_traits>
#include "DireMacros.h"

// Inspired by https://stackoverflow.com/a/71574097/1987466
// Useful for member function names like "operator[]" that would break the build if used as is
#define MEMBER_FUNCTION_ALIAS_DETECTOR_PARAM1(Alias, FunctionName, Param1) \
	template <typename ContainerType>\
	using Alias##_method_t = decltype(std::declval< ContainerType >().FunctionName(Param1));\
	template <typename ContainerType DIRE_COMMA typename = std::void_t<>>\
	struct has_##Alias : std::false_type {};\
	template <typename ContainerType>\
	struct has_##Alias<ContainerType DIRE_COMMA std::void_t<Alias##_method_t< ContainerType >>> :\
		std::true_type {};\
	template <typename ContainerType>\
	inline constexpr bool has_##Alias##_v = has_##Alias<ContainerType>::value;

#define MEMBER_FUNCTION_DETECTOR_PARAM1(FunctionName, Param1) \
	template <typename ContainerType>\
	using FunctionName##_method_t = decltype(std::declval< ContainerType >().FunctionName(Param1));\
	template <typename ContainerType DIRE_COMMA typename = std::void_t<>>\
	struct has_##FunctionName : std::false_type {};\
	template <typename ContainerType>\
	struct has_##FunctionName<ContainerType DIRE_COMMA std::void_t<FunctionName##_method_t< ContainerType >>> :\
		std::true_type {};\
	template <typename ContainerType>\
	inline constexpr bool has_##FunctionName##_v = has_##FunctionName<ContainerType>::value;

#define MEMBER_FUNCTION_DETECTOR_PARAM2(FunctionName, Param1, Param2) \
	template <typename ContainerType>\
	using FunctionName##_method_t = decltype(std::declval< ContainerType >().FunctionName(Param1, Param2));\
	template <typename ContainerType DIRE_COMMA typename = std::void_t<>>\
	struct has_##FunctionName : std::false_type {};\
	template <typename ContainerType>\
	struct has_##FunctionName<ContainerType DIRE_COMMA std::void_t<FunctionName##_method_t< ContainerType >>> :\
		std::true_type {};\
	template <typename ContainerType>\
	inline constexpr bool has_##FunctionName##_v = has_##FunctionName<ContainerType>::value;

#define MEMBER_FUNCTION_DETECTOR(FunctionName) \
	template <typename ContainerType>\
	using FunctionName##_method_t = decltype(std::declval< ContainerType >().FunctionName());\
	template <typename ContainerType DIRE_COMMA typename = std::void_t<>>\
	struct has_##FunctionName : std::false_type {};\
	template <typename ContainerType>\
	struct has_##FunctionName<ContainerType DIRE_COMMA std::void_t<FunctionName##_method_t< ContainerType >>> :\
		std::true_type {};\
	template <typename ContainerType>\
	inline constexpr bool has_##FunctionName##_v = has_##FunctionName<ContainerType>::value;

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
	MEMBER_FUNCTION_ALIAS_DETECTOR_PARAM1(ArrayBrackets, operator[], 0)
	MEMBER_FUNCTION_ALIAS_DETECTOR_PARAM1(MapBrackets, operator[], ContainerType::key_type())

	MEMBER_FUNCTION_DETECTOR_PARAM2(insert, std::declval<ContainerType>().begin(), std::declval<ContainerType>()[0])
	MEMBER_FUNCTION_DETECTOR_PARAM1(erase, std::declval<ContainerType>().begin())
	MEMBER_FUNCTION_ALIAS_DETECTOR_PARAM1(MapErase, erase, typename ContainerType::key_type())
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
	inline constexpr bool HasArraySemantics_v = std::is_array_v<T> || (has_ArrayBrackets_v<T> && HasValueType_v<T>) && !HasMapSemantics_v<T>;

	template<typename T>
	using HasArraySemantics_t = std::enable_if_t<HasArraySemantics_v<T>>;


	// Inspired by https://stackoverflow.com/a/59604594/1987466
	template<typename method_t>
	struct is_const_method;

	template<typename CClass, typename ReturnType, typename ...ArgType>
	struct is_const_method< ReturnType(CClass::*)(ArgType...)> {
		static constexpr bool value = false;
		using ClassType = CClass;
	};

	template<typename CClass, typename ReturnType, typename ...ArgType>
	struct is_const_method< ReturnType(CClass::*)(ArgType...) const> {
		static constexpr bool value = true;
		using ClassType = std::add_const_t<CClass>;
	};
}