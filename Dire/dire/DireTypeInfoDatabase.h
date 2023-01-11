#pragma once

#include "DireSingleton.h"
#include "DireStaticTypeCounter.h"

#include <vector>
#include <any>
#include <unordered_map>

#include "DireString.h"

namespace DIRE_NS
{
	using ReflectableID = unsigned;
	class TypeInfo;
	struct Reflectable2;

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
		inline static const int REFLECTOR_VERSION = 0;

	public:

		Reflector3() = default;

		[[nodiscard]] size_t	GetTypeInfoCount() const
		{
			return myReflectableTypeInfos.size();
		}

		[[nodiscard]] TypeInfo const* GetTypeInfo(unsigned classID) const
		{
			if (classID < myReflectableTypeInfos.size())
			{
				return myReflectableTypeInfos[classID];
			}

			return nullptr;
		}

		[[nodiscard]] TypeInfo* EditTypeInfo(unsigned classID)
		{
			if (classID < myReflectableTypeInfos.size())
			{
				return myReflectableTypeInfos[classID];
			}

			return nullptr;
		}


		void	RegisterInstantiateFunction(unsigned pClassID, ReflectableFactory::InstantiateFunction pInstantiateFunction)
		{
			myInstantiateFactory.RegisterInstantiator(pClassID, pInstantiateFunction);
		}

		[[nodiscard]] Reflectable2* TryInstantiate(unsigned pClassID, std::any const& pAnyParameterPack) const
		{
			ReflectableFactory::InstantiateFunction anInstantiateFunc = myInstantiateFactory.GetInstantiator(pClassID);
			if (anInstantiateFunc == nullptr)
			{
				return nullptr;
			}

			Reflectable2* newInstance = anInstantiateFunc(pAnyParameterPack);
			return newInstance;
		}


		// This follows a very simple binary serialization process right now. It encodes:
		// - the reflectable type ID
		// - the typename string
		struct ExportedTypeInfoData
		{
			unsigned	ReflectableID;
			std::string	TypeName;
		};

		bool	ExportTypeInfoSettings(DIRE_STRING_VIEW pWrittenSettingsFile) const;
		bool	ImportTypeInfoSettings(DIRE_STRING_VIEW pReadSettingsFile);

	private:
		friend TypeInfo;

		[[nodiscard]] unsigned	RegisterTypeInfo(TypeInfo* pTypeInfo)
		{
			myReflectableTypeInfos.push_back(pTypeInfo);
			return (unsigned)GetTypeInfoCount() - 1;
		}

		std::vector<TypeInfo*>	myReflectableTypeInfos;
		ReflectableFactory	myInstantiateFactory;
	};
}
