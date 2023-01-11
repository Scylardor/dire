#pragma once

#include "DireTypeInfo.h"
#include "DireTypeInfoDatabase.h"
#include "DireString.h"
#include <utility> // std::pair

namespace DIRE_NS
{
	class ReflectableTypeInfo
	{
	public:

		ReflectableTypeInfo() = default; // TODO: remove

		ReflectableTypeInfo(const char * pTypename) :
			TypeName(pTypename)
		{
			ReflectableID = Reflector3::EditSingleton().RegisterTypeInfo(this);
		}

		void	PushTypeInfo(TypeInfo& pNewTypeInfo)
		{
			Properties.PushBackNewNode(pNewTypeInfo);
		}

		void	PushFunctionInfo(FunctionInfo& pNewFunctionInfo)
		{
			MemberFunctions.PushBackNewNode(pNewFunctionInfo);
		}

		void	AddParentClass(ReflectableTypeInfo* pParent)
		{
			ParentClasses.push_back(pParent);
		}

		void	AddChildClass(ReflectableTypeInfo* pChild)
		{
			ChildrenClasses.push_back(pChild);
		}

		[[nodiscard]] bool	IsParentOf(const unsigned pChildClassID) const
		{
			auto predicate = [pChildClassID](const ReflectableTypeInfo* pChildTypeInfo)
			{
				return pChildTypeInfo->ReflectableID == pChildClassID;
			};

			return std::find_if(ChildrenClasses.begin(), ChildrenClasses.end(), predicate) != ChildrenClasses.end();
		}

		using ParentPropertyInfo = std::pair< const ReflectableTypeInfo *, const TypeInfo *>;
		[[nodiscard]] ParentPropertyInfo	FindParentClassProperty(const DIRE_STRING_VIEW & pName) const
		{
			for (const ReflectableTypeInfo* parentClass : ParentClasses)
			{
				const TypeInfo * foundProp = parentClass->FindProperty(pName);
				if (foundProp != nullptr)
				{
					return { parentClass, foundProp };
				}
			}

			return { nullptr, nullptr };
		}

		[[nodiscard]] TypeInfo const* FindProperty(const DIRE_STRING_VIEW& pName) const
		{
			for (const auto & prop : Properties)
			{
				if (prop.Name == pName)
				{
					return &prop;
				}
			}

			return nullptr;
		}

		[[nodiscard]] TypeInfo const* FindPropertyInHierarchy(const DIRE_STRING_VIEW& pName) const
		{
			for (auto const& prop : Properties)
			{
				if (prop.Name == pName)
				{
					return &prop;
				}
			}

			// Maybe in the parent classes?
			auto [parentClass, parentProperty] = FindParentClassProperty(pName);
			if (parentClass != nullptr && parentProperty != nullptr) // A parent property was found
			{
				return parentProperty;
			}

			return nullptr;
		}

		[[nodiscard]] FunctionInfo const* FindFunction(const DIRE_STRING_VIEW& pFuncName) const
		{
			for (FunctionInfo const& aFuncInfo : MemberFunctions)
			{
				if (aFuncInfo.GetName() == pFuncName)
				{
					return &aFuncInfo;
				}
			}

			return nullptr;
		}

		using ParentFunctionInfo = std::pair<const ReflectableTypeInfo *, const FunctionInfo *>;
		[[nodiscard]] ParentFunctionInfo	FindParentFunction(const DIRE_STRING_VIEW& pFuncName) const
		{
			for (const ReflectableTypeInfo * parentClass : ParentClasses)
			{
				const FunctionInfo * foundFunc = parentClass->FindFunction(pFuncName);
				if (foundFunc != nullptr)
				{
					return { parentClass, foundFunc };
				}
			}

			return {};
		}

		template <typename F>
		void	ForEachPropertyInHierarchy(F&& pVisitorFunction) const
		{
			// Iterate on the parents list in reverse order so that we traverse the class hierarchy top to bottom.
			for (auto rit = ParentClasses.rbegin(); rit != ParentClasses.rend(); ++rit)
			{
				for (auto const& prop : (*rit)->Properties)
				{
					pVisitorFunction(prop);
				}
			}

			for (auto const& prop : Properties)
			{
				pVisitorFunction(prop);
			}
		}

		[[nodiscard]] const DIRE_STRING_VIEW& GetName() const
		{
			return TypeName;
		}

		void	SetID(const unsigned pNewID)
		{
			ReflectableID = pNewID;
		}

		[[nodiscard]] unsigned	GetID() const
		{
			return ReflectableID;
		}

		[[nodiscard]] int	GetVptrOffset() const
		{
			return VirtualOffset;
		}

		void	CloneHierarchyPropertiesOf(Reflectable2& pNewClone, const Reflectable2& pCloned) const;

		void	ClonePropertiesOf(Reflectable2& pNewClone, const Reflectable2& pCloned) const;

		[[nodiscard]] const IntrusiveLinkedList<TypeInfo>& GetPropertyList() const
		{
			return Properties;
		}

		[[nodiscard]] const IntrusiveLinkedList<FunctionInfo>& GetFunctionList() const
		{
			return MemberFunctions;
		}

