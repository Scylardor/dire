#pragma once
#include <any>
#include <cstddef> // std::byte

#include "DireTypeInfo.h"
#include "DireMacros.h"
#include "DireArrayDataStructureHandler.h"
#include "DireAssert.h"
#include "DireString.h"

namespace DIRE_NS
{
	template <typename T, typename... Args>
	T*	AllocateReflectable(Args&&... pCtorArgs)
	{
		static_assert(std::is_base_of_v<Reflectable, T>, "AllocateReflectable is supposed to be used only for Reflectable-derived classes.");

		DIRE_ALLOCATOR<T> allocator;
		T* mem = allocator.allocate(1);
		DIRE_ASSERT(mem != nullptr);
		using traits_t = std::allocator_traits<DIRE_ALLOCATOR<T>>;
		traits_t::construct(allocator, mem, pCtorArgs...);
		return mem;
	}

	template <typename T, typename... Args>
	struct ClassInstantiator final
	{
		ClassInstantiator()
		{
			static_assert(std::is_base_of_v<Reflectable, T>, "ClassInstantiator is only meant to be used as a member of Reflectable-derived classes.");
			static_assert(std::is_constructible_v<T, Args...>,
				"No constructor associated with the parameter types of declared instantiator. Please keep instantiator and constructor synchronized.");
			TypeInfoDatabase::EditSingleton().RegisterInstantiateFunction(T::GetTypeInfo().GetID(), &Instantiate);
		}

		static Reflectable* Instantiate(std::any const& pCtorParams)
		{
			using ArgumentPackTuple = std::tuple<Args...>;
			ArgumentPackTuple const* argsTuple = std::any_cast<ArgumentPackTuple>(&pCtorParams);
			if (argsTuple == nullptr) // i.e. we were sent garbage
			{
				return nullptr;
			}
			auto f = [](Args... pCtorArgs) -> T*
			{
				return AllocateReflectable<T>(pCtorArgs...);
			};
			T* result = std::apply(f, *argsTuple);
			return result;
		}
	};

#define DECLARE_INSTANTIATOR(...)  \
	inline static DIRE_NS::ClassInstantiator<Self, __VA_ARGS__> DIRE_INSTANTIATOR;

	class Reflectable
	{
		using ParseError = DIRE_STRING;

		struct GetPropertyResult
		{
			GetPropertyResult() = default;
			GetPropertyResult(const void* pAddr, const PropertyTypeInfo* pInfo, const ParseError& pError = "") :
				Address(pAddr), TypeInfo(pInfo), Error(pError)
			{}

			GetPropertyResult(const ParseError& pError) :
				Error(pError)
			{}

			const void* Address = nullptr;
			const PropertyTypeInfo* TypeInfo = nullptr;
			ParseError Error;
		};

	public:

		[[nodiscard]] virtual ReflectableID		GetReflectableClassID() const = 0;
		[[nodiscard]] virtual const TypeInfo*	GetReflectableTypeInfo() const = 0;

		template <typename TRefl>
		[[nodiscard]] bool	IsA() const
		{
			static_assert(std::is_base_of_v<Reflectable, TRefl>, "IsA only works with Reflectable-derived class types.");

			// Special case to manage IsA<Reflectable>, because since all Reflectable are derived from it *implicitly*, IsParentOf would return false.
			if constexpr (std::is_same_v<TRefl, Reflectable>)
			{
				return true;
			}

			TypeInfo const& parentTypeInfo = TRefl::GetTypeInfo();
			return (parentTypeInfo.GetID() == GetReflectableClassID() || parentTypeInfo.IsParentOf(GetReflectableClassID()));
		}

		template <typename T = Reflectable>
		T* Clone()
		{
			static_assert(std::is_base_of_v<Reflectable, T>, "Clone only works with Reflectable-derived class types.");

			const TypeInfo * thisTypeInfo = GetReflectableTypeInfo();
			if (thisTypeInfo == nullptr)
			{
				return nullptr;
			}

			Reflectable* clone = Instantiate();
			if (clone == nullptr)
			{
				return nullptr;
			}

			thisTypeInfo->CloneHierarchyPropertiesOf(*clone, *this);

			return (T*)clone;
		}

		template <typename... Args>
		Reflectable*	Instantiate(Args&&... pArgs)
		{
			if constexpr (sizeof...(Args) == 0)
			{
				return  TypeInfoDatabase::GetSingleton().TryInstantiate(GetReflectableClassID(), {});

			}
			return TypeInfoDatabase::GetSingleton().TryInstantiate(GetReflectableClassID(), {std::tuple<Args...>(std::forward<Args>(pArgs)...)});
		}

