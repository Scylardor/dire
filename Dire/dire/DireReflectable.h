#pragma once
#include "Types/DireTypeInfo.h"
#include "Handlers/DireArrayDataStructureHandler.h"
#include "Utils/DireMacros.h"
#include "Utils/DireString.h"
#include "DireReflectableID.h"

#include <any>

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
	public:
		virtual ~Reflectable() = default;

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
			else
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

		using ParseError = DIRE_STRING;

		template <typename T>
		struct PropertyAccessor
		{
			PropertyAccessor(const T* pPropAddr) :
				myValue(pPropAddr)
			{}

			PropertyAccessor(ParseError&& pError) :
				myValue(std::move(pError))
			{}

			[[nodiscard]] bool	IsValid() const
			{
				return std::holds_alternative<const T*>(myValue);
			}

			[[nodiscard]] const T* GetPointer() const
			{
				auto ptr = std::get_if<const T*>(&myValue);
				return ptr ? *ptr : nullptr;
			}

			[[nodiscard]] const ParseError* GetError() const
			{
				return std::get_if<ParseError>(&myValue);
			}

			operator const T* () const
			{
				return GetPointer();
			}

		private:
			std::variant<const T*, ParseError>	myValue;

		};

		template <typename TProp = void>
		[[nodiscard]] PropertyAccessor<TProp> GetProperty(DIRE_STRING_VIEW pName) const
		{
			GetPropertyResult result = GetPropertyImpl(pName);
			if (result.Error.empty())
			{
				return PropertyAccessor<TProp>(static_cast<const TProp*>(result.Address));
			}

			return PropertyAccessor<TProp>(std::move(result.Error));
		}

		/* Version that returns a reference for when you are 100% confident this property exists */
		template <typename TProp>
		[[nodiscard]] const TProp & GetSafeProperty(DIRE_STRING_VIEW pName) const
		{
			const TProp * propPtr = GetProperty<TProp>(pName);
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

		[[nodiscard]] Dire_EXPORT bool EraseProperty(DIRE_STRING_VIEW pName);

		[[nodiscard]] Dire_EXPORT const FunctionInfo * GetFunction(DIRE_STRING_VIEW pMemberFuncName) const;

		template <typename... Args>
		std::any	InvokeFunction(DIRE_STRING_VIEW pMemberFuncName, Args&&... pFuncArgs)
		{
			const FunctionInfo * theFunction = GetFunction(pMemberFuncName);
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

		struct GetPropertyResult
		{
			GetPropertyResult() :
				Error("Syntax error")
			{}

			GetPropertyResult(const void* pAddr, const PropertyTypeInfo* pInfo) :
				Address(pAddr), TypeInfo(pInfo)
			{}

			GetPropertyResult(const ParseError& pError) :
				Error(pError)
			{}

			const void* Address = nullptr;
			const PropertyTypeInfo* TypeInfo = nullptr;
			ParseError Error;
		};

		[[nodiscard]] Dire_EXPORT GetPropertyResult GetPropertyImpl(DIRE_STRING_VIEW pFullPath) const;

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
	namespace DIRE_NS\
	{\
		template <typename T>\
		struct SuperDetector;\
		template <>\
		struct SuperDetector<ObjectType>\
		{\
			using Super = DIRE_VA_MACRO(DIRE_FIRST_TYPE_OR_REFLECTABLE_, __VA_ARGS__);\
		};\
	}\
	ObjectType : DIRE_VA_MACRO(DIRE_INHERITANCE_LIST, __VA_ARGS__)

#define DIRE_REFLECTABLE_INFO() \
	struct DIRE_SelfTypeTag{}; \
    constexpr auto DIRE_SelfTypeHelper() -> decltype(::DIRE_NS::SelfType::Writer<DIRE_SelfTypeTag, decltype(this)>{}, void()) {} \
    using Self = ::DIRE_NS::SelfType::Read<DIRE_SelfTypeTag>;\
	using Super = typename DIRE_NS::SuperDetector<Self>::Super; \
	static const DIRE_STRING&	DIRE_GetFullyQualifiedName()\
	{\
		static const DIRE_STRING fullyQualifiedName = []{\
			DIRE_STRING funcName = DIRE_FUNCTION_FULLNAME;\
			std::size_t found = funcName.rfind("::");\
			found = funcName.rfind("::", found-1);\
			found = funcName.rfind("::", found-1);\
			if (found != DIRE_STRING::npos)\
			{\
				funcName = funcName.substr(0, found);\
				auto startIdx = funcName.find(' ');\
				if (startIdx != funcName.npos)\
					funcName = funcName.substr(startIdx+1, found - startIdx);\
			}\
			return funcName;\
		}(); \
		return fullyQualifiedName;\
	}\
	inline static ::DIRE_NS::TypedTypeInfo<Self, DIRE_DEFAULT_CONSTRUCTOR_INSTANTIATE>	DIRE_TypeInfo{ DIRE_GetFullyQualifiedName().c_str() }; \
	\
	static const ::DIRE_NS::TypedTypeInfo<Self, DIRE_DEFAULT_CONSTRUCTOR_INSTANTIATE>& GetTypeInfo()\
	{\
		return DIRE_TypeInfo; \
	}\
	static ::DIRE_NS::TypedTypeInfo<Self, DIRE_DEFAULT_CONSTRUCTOR_INSTANTIATE>& EditTypeInfo()\
	{\
		return DIRE_TypeInfo; \
	}\
	[[nodiscard]] virtual ::DIRE_NS::ReflectableID	GetReflectableClassID() const override\
	{\
		return GetTypeInfo().GetID();\
	}\
	[[nodiscard]] virtual const ::DIRE_NS::TypeInfo* GetReflectableTypeInfo() const override\
	{\
		return &GetTypeInfo();\
	}
