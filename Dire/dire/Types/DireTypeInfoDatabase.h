#pragma once

#include "DireDefines.h"

#include <vector>
#include <unordered_map>

#include "dire/Utils/DireString.h"
#include "dire/DireReflectableID.h"

namespace std
{
	class any;
}

namespace DIRE_NS
{
	class TypeInfo;
	class Reflectable;

	class ReflectableFactory
	{
	public:
		using InstantiateFunction = Reflectable * (*)(const std::any &);

		ReflectableFactory() = default;

		Dire_EXPORT void	RegisterInstantiator(ReflectableID pID, InstantiateFunction pFunc);

		Dire_EXPORT InstantiateFunction	GetInstantiator(ReflectableID pID) const;

	private:
		using InstantiatorsHashTable =
			std::unordered_map<ReflectableID, InstantiateFunction, std::hash<ReflectableID>, std::equal_to<ReflectableID>,
			DIRE_ALLOCATOR<std::pair<const ReflectableID, InstantiateFunction>>>;

		InstantiatorsHashTable	myInstantiators;
	};


	class TypeInfoDatabase
	{
		inline static const unsigned DATABASE_VERSION = 0;

	public:

		// Important for DLL builds, because otherwise, the client program will use its own singleton in client code,
		// and DIRE uses its own singleton in DIRE code, which creates inconsistencies.
		Dire_EXPORT static const TypeInfoDatabase&	GetSingleton();
		Dire_EXPORT static TypeInfoDatabase&		EditSingleton();

		[[nodiscard]] Dire_EXPORT size_t	GetTypeInfoCount() const;

		[[nodiscard]] Dire_EXPORT const TypeInfo * GetTypeInfo(ReflectableID classID) const;

		[[nodiscard]] Dire_EXPORT TypeInfo* EditTypeInfo(ReflectableID classID);

		Dire_EXPORT void	RegisterInstantiateFunction(ReflectableID pClassID, ReflectableFactory::InstantiateFunction pInstantiateFunction);

		[[nodiscard]] Dire_EXPORT Reflectable* TryInstantiate(ReflectableID pClassID, std::any const& pAnyParameterPack) const;

		template <typename T, typename... Args>
		[[nodiscard]] T* InstantiateClass(Args &&... pArgs) const
		{
			static_assert(std::is_base_of_v<Reflectable, T>, "ClassInstantiator is only meant to be used as a member of Reflectable-derived classes.");
			if constexpr (sizeof...(Args) == 0)
				return static_cast<T*>(TryInstantiate(T::GetTypeInfo().GetID(), {}));
			else
				return static_cast<T*>(TryInstantiate(T::GetTypeInfo().GetID(), { std::tuple<Args...>(std::forward<Args>(pArgs)...) }));
		}

		Dire_EXPORT DIRE_STRING	BinaryExport() const;
		Dire_EXPORT bool	ExportToBinaryFile(DIRE_STRING_VIEW pWrittenSettingsFile) const;

		Dire_EXPORT static DIRE_STRING	BinaryImport(DIRE_STRING_VIEW pReadSettingsFile);
		Dire_EXPORT bool	ImportFromBinaryFile(DIRE_STRING_VIEW pReadSettingsFile);

	// Allow unit tests to build a database that is not the program's singleton.
#if !DIRE_TESTS_ENABLED
	protected:
		TypeInfoDatabase() = default;
		friend Singleton<TypeInfoDatabase>;

	private:
#endif
		friend TypeInfo;

		[[nodiscard]] ReflectableID	RegisterTypeInfo(TypeInfo* pTypeInfo)
		{
			myReflectableTypeInfos.push_back(pTypeInfo);
			return ReflectableID(GetTypeInfoCount()) - 1;
		}

		std::vector<TypeInfo*, DIRE_ALLOCATOR<TypeInfo*>>	myReflectableTypeInfos;
		ReflectableFactory		myInstantiateFactory;
	};

}
