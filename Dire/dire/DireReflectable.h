#pragma once
#include <any>
#include <cassert>
#include <cstdlib> // atoi
#include <cstddef> // std::byte
#include <any>

#include "DireTypeInfo.h"
#include "DireMacros.h"
#include "DireArrayDataStructureHandler.h"
#include "DireString.h"

namespace std
{
	enum class byte : unsigned char;
}

namespace DIRE_NS
{
	template <typename T, typename... Args>
	struct ClassInstantiator final
	{
		ClassInstantiator()
		{
			static_assert(std::is_base_of_v<Reflectable2, T>, "ClassInstantiator is only meant to be used as a member of Reflectable-derived classes.");
			static_assert(std::is_constructible_v<T, Args...>,
				"No constructor associated with the parameter types of declared instantiator. Please keep instantiator and constructor synchronized.");
			Reflector3::EditSingleton().RegisterInstantiateFunction(T::GetClassReflectableTypeInfo().GetID(), &Instantiate);
		}

		static Reflectable2* Instantiate(std::any const& pCtorParams)
		{
			using ArgumentPackTuple = std::tuple<Args...>;
			ArgumentPackTuple const* argsTuple = std::any_cast<ArgumentPackTuple>(&pCtorParams);
			if (argsTuple == nullptr) // i.e. we were sent garbage
			{
				return nullptr;
			}
			auto f = [](Args... pCtorArgs) -> Reflectable2*
			{
				return new T(pCtorArgs...); // TODO: allow for custom allocator usage
			};
			Reflectable2* result = std::apply(f, *argsTuple);
			return result;
		}
	};

#define DECLARE_INSTANTIATOR(...)  \
	inline static DIRE_NS::ClassInstantiator<Self, __VA_ARGS__> DIRE_INSTANTIATOR;


	using ReflectableID = unsigned;

	static const ReflectableID INVALID_REFLECTABLE_ID = (ReflectableID)-1;

	class Reflectable2
	{
		struct GetPropertyResult
		{
			GetPropertyResult() = default;
			GetPropertyResult(const void* pAddr, const PropertyTypeInfo* pInfo) :
				Address(pAddr), TypeInfo(pInfo)
			{}

			const void* Address = nullptr;
			const PropertyTypeInfo* TypeInfo = nullptr;
		};

	public:
		using ClassID = unsigned;

		[[nodiscard]] virtual ClassID			GetReflectableClassID() const = 0;
		[[nodiscard]] virtual const TypeInfo*	GetTypeInfo() const = 0;

		template <typename TRefl>
		[[nodiscard]] bool	IsA() const
		{
			static_assert(std::is_base_of_v<Reflectable2, TRefl>, "IsA only works with Reflectable-derived class types.");

			// Special case to manage IsA<Reflectable>, because since all Reflectable are derived from it *implicitly*, IsParentOf would return false.
			if constexpr (std::is_same_v<TRefl, Reflectable2>)
			{
				return true;
			}

			TypeInfo const& parentTypeInfo = TRefl::GetClassReflectableTypeInfo();
			return (parentTypeInfo.GetID() == GetReflectableClassID() || parentTypeInfo.IsParentOf(GetReflectableClassID()));
		}

		template <typename T = Reflectable2>
		T* Clone()
		{
			static_assert(std::is_base_of_v<Reflectable2, T>, "Clone only works with Reflectable-derived class types.");

			TypeInfo const* thisTypeInfo = GetTypeInfo();
			if (thisTypeInfo == nullptr)
			{
				return nullptr;
			}

			Reflectable2* clone = Instantiate();
			if (clone == nullptr)
			{
				return nullptr;
			}

			thisTypeInfo->CloneHierarchyPropertiesOf(*clone, *this);

			return (T*)clone;
		}

		template <typename... Args>
		Reflectable2*	Instantiate(Args&&... pArgs)
		{
			if constexpr (sizeof...(Args) == 0)
			{
				return  Reflector3::GetSingleton().TryInstantiate(GetReflectableClassID(), {});

			}
			return Reflector3::GetSingleton().TryInstantiate(GetReflectableClassID(), {std::tuple<Args...>(std::forward<Args>(pArgs)...)});
		}

		void	CloneProperties(Reflectable2 const* pCloned, TypeInfo const* pClonedTypeInfo, Reflectable2* pClone)
		{
			pClonedTypeInfo->CloneHierarchyPropertiesOf(*pClone, *pCloned);
		}


		[[nodiscard]] IntrusiveLinkedList<PropertyTypeInfo> const& GetProperties() const
		{
			return GetTypeInfo()->GetPropertyList();
		}

		[[nodiscard]] IntrusiveLinkedList<FunctionInfo> const& GetMemberFunctions() const
		{
			return GetTypeInfo()->GetFunctionList();
		}

		template <typename T>
		T const* GetMemberAtOffset(ptrdiff_t pOffset) const
		{
			return reinterpret_cast<T const*>(reinterpret_cast<std::byte const*>(this) + pOffset);
		}

		template <typename T>
		T* EditMemberAtOffset(ptrdiff_t pOffset)
		{
			return reinterpret_cast<T*>(reinterpret_cast<std::byte*>(this) + pOffset);
		}

		template <typename TProp = void>
		[[nodiscard]] const TProp * GetProperty(DIRE_STRING_VIEW pName) const
		{
			GetPropertyResult result = GetPropertyImpl(pName);
			return static_cast<const TProp*>(result.Address);
		}

		/* Version that returns a reference for when you are 100% confident this property exists */
		template <typename TProp>
		[[nodiscard]] TProp const& GetSafeProperty(DIRE_STRING_VIEW pName) const
		{
			TProp const* propPtr = GetProperty<TProp>(pName);
			if (propPtr != nullptr)
			{
				return *propPtr;
			}

			// throw exception, or assert
			// TODO: check if exceptions are enabled and allow to customize assert macro
			assert(false);
			throw std::runtime_error("bad");
		}

