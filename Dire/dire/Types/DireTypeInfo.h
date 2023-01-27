#pragma once

#include "DireDefines.h"
#include "dire/Utils/DireIntrusiveList.h"
#include "dire/Utils/DireString.h"
#include "dire/Handlers/DireTypeHandlers.h"
#include "dire/Types/DireTypes.h"

#include "DireTypeInfoDatabase.h"

#include <any>
#include <vector>

namespace DIRE_NS
{
	class Reflectable;

	class PropertyTypeInfo : public IntrusiveListNode<PropertyTypeInfo>
	{
		using CopyConstructorPtr = void (*)(void* pDestAddr, const void * pOther, size_t pOffset);

	public:
		Dire_EXPORT PropertyTypeInfo(const char * pName, const std::ptrdiff_t pOffset, const size_t pSize, const CopyConstructorPtr pCopyCtor = nullptr) :
			myName(pName), myMetatype(MetaType::Unknown), myOffset(pOffset), mySize(pSize), myCopyCtor(pCopyCtor)
		{}

		Dire_EXPORT [[nodiscard]] const IArrayDataStructureHandler* GetArrayHandler() const
		{
			return myDataStructurePropertyHandler.GetArrayHandler();
		}

		Dire_EXPORT [[nodiscard]] const IMapDataStructureHandler* GetMapHandler() const
		{
			return myDataStructurePropertyHandler.GetMapHandler();
		}

		Dire_EXPORT [[nodiscard]] const DataStructureHandler& GetDataStructureHandler() const
		{
			return myDataStructurePropertyHandler;
		}

		[[nodiscard]] const DIRE_STRING_VIEW& GetName() const { return myName; }

		[[nodiscard]] MetaType		GetMetatype() const { return myMetatype; }

		[[nodiscard]] size_t	GetOffset() const { return myOffset; }

		[[nodiscard]] size_t	GetSize() const { return mySize; }

		[[nodiscard]] ReflectableID	GetReflectableID() const { return myReflectableID; }

		[[nodiscard]] CopyConstructorPtr	GetCopyConstructorFunction() const { return myCopyCtor; }

#ifdef DIRE_SERIALIZATION_ENABLED
		 virtual void	SerializeAttributes(class ISerializer& pSerializer) const = 0;

		struct SerializationState
		{
			bool	IsSerializable = false;
			bool	HasAttributesToSerialize = false;
		};

		 virtual SerializationState	GetSerializableState() const = 0;
#endif

	protected:

		template <typename TProp>
		void	SelectType()
		{
			SetType(FromActualTypeToEnumType<TProp>::EnumType);
		}

		template <typename TProp>
		void	PopulateDataStructureHandler()
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
		void	SetReflectableID()
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
		void	InitializeReflectionCopyConstructor()
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

		void	SetType(const MetaType pType)
		{
			myMetatype = pType;
		}

	private:

		DIRE_STRING_VIEW		myName;
		MetaType				myMetatype;
		std::ptrdiff_t			myOffset;
		std::size_t				mySize;
		DataStructureHandler	myDataStructurePropertyHandler; // Useful for array-like or associative data structures, will stay null for other types.
		CopyConstructorPtr		myCopyCtor = nullptr; // if null : this type is not copy-constructible

		// Storing the reflectable ID here could be considered a kind of hack.
		// But given that it's an unsigned (by default), all other options would basically take more memory than "just" copying it everywhere!...
		ReflectableID		myReflectableID = INVALID_REFLECTABLE_ID;
	};


	class IFunctionTypeInfo
	{
	public:
		virtual ~IFunctionTypeInfo() = default;

		virtual std::any	Invoke(void* pObject, const std::any & pInvokeParams) const = 0;
	};

	class FunctionInfo : public IntrusiveListNode<FunctionInfo>, IFunctionTypeInfo
	{
	public:
		using ParameterList = std::vector<MetaType, DIRE_ALLOCATOR<MetaType>>;

		FunctionInfo(const char* pName, MetaType pReturnType) :
			Name(pName), ReturnType(pReturnType)
		{}

