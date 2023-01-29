#pragma once

namespace DIRE_NS
{
	template <typename T>
	struct Subclass
	{
		static_assert(std::is_base_of_v<Reflectable, T>, "Subclass only works for Reflectable-derived classes.");

		Subclass()
		{
			SubClassID = T::GetTypeInfo().GetID();
		}

		[[nodiscard]] bool	IsValid() const
		{
			TypeInfo const& parentTypeInfo = T::GetTypeInfo();
			return (parentTypeInfo.IsParentOf(SubClassID));
		}

		void	SetClass(ReflectableID pNewID)
		{
			SubClassID = pNewID;
		}

		template <typename U>
		void	SetClass()
		{
			static_assert(std::is_base_of_v<Reflectable, U>, "Subclass can only store IDs of Reflectable-derived classes.");
			SubClassID = U::GetTypeInfo().GetID();
		}

		ReflectableID	GetClassID() const
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

			Reflectable* newInstance;

			if constexpr (sizeof...(Args) != 0)
			{
				using ArgumentPackTuple = std::tuple<Args...>;
				std::any packedArgs(std::in_place_type<ArgumentPackTuple>, std::forward_as_tuple(std::forward<Args>(pCtorArgs)...));
				newInstance = TypeInfoDatabase::GetSingleton().TryInstantiate(SubClassID, packedArgs);
			}
			else
			{
				newInstance = TypeInfoDatabase::GetSingleton().TryInstantiate(SubClassID, {});
			}

			return static_cast<Ret*>(newInstance);
		}

	private:

		ReflectableID	SubClassID{ T::GetTypeInfo().GetID() };
	};
}