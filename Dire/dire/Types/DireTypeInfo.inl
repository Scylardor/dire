#pragma once
#include "dire/Handlers/DireArrayDataStructureHandler.h"
#include "dire/Handlers/DireEnumDataStructureHandler.h"
#include "dire/Handlers/DireMapDataStructureHandler.h"


namespace DIRE_NS
{
	template <typename Ret, typename T, typename... Args>
	Ret	Invoke(const DIRE_STRING_VIEW& pFuncName, T& pObject, Args&&... pArgs)
	{
		const TypeInfo& theClassReflector = T::EditTypeInfo();
		const FunctionInfo* funcInfo = theClassReflector.FindFunction(pFuncName);
		if constexpr (std::is_void_v<Ret>)
		{
			if (funcInfo != nullptr)
				funcInfo->InvokeWithArgs(&pObject, std::forward<Args>(pArgs)...);

			return;
		}
		else
		{
			std::any result = funcInfo->InvokeWithArgs(&pObject, std::forward<Args>(pArgs)...);
			return std::any_cast<Ret>(result);
		}
	}


	template <typename TProp>
	void PropertyTypeInfo::PopulateDataStructureHandler()
	{
		if constexpr (HasMapSemantics_v<TProp>)
		{
			myDataStructurePropertyHandler = DataStructureHandler(&TypedMapDataStructureHandler<TProp>::GetInstance());
		}
		else if constexpr (HasArraySemantics_v<TProp>)
		{
			myDataStructurePropertyHandler = DataStructureHandler(&TypedArrayDataStructureHandler<TProp>::GetInstance());
		}
		else if constexpr (std::is_base_of_v<Enum, TProp>)
		{
			myDataStructurePropertyHandler = DataStructureHandler(&TypedEnumDataStructureHandler<TProp>::GetInstance());
		}
	}

	template <typename TProp>
	void PropertyTypeInfo::SetReflectableID()
	{
		if constexpr (std::is_base_of_v<Reflectable, TProp>)
		{
			myReflectableID = TProp::GetTypeInfo().GetID();
		}
		else
		{
			myReflectableID = INVALID_REFLECTABLE_ID;
		}
	}

	template <typename TProp>
	void PropertyTypeInfo::InitializeReflectionCopyConstructor()
	{
		if constexpr (std::is_trivially_copyable_v<TProp>)
		{
			myCopyCtor = [](void* pDestAddr, void const* pSrc, size_t pOffset)
			{
				memcpy((std::byte*)pDestAddr + pOffset, (std::byte const*)pSrc + pOffset, sizeof(TProp));
			};
		}
		else if constexpr (std::is_assignable_v<TProp, TProp> && !std::is_trivially_copy_assignable_v<TProp>)
		{ // class has an overloaded operator=
			myCopyCtor = [](void* pDestAddr, void const* pSrc, size_t pOffset)
			{
				TProp* actualDestination = (TProp*)((std::byte*)pDestAddr + pOffset);
				TProp const* actualSrc = (TProp const*)((std::byte const*)pSrc + pOffset);
				(*actualDestination) = *actualSrc;
			};
		}
		else // C-style arrays, for example...
		{
			myCopyCtor = [](void* pDestAddr, void const* pOther, size_t pOffset)
			{
				TProp* actualDestination = (TProp*)((std::byte*)pDestAddr + pOffset);
				TProp const* actualSrc = (TProp const*)((std::byte const*)pOther + pOffset);

				std::copy(std::begin(*actualSrc), std::end(*actualSrc), std::begin(*actualDestination));
			};
		}
	}

	template <typename ... Args>
	std::any FunctionInfo::InvokeWithArgs(void* pCallerObject, Args&&... pFuncArgs) const
	{
		using ArgumentPackTuple = std::tuple<Args...>;
		std::any packedArgs(std::in_place_type<ArgumentPackTuple>, std::forward<Args>(pFuncArgs)...);
		std::any result = Invoke(pCallerObject, packedArgs);
		return result;
	}

