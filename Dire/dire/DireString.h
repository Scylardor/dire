#pragma once

#include <type_traits> // enable_if

/* If user says it's ok to use std::string, include std::string and use it everywhere we need a dynamic string */
#if DIRE_USE_STD_STRING
# include <string>
# include <string_view>
# define DIRE_STRING std::string
# define DIRE_STRING_VIEW std::string_view
#endif
/* If DIRE_USE_STD_STRING was not set, it means the user provided another string type to use in DIRE_STRING through CMake. */

namespace DIRE_NS
{
	// inspired by https://stackoverflow.com/a/33399801/1987466
	namespace ADL
	{
		template<class T>
		DIRE_STRING AsString(T&& t)
		{
			using std::to_string;
			return to_string(std::forward<T>(t));
		}
	}
	template<class T>
	DIRE_STRING to_string(T&& t)
	{
		return ADL::AsString(std::forward<T>(t));
	}

	class Enum;

	template<typename T, typename = void>
	struct FromCharsConverter;

	template <typename T>
	struct FromCharsConverter<T, typename std::enable_if_t<std::is_base_of_v<Enum, T>, void>>
	{
		static T Convert(const std::string_view& pChars)
		{
			return *T::GetValueFromString(pChars.data());
		}
	};

	template <typename T>
	struct FromCharsConverter<T, typename std::enable_if_t<std::is_arithmetic_v<T>, void>>
	{
		static T Convert(const std::string_view& pChars)
		{
			T result;
			if constexpr (std::is_same_v<T, bool>)
			{
				// bool is an arithmetic type but from_chars doesn't support it so we need a special case to handle it
				result = (pChars == "true" || pChars == "1");
			}
			else
			{
				auto [unusedPtr, error] { std::from_chars(pChars.data(), pChars.data() + pChars.size(), result) };
				(void)unusedPtr;
			}

			// TODO: check for errors
			return result;
		}
	};

	template <class T>
	T from_chars(DIRE_STRING_VIEW const& pChars)
	{
		T key = FromCharsConverter<T>::Convert(pChars);
		return key;
	}

}