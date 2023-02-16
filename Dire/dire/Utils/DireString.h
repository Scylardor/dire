#pragma once

#include <type_traits> // enable_if
#include <charconv> // from_chars
#include <system_error>
#include <variant>
#include "DireDefines.h"

namespace DIRE_NS
{
	namespace String
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
	}

	using ConvertError = DIRE_STRING;

	/**
	 * \brief Encapsulates the result of a string conversion. It holds either the result in the requested type, or an error.
	 * \tparam T The type we want to convert a string into
	 */
	template <typename T>
	class ConvertResult
	{
	public:

		explicit ConvertResult(T pVal) :
			Value(pVal)
		{}

		ConvertResult(const DIRE_STRING_VIEW& pToken, std::errc pErrorCode)
		{
			std::error_code errorCode = std::make_error_code(pErrorCode);

			auto neededSize = snprintf(nullptr, 0, "Converting '%s' failed: '%s'.", pToken.data(), errorCode.message().c_str());
			ConvertError& error = Value.template emplace<ConvertError>(neededSize + 1, '\0');
			snprintf(error.data(), error.size(), "Converting '%s' failed: '%s'.", pToken.data(), errorCode.message().c_str());
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

	/**
	 * \brief Specialization used to convert from a string to a Dire Enum
	 * \tparam T The Dire enum type
	 */
	template <typename T>
	struct FromCharsConverter<T, typename std::enable_if_t<std::is_base_of_v<Enum, T>, void>>
	{
		static T Convert(const DIRE_STRING_VIEW& pChars)
		{
			T value = T::GetValueFromSafeString(pChars);
			return value;
		}
	};

	/**
	 * \brief Specialization used to convert from a string to a simple number
	 * \tparam T The number type
	 */
	template <typename T>
	struct FromCharsConverter<T, typename std::enable_if_t<std::is_arithmetic_v<T>, void>>
	{
		static ConvertResult<T> Convert(const DIRE_STRING_VIEW& pChars)
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

			return ConvertResult<T>(value);
		}
	};

	template <class T>
	ConvertResult<T> from_chars(DIRE_STRING_VIEW const& pChars)
	{
		ConvertResult<T> key = FromCharsConverter<T>::Convert(pChars);
		return key;
	}

}
