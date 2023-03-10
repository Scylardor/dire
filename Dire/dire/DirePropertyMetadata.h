#pragma once

#ifdef DIRE_SERIALIZATION_ENABLED
# include "Serialization/DireSerialization.h"
#endif

namespace DIRE_NS
{
	class ISerializer;

	struct IMetadataAttribute
	{
	};

	// inspired by https://stackoverflow.com/a/41171291/1987466
	template <typename T, typename Tuple>
	struct TupleHasType;

	template <typename T, typename... Us>
	struct TupleHasType<T, std::tuple<Us...>> : std::disjunction<std::is_same<T, Us>...> {};

	/**
	 * \brief The holder class for Dire property metadatas.
	 * Aims to provide a functionality similar to Unity's attributes or Unreal's UMETA (or meta keywords), by tagging each property.
	 * In its current iteration, Metadata is supposed to be used only with constexpr types and values, which is why it just acts as a holder of a variadic type list.
	 * It is only useful in the context of serialization, and as of now, is only useful for JSON serializing. Its purposes is not to store in-game data.
	 * \tparam Ts The list of metadata attributes
	 */
	template <typename... Ts>
	struct Metadata
	{
		static_assert((std::is_base_of_v<IMetadataAttribute, std::remove_reference_t<Ts&&>> && ...), "You passed a type parameter to TMetadata that is not a IMetadataAttribute");

		Metadata() = default;

		template <typename T>
		void	RegisterAttribute(const T& pMetadataAttribute)
		{
			pMetadataAttribute.Register(*this);
		}

		template <typename T>
		[[nodiscard]] static constexpr bool	HasAttribute()
		{
			return TupleHasType<T, std::tuple<Ts...>>::value;
		}


		[[nodiscard]] static constexpr auto	GetAttributesCount()
		{
			return sizeof...(Ts);
		}

#ifdef DIRE_SERIALIZATION_ENABLED
		template <typename T>
		static void	SerializeAttribute(class ISerializer& pSerializer)
		{
			T::Serialize(pSerializer);
		}

		static void	Serialize(ISerializer& pSerializer)
		{
			(Ts::Serialize(pSerializer), ...);
		}

		static 	bool	HasAttributesToSerialize()
		{
			return GetAttributesCount() > 0;
		}
#endif

		static const Metadata& Empty()
		{
			static const Metadata empty;
			return empty;
		}
	};

	template<>
	inline void Metadata<>::Serialize(ISerializer&){}

	struct Serializable : IMetadataAttribute
	{
#ifdef DIRE_SERIALIZATION_ENABLED
		static void	Serialize(ISerializer&) {}
#endif
	};

	struct NotSerializable : IMetadataAttribute
	{
#ifdef DIRE_SERIALIZATION_ENABLED
		static void	Serialize(ISerializer&) {}
#endif
	};

	struct Transient : IMetadataAttribute
	{
#ifdef DIRE_SERIALIZATION_ENABLED
		static void	Serialize(ISerializer& pSerializer)
		{
			pSerializer.SerializeValuesForObject("Transient", [](ISerializer& /*pSerializer*/){}); // nothing to do for a "boolean" attribute
		}
#endif
	};

// We need C++20 (and later) for complicated non-type template parameters like floats and string literals.
#if DIRE_HAS_CPP20
	// inspired by https://vector-of-bool.github.io/2021/10/22/string-templates.html
	template<unsigned N>
	struct Dire_EXPORT FixedString
	{
		char	String[N + 1]{}; // +1 for null terminator

		constexpr FixedString(char const* pInitStr)
		{
			for (unsigned i = 0; i != N; ++i)
				String[i] = pInitStr[i];
		}
		constexpr operator const char* () const
		{
			return String;
		}
	};

	// important deduction guide
	template<unsigned N>
	FixedString(char const (&)[N])->FixedString<N - 1>; // Drop the null terminator

	template<FixedString Str>
	struct DisplayName : IMetadataAttribute
	{
# ifdef DIRE_SERIALIZATION_ENABLED
		static void	Serialize(ISerializer& pSerializer)
		{
			pSerializer.SerializeValuesForObject("DisplayName", [](ISerializer& pSerializer)
			{
				pSerializer.SerializeString("Name");
				pSerializer.SerializeString(Name);
			});
		}

	private:
		inline static constexpr char const* Name = Str;
# endif
	};

	template <float Min, float Max>
	struct FValueRange : IMetadataAttribute
	{
		static_assert(Min <= Max, "Min and Max values are inverted!");

# ifdef DIRE_SERIALIZATION_ENABLED
		static void	Serialize(ISerializer& pSerializer)
		{
			pSerializer.SerializeValuesForObject("FValueRange", [](ISerializer& pSerializer)
			{
				pSerializer.SerializeString("Min");
				pSerializer.SerializeFloat(Min);
				pSerializer.SerializeString("Max");
				pSerializer.SerializeFloat(Max);
			});
		}
# endif
	};

#endif // DIRE_HAS_CPP20

	template <int32_t Min, int32_t Max>
	struct IValueRange : IMetadataAttribute
	{
		static_assert(Min <= Max, "Min and Max values are inverted!");

#ifdef DIRE_SERIALIZATION_ENABLED
		static void	Serialize(ISerializer& pSerializer)
		{
			pSerializer.SerializeValuesForObject("IValueRange", [](ISerializer& pSerializer)
			{
				pSerializer.SerializeString("Min");
				pSerializer.SerializeInt(Min);
				pSerializer.SerializeString("Max");
				pSerializer.SerializeInt(Max);
			});
		}
#endif
	};
}