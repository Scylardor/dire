#pragma once

#include "DireSingleton.h"
#include "DireStaticTypeCounter.h"

#include <vector>
#include <unordered_map>

#include "DireString.h"
#include "DireReflectableID.h"

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

		Dire_EXPORT void	RegisterInstantiator(ReflectableID pID, InstantiateFunction pFunc)
		{
			// Replace the existing one, if any.
			auto [it, inserted] = myInstantiators.try_emplace(pID, pFunc);
			if (!inserted)
			{
				it->second = pFunc;
			}
		}

		Dire_EXPORT InstantiateFunction	GetInstantiator(ReflectableID pID) const
		{
			auto it = myInstantiators.find(pID);
			if (it == myInstantiators.end())
			{
				return nullptr;
			}

			return it->second;
		}

	private:
		using InstantiatorsHashTable =
			std::unordered_map<ReflectableID, InstantiateFunction, std::hash<ReflectableID>, std::equal_to<ReflectableID>,
			DIRE_ALLOCATOR<std::pair<const ReflectableID, InstantiateFunction>>>;

		InstantiatorsHashTable	myInstantiators;
	};


	class TypeInfoDatabase : public Singleton<TypeInfoDatabase>
	{
		inline static const unsigned DATABASE_VERSION = 0;

	public:
		Dire_EXPORT [[nodiscard]] size_t	GetTypeInfoCount() const
		{
			return myReflectableTypeInfos.size();
		}

		Dire_EXPORT [[nodiscard]] const TypeInfo * GetTypeInfo(ReflectableID classID) const;

		Dire_EXPORT [[nodiscard]] TypeInfo* EditTypeInfo(ReflectableID classID);

		Dire_EXPORT void	RegisterInstantiateFunction(ReflectableID pClassID, ReflectableFactory::InstantiateFunction pInstantiateFunction)
		{
			myInstantiateFactory.RegisterInstantiator(pClassID, pInstantiateFunction);
		}

		Dire_EXPORT  [[nodiscard]] Reflectable* TryInstantiate(ReflectableID pClassID, std::any const& pAnyParameterPack) const;

		template <typename T, typename... Args>
		[[nodiscard]] T* InstantiateClass(Args &&... pArgs) const
		{
			static_assert(std::is_base_of_v<Reflectable, T>, "ClassInstantiator is only meant to be used as a member of Reflectable-derived classes.");
			if (sizeof...(Args) == 0)
				return static_cast<T*>(TryInstantiate(T::GetTypeInfo().GetID(), {}));
			return static_cast<T*>(TryInstantiate(T::GetTypeInfo().GetID(), { std::tuple<Args...>(std::forward<Args>(pArgs)...) }));
		}

		Dire_EXPORT DIRE_STRING	BinaryExport() const;
		Dire_EXPORT bool	ExportToBinaryFile(DIRE_STRING_VIEW pWrittenSettingsFile) const;

		Dire_EXPORT static DIRE_STRING	BinaryImport(DIRE_STRING_VIEW pReadSettingsFile);
		Dire_EXPORT bool	ImportFromBinaryFile(DIRE_STRING_VIEW pReadSettingsFile);

	// Allow unit tests to build a database that is not the program's singleton.
#if !DIRE_UNIT_TESTS
	protected:
		TypeInfoDatabase() = default;
		friend Singleton<TypeInfoDatabase>;

	private:
#endif
		friend TypeInfo;

		[[nodiscard]] ReflectableID	RegisterTypeInfo(TypeInfo* pTypeInfo)
		{
			myReflectableTypeInfos.push_back(pTypeInfo);
			return (ReflectableID)GetTypeInfoCount() - 1;
		}

		std::vector<TypeInfo*, DIRE_ALLOCATOR<TypeInfo*>>	myReflectableTypeInfos;
		ReflectableFactory		myInstantiateFactory;
	};

	// Important for DLL builds, because otherwise, the client program will use its own singleton in client code,
	// and DIRE uses its own singleton in DIRE code, which creates inconsistencies.
	Dire_EXPORT const TypeInfoDatabase& Singleton<TypeInfoDatabase>::GetSingleton();
	Dire_EXPORT TypeInfoDatabase& Singleton<TypeInfoDatabase>::EditSingleton();
}