		// Not using perfect forwarding here because std::any cannot hold references: https://stackoverflow.com/questions/70239155/how-to-cast-stdtuple-to-stdany
		// Sad but true, gotta move forward with our lives.
		template <typename... Args>
		std::any	InvokeWithArgs(void* pCallerObject, Args&&... pFuncArgs) const
		{
			using ArgumentPackTuple = std::tuple<Args...>;
			std::any packedArgs(std::in_place_type<ArgumentPackTuple>, std::forward<Args>(pFuncArgs)...);
			std::any result = Invoke(pCallerObject, packedArgs);
			return result;
		}

		template <typename Ret, typename... Args>
		Ret	TypedInvokeWithArgs(void* pCallerObject, Args&&... pFuncArgs) const
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

		const DIRE_STRING&		GetName() const { return Name; }
		MetaType				GetReturnType() const { return ReturnType; }
		const ParameterList&	GetParametersTypes() const { return Parameters; }

	protected:

		void	PushBackParameterType(MetaType pNewParamType)
		{
			Parameters.push_back(pNewParamType);
		}

	private:

		DIRE_STRING		Name;
		MetaType		ReturnType{ MetaType::Void };
		ParameterList	Parameters;
	};

	template <typename Ty >
	struct TypedFunctionInfo; // not defined

	template <typename Class, typename Ret, typename ... Args >
	struct TypedFunctionInfo<Ret(Class::*)(Args...)> final : FunctionInfo
	{
		using MemberFunctionSignature = Ret(Class::*)(Args...);

		TypedFunctionInfo(MemberFunctionSignature memberFunc, const char* methodName) :
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

		// The idea of using std::apply has been stolen from here : https://stackoverflow.com/a/36656413/1987466
		std::any	Invoke(void* pObject, std::any const& pInvokeParams) const override
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

		MemberFunctionSignature	myMemberFunctionPtr = nullptr;
	};

	template <typename Ret = void, typename T, typename... Args>
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


	class TypeInfo
	{
	public:
		using TypeInfoList = std::vector<TypeInfo*, DIRE_ALLOCATOR<TypeInfo*>>;

		TypeInfo(const char* pTypename) :
			ReflectableID(TypeInfoDatabase::EditSingleton().RegisterTypeInfo(this)),
			TypeName(pTypename)
		{}

		void	PushTypeInfo(PropertyTypeInfo& pNewTypeInfo)
		{
			Properties.PushBackNewNode(pNewTypeInfo);
		}

		void	PushFunctionInfo(FunctionInfo& pNewFunctionInfo)
		{
			MemberFunctions.PushBackNewNode(pNewFunctionInfo);
		}

		void	AddParentClass(TypeInfo* pParent)
		{
			ParentClasses.push_back(pParent);
		}

		void	AddChildClass(TypeInfo* pChild)
		{
			ChildrenClasses.push_back(pChild);
		}

		[[nodiscard]] bool	IsParentOf(const ReflectableID pChildClassID, bool pIncludingMyself = true) const
		{
			// same logic as std::is_base_of: class is parent of itself (unless we strictly want only children)
			if (pChildClassID == GetID())
			{
				return pIncludingMyself;
			}

			auto predicate = [pChildClassID](const TypeInfo* pChildTypeInfo)
			{
				return pChildTypeInfo->ReflectableID == pChildClassID;
			};

			return std::find_if(ChildrenClasses.begin(), ChildrenClasses.end(), predicate) != ChildrenClasses.end();
		}

		using ParentPropertyInfo = std::pair< const TypeInfo*, const PropertyTypeInfo*>;
		[[nodiscard]] ParentPropertyInfo	FindParentClassProperty(const DIRE_STRING_VIEW& pName) const
		{
			for (const TypeInfo* parentClass : ParentClasses)
			{
				const PropertyTypeInfo* foundProp = parentClass->FindProperty(pName);
				if (foundProp != nullptr)
				{
					return { parentClass, foundProp };
				}
			}

			return { nullptr, nullptr };
		}

		[[nodiscard]] const PropertyTypeInfo * FindProperty(const DIRE_STRING_VIEW& pName) const
		{
			for (const auto& prop : Properties)
			{
				if (prop.GetName() == pName)
				{
					return &prop;
				}
			}

			return nullptr;
		}