	template <typename Ret, typename ... Args>
	Ret FunctionInfo::TypedInvokeWithArgs(void* pCallerObject, Args&&... pFuncArgs) const
	{
		if constexpr (std::is_void_v<Ret>)
		{
			InvokeWithArgs(pCallerObject, std::forward<Args>(pFuncArgs)...);
			return;
		}
		else
		{
			std::any result = InvokeWithArgs(pCallerObject, std::forward<Args>(pFuncArgs)...);
			return std::any_cast<Ret>(result);
		}
	}

	template <typename Class, typename Ret, typename ... Args>
	TypedFunctionInfo<Ret(Class::*)(Args...)>::TypedFunctionInfo(MemberFunctionSignature memberFunc, const char* methodName) :
		FunctionInfo(methodName, FromActualTypeToEnumType<Ret>::EnumType),
		myMemberFunctionPtr(memberFunc)
	{
		if constexpr (sizeof...(Args) > 0)
		{
			(PushBackParameterType(FromActualTypeToEnumType<Args>::EnumType), ...);
		}

		TypeInfo& theClassReflector = Class::EditTypeInfo();
		theClassReflector.PushFunctionInfo(*this);
	}

	template <typename Class, typename Ret, typename ... Args>
	std::any TypedFunctionInfo<Ret(Class::*)(Args...)>::Invoke(void* pObject, std::any const& pInvokeParams) const
	{
		using ArgumentsTuple = std::tuple<Args...>;
		ArgumentsTuple const* argPack = std::any_cast<ArgumentsTuple>(&pInvokeParams);
		if (argPack == nullptr)
		{
			return {}; // what we have been sent is actually not a tuple of the expected argument types...
		}

		auto f = [this, pObject](Args... pMemFunArgs) -> Ret
		{
			Class* theActualObject = static_cast<Class*>(pObject);
			return (theActualObject->*myMemberFunctionPtr)(pMemFunArgs...);
		};
		if constexpr (std::is_void_v<Ret>)
		{
			std::apply(f, *argPack);
			return {};
		}
		else
		{
			std::any result = std::apply(f, *argPack);
			return result;
		}
	}

	template <typename F>
	void TypeInfo::ForEachPropertyInHierarchy(F&& pVisitorFunction) const
	{
		// Iterate on the parents list in reverse order so that we traverse the class hierarchy top to bottom.
		for (auto rit = myParentClasses.rbegin(); rit != myParentClasses.rend(); ++rit)
		{
			for (auto const& prop : (*rit)->myProperties)
			{
				pVisitorFunction(prop);
			}
		}

		for (auto const& prop : myProperties)
		{
			pVisitorFunction(prop);
		}
	}

	template <typename T, bool UseDefaultCtorForInstantiate>
	TypedTypeInfo<T, UseDefaultCtorForInstantiate>::TypedTypeInfo(char const* pTypename) :
		TypeInfo(pTypename)
	{
		if constexpr (std::is_base_of_v<Reflectable, T>)
		{
			RecursiveRegisterParentClasses <typename T::Super>();
		}

		if constexpr (UseDefaultCtorForInstantiate && std::is_default_constructible_v<T>)
		{
			static_assert(std::is_base_of_v<Reflectable, T>, "This class is only supposed to be used as a member variable of a Reflectable-derived class.");
			TypeInfoDatabase::EditSingleton().RegisterInstantiateFunction(T::GetTypeInfo().GetID(),
				[](std::any const& pParams) -> Reflectable*
				{
					if (pParams.has_value()) // we've been sent parameters but this is default construction! Error
						return nullptr;

					return new T();
				}
			);
		}
	}

	template <typename T, bool UseDefaultCtorForInstantiate>
	template <typename TParent>
	void TypedTypeInfo<T, UseDefaultCtorForInstantiate>::RecursiveRegisterParentClasses()
	{
		if constexpr (!std::is_same_v<TParent, Reflectable>)
		{
			TypeInfo& parentTypeInfo = TParent::EditTypeInfo();
			parentTypeInfo.AddChildClass(this);
			this->AddParentClass(&parentTypeInfo);
			RecursiveRegisterParentClasses<typename TParent::Super>();
		}
	}
}