		template <typename TProp>
		bool SetProperty(DIRE_STRING_VIEW pName, TProp&& pSetValue)
		{
			TProp const* propPtr = GetProperty<TProp>(pName);
			if (propPtr == nullptr)
			{
				return false;
			}

			TProp* editablePropPtr = const_cast<TProp*>(propPtr);
			(*editablePropPtr) = std::forward<TProp>(pSetValue);
			return true;
		}

		[[nodiscard]] bool EraseProperty(DIRE_STRING_VIEW pName);

		[[nodiscard]] const GetPropertyResult GetPropertyImpl(DIRE_STRING_VIEW pName) const;

		[[nodiscard]] FunctionInfo const* GetFunction(DIRE_STRING_VIEW pMemberFuncName) const;

		template <typename... Args>
		std::any	InvokeFunction(DIRE_STRING_VIEW pMemberFuncName, Args&&... pFuncArgs)
		{
			FunctionInfo const* theFunction = GetFunction(pMemberFuncName);
			if (theFunction == nullptr)
			{
				return {};
			}

			return theFunction->InvokeWithArgs(this, std::forward<Args>(pFuncArgs)...);
		}

		template <typename Ret, typename... Args>
		Ret	TypedInvokeFunction(DIRE_STRING_VIEW pMemberFuncName, Args&&... pFuncArgs)
		{
			if constexpr (std::is_void_v<Ret>)
			{
				InvokeFunction(pMemberFuncName, std::forward<Args>(pFuncArgs)...);
				return;
			}
			else
			{
				std::any result = InvokeFunction(pMemberFuncName, std::forward<Args>(pFuncArgs)...);
				return std::any_cast<Ret>(result);
			}
		}

	private:

		[[nodiscard]] GetPropertyResult GetArrayProperty(TypeInfo const* pTypeInfoOwner, DIRE_STRING_VIEW pName, DIRE_STRING_VIEW pRemainingPath, int pArrayIdx, std::byte const* pPropPtr) const;

		[[nodiscard]] GetPropertyResult RecurseFindArrayProperty(ArrayDataStructureHandler const* pArrayHandler,
			DIRE_STRING_VIEW pName, DIRE_STRING_VIEW pRemainingPath, int pArrayIdx, std::byte const* pPropPtr) const;

		[[nodiscard]] GetPropertyResult RecurseArrayMapProperty(DataStructureHandler const& pDataStructureHandler,
			std::byte const* pPropPtr, DIRE_STRING_VIEW pRemainingPath, DIRE_STRING_VIEW pKey) const;

		[[nodiscard]] GetPropertyResult GetArrayMapProperty(TypeInfo const* pTypeInfoOwner, DIRE_STRING_VIEW pName, DIRE_STRING_VIEW pRemainingPath, DIRE_STRING_VIEW pKey, std::byte const* pPropPtr) const;

		[[nodiscard]] GetPropertyResult GetCompoundProperty(TypeInfo const* pTypeInfoOwner, DIRE_STRING_VIEW pName, DIRE_STRING_VIEW pFullPath, std::byte const* propertyAddr) const;
	};
}

#define dire_reflectable(ObjectType, ...) \
	ObjectType;\
	template <typename T>\
	struct DIRE_TypeInfoHelper;\
	template <>\
	struct DIRE_TypeInfoHelper<ObjectType>\
	{\
		using Super = DIRE_VA_MACRO(DIRE_FIRST_TYPE_OR_REFLECTABLE_, __VA_ARGS__);\
	};\
	ObjectType : DIRE_VA_MACRO(DIRE_INHERITANCE_LIST_OR_REFLECTABLE, __VA_ARGS__)

#define DIRE_REFLECTABLE_INFO() \
	struct DIRE_SelfTypeTag {}; \
	constexpr auto DIRE_SelfTypeHelper() -> decltype(DIRE_NS::SelfHelpers::Writer<DIRE_SelfTypeTag, decltype(this)>{}); \
	using Self = DIRE_NS::SelfHelpers::Read<DIRE_SelfTypeTag>; \
	using Super = DIRE_TypeInfoHelper<Self>::Super; \
	static const DIRE_STRING&	DIRE_GetFullyQualifiedName()\
	{\
		static const DIRE_STRING fullyQualifiedName = []{\
			DIRE_STRING funcName = DIRE_FUNCTION_FULLNAME;\
			std::size_t found = funcName.rfind("::");\
			found = funcName.rfind("::", found-1);\
			found = funcName.rfind("::", found-1);\
			if (found != std::string::npos)\
				funcName = funcName.substr(0, found);\
			return funcName;\
		}(); \
		return fullyQualifiedName;\
	}\
	inline static DIRE_NS::TypedTypeInfo<Self, DIRE_DEFAULT_CONSTRUCTOR_INSTANTIATE>	DIRE_TypeInfo{ DIRE_GetFullyQualifiedName().c_str() }; \
	\
	static const DIRE_NS::TypeInfo & GetClassReflectableTypeInfo()\
	{\
		return DIRE_TypeInfo; \
	}\
	static DIRE_NS::TypeInfo& EditClassReflectableTypeInfo()\
	{\
		return DIRE_TypeInfo; \
	}\
	[[nodiscard]] virtual ClassID	GetReflectableClassID() const override\
	{\
		return GetClassReflectableTypeInfo().GetID();\
	}\
	[[nodiscard]] virtual const DIRE_NS::TypeInfo* GetTypeInfo() const\
	{\
		return &GetClassReflectableTypeInfo();\
	}