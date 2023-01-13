#pragma once

namespace DIRE_NS
{
	template <typename T>
	struct Subclass
	{
		static_assert(std::is_base_of_v<Reflectable2, T>, "Subclass only works for Reflectable-derived classes.");

		Subclass() = default;

		[[nodiscard]] bool	IsValid() const
		{
			TypeInfo const& parentTypeInfo = T::GetClassReflectableTypeInfo();
			return (parentTypeInfo.IsParentOf(SubClassID));
		}

		void	SetClass(unsigned pNewID) //TODO: reflectable ID
		{
			SubClassID = pNewID;
		}

		unsigned	GetClassID() const //TODO: reflectable ID
		{
			return SubClassID;
		}

		template <typename Ret = T, typename... Args>
		[[nodiscard]] Ret* Instantiate(Args&&... pCtorArgs)
		{
			static_assert(std::is_base_of_v<T, Ret>, "Instantiate return type must be derived from the subclass type parameter!");

			if (!IsValid())
			{
				return nullptr;
			}

			Reflectable2* newInstance;

			if constexpr (sizeof...(Args) != 0)
			{
				using ArgumentPackTuple = std::tuple<Args...>;
				std::any packedArgs(std::in_place_type<ArgumentPackTuple>, std::forward_as_tuple(std::forward<Args>(pCtorArgs)...));
				newInstance = Reflector3::GetSingleton().TryInstantiate(SubClassID, packedArgs);
			}
			else
			{
				newInstance = Reflector3::GetSingleton().TryInstantiate(SubClassID, {});
			}

			return static_cast<Ret*>(newInstance);
		}

	private:

		unsigned	SubClassID{ T::GetClassReflectableTypeInfo().GetID() };  //TODO: reflectable ID
	};
}