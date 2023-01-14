#pragma once

#include "DireSingleton.h"
#include "DireStaticTypeCounter.h"

#include <vector>
#include <unordered_map>

#include "DireString.h"

namespace std
{
	class any;
}

namespace DIRE_NS
{
	using ReflectableID = unsigned;
	class TypeInfo;
	class Reflectable2;

	class ReflectableFactory
	{
	public:
		using InstantiateFunction = Reflectable2 * (*)(std::any const&);

		ReflectableFactory() = default;

		void	RegisterInstantiator(unsigned pID, InstantiateFunction pFunc)
		{
			// Replace the existing one, if any.
			auto [it, inserted] = myInstantiators.try_emplace(pID, pFunc);
			if (!inserted)
			{
				it->second = pFunc;
			}
		}

		InstantiateFunction	GetInstantiator(unsigned pID) const
		{
			auto it = myInstantiators.find(pID);
			if (it == myInstantiators.end())
			{
				return nullptr;
			}

			return it->second;
		}

	private:

		std::unordered_map<unsigned, InstantiateFunction>	myInstantiators;
	};

	// TODO: replace all the unsigned by a unified typedef
	struct Reflector3 : Singleton<Reflector3>, AutomaticTypeCounter<Reflector3, ReflectableID>
	{
		inline static const unsigned DATABASE_VERSION = 0;

		[[nodiscard]] size_t	GetTypeInfoCount() const
		{
			return myReflectableTypeInfos.size();
		}

		[[nodiscard]] TypeInfo const* GetTypeInfo(unsigned classID) const;

		[[nodiscard]] TypeInfo* EditTypeInfo(unsigned classID);


		void	RegisterInstantiateFunction(unsigned pClassID, ReflectableFactory::InstantiateFunction pInstantiateFunction)
		{
			myInstantiateFactory.RegisterInstantiator(pClassID, pInstantiateFunction);
		}

		[[nodiscard]] Reflectable2* TryInstantiate(unsigned pClassID, std::any const& pAnyParameterPack) const;


		// This follows a very simple binary serialization process right now. It encodes:
		// - the reflectable type ID
		// - the typename string
		struct ExportedTypeInfoData
		{
			unsigned	ReflectableID;
			std::string	TypeName;
		};

		DIRE_STRING	BinaryExport() const;
		bool	ExportToBinaryFile(DIRE_STRING_VIEW pWrittenSettingsFile) const;

		bool	ExportTypeInfoSettings(DIRE_STRING_VIEW pWrittenSettingsFile) const;
		bool	ImportTypeInfoSettings(DIRE_STRING_VIEW pReadSettingsFile);

	// Allow unit tests to build a database that is not the program's singleton.
#if !DIRE_UNIT_TESTS
	protected:
		Reflector3() = default;
		friend Singleton<Reflector3>;

	private:
#endif
		friend TypeInfo;

		[[nodiscard]] unsigned	RegisterTypeInfo(TypeInfo* pTypeInfo)
		{
			myReflectableTypeInfos.push_back(pTypeInfo);
			return (unsigned)GetTypeInfoCount() - 1;
		}

		std::vector<TypeInfo*>	myReflectableTypeInfos;
		ReflectableFactory		myInstantiateFactory;

	};
}
