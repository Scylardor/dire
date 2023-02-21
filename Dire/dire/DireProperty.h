#pragma once

#include "Types/DireTypeInfo.h"
#include "DirePropertyMetadata.h"

namespace DIRE_NS 
{
	template<typename T>
	struct GetParenthesizedType;

	/**
	 * \brief Allows to extract the type of the first parameter of a function type.
	 * Inspired by https://stackoverflow.com/a/4298441/1987466
	 * \tparam R Return type
	 * \tparam P1 Type of first parameter
	 */
	template<typename R, typename P1>
	struct GetParenthesizedType<R(P1)>
	{
		typedef P1 Type;
	};

	/**
	 * \brief Takes a tuple, and returns the same tuple stripped of the first type element.
	 * Inspired by https://stackoverflow.com/questions/53503124/get-a-new-tuple-containing-all-but-first-element-of-a-tuple
	 * \tparam T1 First type of the type pack
	 * \tparam Ts The rest
	 * \param tuple The tuple to split
	 * \return The tuple with the first element removed
	 */
	template <typename T1, typename... Ts>
	std::tuple<Ts...> ExtractTupleWithoutFirstType(const std::tuple<T1, Ts...>& tuple)
	{
		return std::apply([](auto&&, const auto&... args) {return std::tie(args...); }, tuple);
	}

	/**
	 * \brief A class that is used to extract a tuple from the arguments passed to the constructor of a property.
	 * 	Version for any number of parameters, but no Metadata.
	 * \tparam T The tuple type holding all the parameters
	 */
	template <typename T>
	struct ParameterExtractor
	{
		typedef T	CtorParamsType;
		typedef Metadata<>	MetadataType;

		static CtorParamsType CtorParameters(const T& tuple)
		{
			return tuple;
		}
	};

	/**
	 * \brief A class that is used to extract a tuple from the arguments passed to the constructor of a property.
	 * 	Version for when the parameter list includes Metadata.
	 *	It's going to strip the first type of the tuple, isolating the Metadata, then repack the tuple with the rest of the arguments.
	 * \tparam Ts The parameter types of the Metadata
	 * \tparam Us The rest od the argument types
	 */
	template <typename... Ts, typename... Us>
	struct ParameterExtractor<std::tuple<Metadata<Ts...>, Us...>>
	{
		typedef std::tuple<Us...>	CtorParamsType;
		typedef Metadata<Ts...>	MetadataType;
		using ParamPackTuple = std::tuple<Metadata<Ts...>, Us...>;

		template <typename... Vs>
		static CtorParamsType CtorParameters(const ParamPackTuple& tuple)
		{
			return ExtractTupleWithoutFirstType(tuple);
		}
	};


	/**
	 * \brief The property template declared by DIRE_PROPERTY. Never use this type directly, use the macro.
	 * \tparam TOwner The type of the owning class
	 * \tparam TProp The property type
	 * \tparam MetadataType The type holding the list of metadatas for this prop
	 */
	template <typename TOwner, typename TProp, typename MetadataType>
	class TypedProperty final : public PropertyTypeInfo
	{
	public:
		TypedProperty(const char* pName, std::ptrdiff_t pOffset) :
			PropertyTypeInfo(pName, pOffset, sizeof(TProp))
		{
			TypeInfo& typeInfo = TOwner::EditTypeInfo();
			typeInfo.PushTypeInfo(*this);

			SelectType<TProp>();

			PopulateDataStructureHandler<TProp>();

			SetReflectableID<TProp>();

			InitializeReflectionCopyConstructor<TProp>();
		}

		template <typename T>
		[[nodiscard]] bool	HasMetadataAttribute() const
		{
			return MetadataType::template HasAttribute<T>();
		}

#ifdef DIRE_SERIALIZATION_ENABLED
		virtual void	SerializeAttributes(class ISerializer& pSerializer) const override
		{
			MetadataType::Serialize(pSerializer);
		}

		virtual SerializationState GetSerializableState() const override
		{
#	if DIRE_SERIALIZABLE_PROPERTIES_BY_DEFAULT
			if (HasMetadataAttribute<NotSerializable>())
#	else
			if (!HasMetadataAttribute<Serializable>())
#	endif
			{
				return SerializationState(); // No serialization for you
			}

			SerializationState state;
			state.IsSerializable = true;
			state.HasAttributesToSerialize = MetadataType::HasAttributesToSerialize();
			return state;
		}
#endif
	};
}

// This offset trick was inspired by https://stackoverflow.com/a/4938266/1987466
#define DIRE_PROPERTY(type, name, ...) \
	struct name##_tag\
	{ \
		using ParamsTupleType = decltype(std::make_tuple(__VA_ARGS__));\
		using ParamExtractor = ::DIRE_NS::ParameterExtractor<ParamsTupleType>;\
		static ptrdiff_t Offset() /* just stole MSVC implementation to silence warnings about non-standard-layout */ \
		{ \
			return ((ptrdiff_t)&reinterpret_cast<char const volatile&>((((Self*)nullptr)->name))); \
		} \
    }; \
	::DIRE_NS::GetParenthesizedType<void  DIRE_LPAREN DIRE_UNPAREN(type) DIRE_RPAREN>::Type name{ std::make_from_tuple< DIRE_UNPAREN(type) >(name##_tag::ParamExtractor::CtorParameters(std::make_tuple(__VA_ARGS__))) };\
	inline static ::DIRE_NS::TypedProperty<Self, DIRE_UNPAREN(type), name##_tag::ParamExtractor::MetadataType> name##_TYPEINFO_PROPERTY{DIRE_STRINGIZE(name), name##_tag::Offset() };

#define DIRE_ARRAY_PROPERTY(type, name, size, ...) \
	struct name##_tag\
	{ \
		using ParamsTupleType = decltype(std::make_tuple(__VA_ARGS__));\
		using ParamExtractor = ::DIRE_NS::ParameterExtractor<ParamsTupleType>;\
		static ptrdiff_t Offset() /* just stole MSVC implementation to silence warnings about non-standard-layout */ \
		{ \
			return ((ptrdiff_t)&reinterpret_cast<char const volatile&>((((Self*)nullptr)->name))); \
		} \
	}; \
	::DIRE_NS::GetParenthesizedType<void DIRE_LPAREN DIRE_UNPAREN(type) DIRE_RPAREN>::Type name size{ }; \
	inline static ::DIRE_NS::TypedProperty<Self, DIRE_UNPAREN(type) size, name##_tag::ParamExtractor::MetadataType> name##_TYPEINFO_PROPERTY{ DIRE_STRINGIZE(name), name##_tag::Offset()};