		void	CloneProperties(Reflectable const* pCloned, const TypeInfo * pClonedTypeInfo, Reflectable* pClone)
		{
			pClonedTypeInfo->CloneHierarchyPropertiesOf(*pClone, *pCloned);
		}


		[[nodiscard]] IntrusiveLinkedList<PropertyTypeInfo> const& GetProperties() const
		{
			return GetReflectableTypeInfo()->GetPropertyList();
		}

		[[nodiscard]] IntrusiveLinkedList<FunctionInfo> const& GetMemberFunctions() const
		{
			return GetReflectableTypeInfo()->GetFunctionList();
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

			throw std::runtime_error("GetSafeProperty was called but the property didn't exist.");
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

		Dire_EXPORT [[nodiscard]] bool EraseProperty(DIRE_STRING_VIEW pName);

		Dire_EXPORT [[nodiscard]] FunctionInfo const* GetFunction(DIRE_STRING_VIEW pMemberFuncName) const;

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

		Dire_EXPORT [[nodiscard]] const GetPropertyResult GetPropertyImpl(DIRE_STRING_VIEW pFullPath) const;

		[[nodiscard]] GetPropertyResult GetArrayProperty(const TypeInfo * pTypeInfoOwner, DIRE_STRING_VIEW pName, DIRE_STRING_VIEW pRemainingPath, int pArrayIdx, const std::byte * pPropPtr) const;

		[[nodiscard]] GetPropertyResult RecurseFindArrayProperty(const IArrayDataStructureHandler * pArrayHandler,
			DIRE_STRING_VIEW pName, DIRE_STRING_VIEW pRemainingPath, int pArrayIdx, const std::byte * pPropPtr) const;

		[[nodiscard]] GetPropertyResult RecurseArrayMapProperty(const PropertyTypeInfo * pProperty,
			const std::byte * pPropPtr, DIRE_STRING_VIEW pRemainingPath, DIRE_STRING_VIEW pKey) const;

		[[nodiscard]] GetPropertyResult RecurseArrayProperty(const IArrayDataStructureHandler* pArrayHandler, const std::byte* pPropPtr,
			DIRE_STRING_VIEW pRemainingPath, DIRE_STRING_VIEW pKey) const;

		[[nodiscard]] GetPropertyResult	RecurseMapProperty(const IMapDataStructureHandler* pMapHandler, const std::byte* pPropPtr,
			DIRE_STRING_VIEW pRemainingPath, DIRE_STRING_VIEW pKey) const;

		[[nodiscard]] GetPropertyResult GetArrayMapProperty(const TypeInfo * pTypeInfoOwner, DIRE_STRING_VIEW pName, DIRE_STRING_VIEW pRemainingPath, DIRE_STRING_VIEW pKey, const std::byte * pPropPtr) const;

		[[nodiscard]] GetPropertyResult GetCompoundProperty(const TypeInfo * pTypeInfoOwner, DIRE_STRING_VIEW pName, DIRE_STRING_VIEW pFullPath, const std::byte * propertyAddr) const;
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
			if (found != DIRE_STRING::npos)\
				funcName = funcName.substr(0, found);\
			return funcName;\
		}(); \
		return fullyQualifiedName;\
	}\
	inline static DIRE_NS::TypedTypeInfo<Self, DIRE_DEFAULT_CONSTRUCTOR_INSTANTIATE>	DIRE_TypeInfo{ DIRE_GetFullyQualifiedName().c_str() }; \
	\
	static const DIRE_NS::TypedTypeInfo<Self, DIRE_DEFAULT_CONSTRUCTOR_INSTANTIATE>& GetTypeInfo()\
	{\
		return DIRE_TypeInfo; \
	}\
	static DIRE_NS::TypedTypeInfo<Self, DIRE_DEFAULT_CONSTRUCTOR_INSTANTIATE>& EditTypeInfo()\
	{\
		return DIRE_TypeInfo; \
	}\
	[[nodiscard]] virtual DIRE_NS::ReflectableID	GetReflectableClassID() const override\
	{\
		return GetTypeInfo().GetID();\
	}\
	[[nodiscard]] virtual const DIRE_NS::TypeInfo* GetReflectableTypeInfo() const override\
	{\
		return &GetTypeInfo();\
	}