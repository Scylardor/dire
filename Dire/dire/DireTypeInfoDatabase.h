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

		void	RegisterInstantiator(ReflectableID pID, InstantiateFunction pFunc)
		{
			// Replace the existing one, if any.
			auto [it, inserted] = myInstantiators.try_emplace(pID, pFunc);
			if (!inserted)
			{
				it->second = pFunc;
			}
		}

		InstantiateFunction	GetInstantiator(ReflectableID pID) const
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
		[[nodiscard]] size_t	GetTypeInfoCount() const
		{
			return myReflectableTypeInfos.size();
		}

		[[nodiscard]] const TypeInfo * GetTypeInfo(ReflectableID classID) const;

		[[nodiscard]] TypeInfo* EditTypeInfo(ReflectableID classID);

		void	RegisterInstantiateFunction(ReflectableID pClassID, ReflectableFactory::InstantiateFunction pInstantiateFunction)
		{
			myInstantiateFactory.RegisterInstantiator(pClassID, pInstantiateFunction);
		}

		[[nodiscard]] Reflectable* TryInstantiate(ReflectableID pClassID, std::any const& pAnyParameterPack) const;

		template <typename T, typename... Args>
		[[nodiscard]] T* InstantiateClass(Args &&... pArgs) const
		{
			static_assert(std::is_base_of_v<Reflectable, T>, "ClassInstantiator is only meant to be used as a member of Reflectable-derived classes.");
			if (sizeof...(Args) == 0)
				return static_cast<T*>(TryInstantiate(T::GetTypeInfo().GetID(), {}));
			return static_cast<T*>(TryInstantiate(T::GetTypeInfo().GetID(), { std::tuple<Args...>(std::forward<Args>(pArgs)...) }));
		}

		DIRE_STRING	BinaryExport() const;
		bool	ExportToBinaryFile(DIRE_STRING_VIEW pWrittenSettingsFile) const;

		static DIRE_STRING	BinaryImport(DIRE_STRING_VIEW pReadSettingsFile);
		bool	ImportFromBinaryFile(DIRE_STRING_VIEW pReadSettingsFile);

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
}