		[[nodiscard]] const PropertyTypeInfo * FindPropertyInHierarchy(const DIRE_STRING_VIEW& pName) const
		{
			for (auto const& prop : Properties)
			{
				if (prop.GetName() == pName)
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

		using ParentFunctionInfo = std::pair<const TypeInfo*, const FunctionInfo*>;
		[[nodiscard]] ParentFunctionInfo	FindParentFunction(const DIRE_STRING_VIEW& pFuncName) const
		{
			for (const TypeInfo* parentClass : ParentClasses)
			{
				const FunctionInfo* foundFunc = parentClass->FindFunction(pFuncName);
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

		void	SetID(const ReflectableID pNewID)
		{
			ReflectableID = pNewID;
		}

		[[nodiscard]] ReflectableID	GetID() const
		{
			return ReflectableID;
		}

		Dire_EXPORT void	CloneHierarchyPropertiesOf(Reflectable& pNewClone, const Reflectable& pCloned) const;

		void	ClonePropertiesOf(Reflectable& pNewClone, const Reflectable& pCloned) const;

		[[nodiscard]] const IntrusiveLinkedList<PropertyTypeInfo>& GetPropertyList() const
		{
			return Properties;
		}

		[[nodiscard]] const IntrusiveLinkedList<FunctionInfo>& GetFunctionList() const
		{
			return MemberFunctions;
		}

		[[nodiscard]] const TypeInfoList& GetParentClasses() const
		{
			return ParentClasses;
		}

		[[nodiscard]] const TypeInfoList& GetChildrenClasses() const
		{
			return ChildrenClasses;
		}

	protected:
		ReflectableID							ReflectableID = INVALID_REFLECTABLE_ID;
		DIRE_STRING_VIEW						TypeName;
		IntrusiveLinkedList<PropertyTypeInfo>	Properties;
		IntrusiveLinkedList<FunctionInfo>		MemberFunctions;
		TypeInfoList	ParentClasses;
		TypeInfoList	ChildrenClasses;
	};

	template <typename T, bool UseDefaultCtorForInstantiate>
	class TypedTypeInfo final : public TypeInfo
	{
	public:
		TypedTypeInfo(char const* pTypename) :
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

	private:
		template <typename TParent>
		void	RecursiveRegisterParentClasses()
		{
			if constexpr (!std::is_same_v<TParent, Reflectable>)
			{
				TypeInfo& parentTypeInfo = TParent::EditTypeInfo();
				parentTypeInfo.AddChildClass(this);
				this->AddParentClass(&parentTypeInfo);
				RecursiveRegisterParentClasses<typename TParent::Super>();
			}
		}
	};
}

#define DIRE_LPAREN (
#define DIRE_RPAREN )

// inspired by https://www.mikeash.com/pyblog/friday-qa-2015-03-20-preprocessor-abuse-and-optional-parentheses.html
#define DIRE_EXTRACT(...) DIRE_EXTRACT __VA_ARGS__
#define DIRE_NOTHING_DIRE_EXTRACT
#define DIRE_PASTE(x, ...) x ## __VA_ARGS__
#define DIRE_EVALUATING_PASTE(x, ...) DIRE_PASTE(x, __VA_ARGS__)

#define DIRE_UNPAREN(...) DIRE_EVALUATING_PASTE(DIRE_NOTHING_, DIRE_EXTRACT __VA_ARGS__)

#define DIRE_FUNCTION_TYPEINFO(Name) \
	inline static DIRE_NS::TypedFunctionInfo<decltype(&Name)> Name##_TYPEINFO{ &Name, DIRE_STRINGIZE(Name)};

#define DIRE_FUNCTION(RetType, Name, ...) \
	RetType Name DIRE_LPAREN DIRE_UNPAREN(__VA_ARGS__) DIRE_RPAREN ; \
	DIRE_FUNCTION_TYPEINFO(Name);


#define DIRE_DECLARE_TYPEINFO() \
	typedef auto DIRE_SelfDetector() -> std::remove_reference<decltype(*this)>::type;\
    using Self = decltype(((DIRE_SelfDetector*)0)());\
	inline static DIRE_NS::TypedTypeInfo<Self, false>	DIRE_TypeInfo{""};\
	static DIRE_NS::TypeInfo const&	GetTypeInfo()\
	{\
		return DIRE_TypeInfo;\
	}\
	static DIRE_NS::TypeInfo&	EditTypeInfo()\
	{\
		return DIRE_TypeInfo;\
	}
