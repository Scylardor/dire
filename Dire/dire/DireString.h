#pragma once

#include <type_traits> // enable_if
#include <charconv> // from_chars
#include <variant>

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

	using ConvertError = DIRE_STRING;

	template <typename T>
	class ConvertResult
	{
	public:

		ConvertResult(T pVal) :
			Value(pVal)
		{}

		ConvertResult(const DIRE_STRING_VIEW& pToken, std::errc pErrorCode)
		{
			auto neededSize = snprintf(nullptr, 0, "Converting '%s' failed: '%s'.", pToken.data(), std::strerror((int)pErrorCode));
			ConvertError& error = Value.template emplace<ConvertError>(neededSize + 1, '\0');
			snprintf(error.data(), error.size(), "Converting '%s' failed: '%s'.", pToken.data(), std::strerror((int)pErrorCode));
		}

		[[nodiscard]] T	GetValue() const { return std::get<T>(Value); }

		[[nodiscard]] bool			HasError() const { return std::get_if<ConvertError>(&Value) != nullptr; }
		[[nodiscard]] ConvertError	GetError() const
		{
			const ConvertError* error = std::get_if<ConvertError>(&Value);
			if (error != nullptr)
				return *error;

			return "";
		}

	private:
		std::variant<T, ConvertError>	Value;
	};

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
		static ConvertResult<T> Convert(const std::string_view& pChars)
		{
			T value;
			if constexpr (std::is_same_v<T, bool>)
			{
				// bool is an arithmetic type but from_chars doesn't support it so we need a special case to handle it
				if (pChars != "true" && pChars != "false" && pChars != "1" && pChars != "0")
				{
					return ConvertResult<T>(pChars, std::errc::invalid_argument);
				}

				value = (pChars == "true" || pChars == "1");
			}
			else
			{
				auto [unusedPtr, error] { ::std::from_chars(pChars.data(), pChars.data() + pChars.size(), value) };
				(void)unusedPtr;
				if (error != std::errc())
				{
					return ConvertResult<T>(pChars, error);
				}
			}

			// TODO: check for errors
			return value;
		}
	};

	template <class T>
	ConvertResult<T> from_chars(DIRE_STRING_VIEW const& pChars)
	{
		ConvertResult<T> key = FromCharsConverter<T>::Convert(pChars);
		return key;
	}

}