		[[nodiscard]] const std::vector<ReflectableTypeInfo*>& GetParentClasses() const
		{
			return ParentClasses;
		}

	protected:
		unsigned							ReflectableID = (unsigned)-1;
		int									VirtualOffset{ 0 }; // for polymorphic classes
		DIRE_STRING_VIEW					TypeName;
		IntrusiveLinkedList<TypeInfo>		Properties;
		IntrusiveLinkedList<FunctionInfo>	MemberFunctions;
		std::vector<ReflectableTypeInfo*>	ParentClasses;
		std::vector<ReflectableTypeInfo*>	ChildrenClasses;
	};

	template <typename T, bool UseDefaultCtorForInstantiate>
	class TypedReflectableTypeInfo : public ReflectableTypeInfo
	{
	public:
		TypedReflectableTypeInfo(char const* pTypename) :
			ReflectableTypeInfo(pTypename)
		{
			if constexpr (std::is_base_of_v<Reflectable2, T>)
			{
				RecursiveRegisterParentClasses <T, UseDefaultCtorForInstantiate, typename T::Super>(*this);
			}

			if constexpr (UseDefaultCtorForInstantiate && std::is_default_constructible_v<T>)
			{
				static_assert(std::is_base_of_v<Reflectable2, T>, "This class is only supposed to be used as a member variable of a Reflectable-derived class.");
				Reflector3::EditSingleton().RegisterInstantiateFunction(T::GetClassReflectableTypeInfo().GetID(),
					[](std::any const&) -> Reflectable2*
					{
						return new T();
					});
			}

			if constexpr (std::is_polymorphic_v<T>)
			{
				VirtualOffset += sizeof(void*); // should be 4 on x86, 8 on x64
			}
		}

	private:
		template <typename TParent>
		void	RecursiveRegisterParentClasses()
		{
			if constexpr (!std::is_same_v<TParent, Reflectable2>)
			{
				ReflectableTypeInfo& parentTypeInfo = TParent::EditClassReflectableTypeInfo();
				parentTypeInfo.AddChildClass(this);
				this->AddParentClass(&parentTypeInfo);
				RecursiveRegisterParentClasses<typename TParent::Super>();
			}
		}

	};
}





// This was inspired by https://stackoverflow.com/a/70701479/1987466 and https://gcc.godbolt.org/z/rrb1jdTrK
namespace DIRE_NS
{
	namespace SelfHelpers
	{
		template <typename T>
		struct Reader
		{
			friend auto adl_GetSelfType(Reader<T>);
		};

		template <typename T, typename U>
		struct Writer
		{
			friend auto adl_GetSelfType(Reader<T>) { return U{}; }
		};

		inline void adl_GetSelfType() {}

		template <typename T>
		using Read = std::remove_pointer_t<decltype(adl_GetSelfType(Reader<T>{})) > ;
	}

}


#define DIRE_DECLARE_TYPEINFO() \
	struct DIRE_SelfTypeTag {}; \
    constexpr auto DIRE_SelfTypeHelper() -> decltype(DIRE_NS::SelfHelpers::Writer<DIRE_SelfTypeTag, decltype(this)>{}); \
    using Self = DIRE_NS::SelfHelpers::Read<DIRE_SelfTypeTag>;\
	inline static DIRE_NS::TypedReflectableTypeInfo<Self, false>	DIRE_TypeInfo{nullptr};\
	static DIRE_NS::ReflectableTypeInfo const&	GetClassReflectableTypeInfo()\
	{\
		return DIRE_TypeInfo;\
	}\
	static DIRE_NS::ReflectableTypeInfo&	EditClassReflectableTypeInfo()\
	{\
		return DIRE_TypeInfo;\
	}
//	mutable ReflectableClassIDSetter2<Self> structname##_TYPEINFO_ID_SETTER{this}; // TODO: fix this "structname"
// I had to use mutable, otherwise the MapUpdate function breaks compilation because we try to copy assign. Maybe try to find a better way

#define DIRE_REFLECTABLE_INFO() \
	struct DIRE_SelfTypeTag {}; \
    constexpr auto DIRE_SelfTypeHelper() -> decltype(::SelfType::Writer<DIRE_SelfTypeTag, decltype(this)>{}); \
    using Self = ::SelfType::Read<DIRE_SelfTypeTag>;\
	using Super = TypeInfoHelper<Self>::Super;\
	inline static TypedReflectableTypeInfo<Self, USE_DEFAULT_CONSTRUCTOR_INSTANTIATE()> DIRE_TypeInfo{TypeInfoHelper<Self>::TypeName};\
	static ReflectableTypeInfo const&	GetClassReflectableTypeInfo()\
	{\
		return DIRE_TypeInfo;\
	}\
	static ReflectableTypeInfo&	EditClassReflectableTypeInfo()\
	{\
		return DIRE_TypeInfo;\
	}\
	//mutable ReflectableClassIDSetter2<Self> structname##_TYPEINFO_ID_SETTER{this}; // TODO: fix this "structname"
	// I had to use mutable, otherwise the MapUpdate function breaks compilation because we try to copy assign. Maybe try to find a better way
