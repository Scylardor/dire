#pragma once

#include "DireDefines.h"
#include "dire/Utils/DireIntrusiveList.h"
#include "dire/Utils/DireString.h"
#include "dire/Handlers/DireTypeHandlers.h"
#include "dire/Types/DireTypes.h"

#include "DireTypeInfoDatabase.h"

#include "dire/DireReflectableID.h"

#include <any>
#include <vector>

namespace DIRE_NS
{
	class Reflectable;

	/**
	 * \brief The base class for Dire to store information of properties.
	 * Never use it directly: it is to be inherited by templated TypedProperty in the DIRE_PROPERTY macro.
	 */
	class PropertyTypeInfo : public IntrusiveListNode<PropertyTypeInfo>
	{
		using CopyConstructorPtr = void (*)(void* pDestAddr, const void * pOther, size_t pOffset);

	public:

		PropertyTypeInfo(const char * pName, const std::ptrdiff_t pOffset, const size_t pSize, const CopyConstructorPtr pCopyCtor = nullptr) :
			myName(pName), myMetatype(MetaType::Unknown), myOffset(pOffset), mySize(pSize), myCopyCtor(pCopyCtor)
		{}

		[[nodiscard]] const IArrayDataStructureHandler* GetArrayHandler() const
		{
			return myDataStructurePropertyHandler.GetArrayHandler();
		}

		[[nodiscard]] const IMapDataStructureHandler* GetMapHandler() const
		{
			return myDataStructurePropertyHandler.GetMapHandler();
		}

		[[nodiscard]] const DataStructureHandler& GetDataStructureHandler() const
		{
			return myDataStructurePropertyHandler;
		}

		[[nodiscard]] const DIRE_STRING_VIEW&	GetName() const { return myName; }

		[[nodiscard]] const MetaType&			GetMetatype() const { return myMetatype; }

		[[nodiscard]] size_t					GetOffset() const { return size_t(myOffset); }

		[[nodiscard]] size_t					GetSize() const { return mySize; }

		[[nodiscard]] ReflectableID				GetReflectableID() const { return myReflectableID; }

		[[nodiscard]] CopyConstructorPtr		GetCopyConstructorFunction() const { return myCopyCtor; }

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
		void	PopulateDataStructureHandler();

		template <typename TProp>
		void	SetReflectableID();

		template <typename TProp>
		void	InitializeReflectionCopyConstructor();

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

	/**
	 * \brief The base interface for storing class member function info.
	 * It provides the Invoke method that allows to dynamically call the bound member function along with any number of parameters.
	 */
	class IFunctionTypeInfo
	{
	public:
		virtual ~IFunctionTypeInfo() = default;

		virtual std::any	Invoke(void* pObject, const std::any & pInvokeParams) const = 0;
	};

	/**
	 * \brief An abstract type that stores data and defines functions common to all types of function infos.
	 * Never use directly: use DIRE_FUNCTION or DIRE_FUNCTION_TYPEINFO() macros for that.
	 * These macros actually use a template child class, TypedFunctionInfo.
	 */
	class FunctionInfo : public IntrusiveListNode<FunctionInfo>, IFunctionTypeInfo
	{
	public:
		using ParameterList = std::vector<MetaType, DIRE_ALLOCATOR<MetaType>>;

		FunctionInfo(const char* pName, MetaType pReturnType) :
			Name(pName), ReturnType(pReturnType)
		{}

		/**
		 * \brief Allows to invoke the member function while directly providing the arguments of the function (it packs them into std::any for you)
		 * \tparam Args The arguments variadic pack
		 * \param pCallerObject The object the member function should use as this
		 * \param pFuncArgs The arguments
		 * \return The type-erased return of the function, or empty any if void
		 */
		template <typename... Args>
		std::any	InvokeWithArgs(void* pCallerObject, Args&&... pFuncArgs) const;

		/**
		 * \brief Allows to invoke the member function while directly providing the arguments of the function (it packs them into std::any for you).
		 *	By providing the return type, it also allows the function to directly return the return value without handing out a std::any.
		 * \tparam Args The arguments variadic pack
		 * \param pCallerObject The object the member function should use as this
		 * \param pFuncArgs The arguments
		 * \return The return value of the function
		 */
		template <typename Ret, typename... Args>
		Ret	TypedInvokeWithArgs(void* pCallerObject, Args&&... pFuncArgs) const;

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

	/**
	 * \brief The class declared by the DIRE_FUNCTION and DIRE_TYPED_FUNCTION macros. Never use it directly, use the macros instead.
	 * \tparam Class The owning class of the function
	 * \tparam Ret The return type
	 * \tparam Args Types of parameters of the function
	 */
	template <typename Class, typename Ret, typename ... Args >
	struct TypedFunctionInfo<Ret(Class::*)(Args...)> final : FunctionInfo
	{
		using MemberFunctionSignature = Ret(Class::*)(Args...);

		TypedFunctionInfo(MemberFunctionSignature memberFunc, const char* methodName);

		// The idea of using std::apply has been stolen from here : https://stackoverflow.com/a/36656413/1987466
		std::any	Invoke(void* pObject, std::any const& pInvokeParams) const override;

		MemberFunctionSignature	myMemberFunctionPtr = nullptr;
	};

	template <typename Ret = void, typename T, typename... Args>
	Ret	Invoke(const DIRE_STRING_VIEW& pFuncName, T& pObject, Args&&... pArgs);



	class TypeInfo
	{
	public:
		using TypeInfoList = std::vector<TypeInfo*, DIRE_ALLOCATOR<TypeInfo*>>;

		explicit TypeInfo(const char* pTypename) :
			myReflectableID(TypeInfoDatabase::EditSingleton().RegisterTypeInfo(this)),
			myTypeName(pTypename)
		{}

		void	PushTypeInfo(PropertyTypeInfo& pNewTypeInfo)
		{
			myProperties.PushBackNewNode(pNewTypeInfo);
		}

		void	PushFunctionInfo(FunctionInfo& pNewFunctionInfo)
		{
			myMemberFunctions.PushBackNewNode(pNewFunctionInfo);
		}

		void	AddParentClass(TypeInfo* pParent)
		{
			myParentClasses.push_back(pParent);
		}

		void	AddChildClass(TypeInfo* pChild)
		{
			myChildrenClasses.push_back(pChild);
		}

		[[nodiscard]] Dire_EXPORT bool					IsParentOf(const ReflectableID pChildClassID, bool pIncludingMyself = true) const;

		using ParentPropertyInfo = std::pair< const TypeInfo*, const PropertyTypeInfo*>;
		[[nodiscard]] ParentPropertyInfo				FindParentClassProperty(const DIRE_STRING_VIEW& pName) const;

		[[nodiscard]] const PropertyTypeInfo *			FindProperty(const DIRE_STRING_VIEW& pName) const;

		[[nodiscard]] const PropertyTypeInfo *			FindPropertyInHierarchy(const DIRE_STRING_VIEW& pName) const;

		[[nodiscard]] Dire_EXPORT const FunctionInfo *	FindFunction(const DIRE_STRING_VIEW& pFuncName) const;

		using ParentFunctionInfo = std::pair<const TypeInfo*, const FunctionInfo*>;
		[[nodiscard]] ParentFunctionInfo				FindParentFunction(const DIRE_STRING_VIEW& pFuncName) const;

		template <typename F>
		void	ForEachPropertyInHierarchy(F&& pVisitorFunction) const;

		[[nodiscard]] const DIRE_STRING_VIEW& GetName() const
		{
			return myTypeName;
		}

		void	SetID(const ReflectableID pNewID)
		{
			myReflectableID = pNewID;
		}

		[[nodiscard]] ReflectableID	GetID() const
		{
			return myReflectableID;
		}

		Dire_EXPORT void	CloneHierarchyPropertiesOf(Reflectable& pNewClone, const Reflectable& pCloned) const;

		void	ClonePropertiesOf(Reflectable& pNewClone, const Reflectable& pCloned) const;

		[[nodiscard]] const IntrusiveLinkedList<PropertyTypeInfo>& GetPropertyList() const
		{
			return myProperties;
		}

		[[nodiscard]] const IntrusiveLinkedList<FunctionInfo>& GetFunctionList() const
		{
			return myMemberFunctions;
		}

		[[nodiscard]] const TypeInfoList& GetParentClasses() const
		{
			return myParentClasses;
		}

		[[nodiscard]] const TypeInfoList& GetChildrenClasses() const
		{
			return myChildrenClasses;
		}

	protected:
		ReflectableID							myReflectableID = INVALID_REFLECTABLE_ID;
		DIRE_STRING_VIEW						myTypeName;
		IntrusiveLinkedList<PropertyTypeInfo>	myProperties;
		IntrusiveLinkedList<FunctionInfo>		myMemberFunctions;
		TypeInfoList							myParentClasses;
		TypeInfoList							myChildrenClasses;
	};

	/**
	 * \brief The metadata type declared by DIRE_TYPE_INFO and DIRE_REFLECTABLE_INFO. Never use directly, always use one these two macros.
	 * The boolean parameter lets you decide if this class should use its default constructor as an instantiator function. DIRE's default is true.
	 * \tparam T The type
	 * \tparam UseDefaultCtorForInstantiate True if this class should register its default constructor as instantiator (true by default)
	 */
	template <typename T, bool UseDefaultCtorForInstantiate>
	class TypedTypeInfo final : public TypeInfo
	{
	public:
		explicit TypedTypeInfo(char const* pTypename);

	private:
		template <typename TParent>
		void	RecursiveRegisterParentClasses();
	};

}

#include "DireTypeInfo.inl"

#define DIRE_LPAREN (
#define DIRE_RPAREN )

// inspired by https://www.mikeash.com/pyblog/friday-qa-2015-03-20-preprocessor-abuse-and-optional-parentheses.html
#define DIRE_EXTRACT(...) DIRE_EXTRACT __VA_ARGS__
#define DIRE_NOTHING_DIRE_EXTRACT
#define DIRE_PASTE(x, ...) x ## __VA_ARGS__
#define DIRE_EVALUATING_PASTE(x, ...) DIRE_PASTE(x, __VA_ARGS__)

#define DIRE_UNPAREN(...) DIRE_EVALUATING_PASTE(DIRE_NOTHING_, DIRE_EXTRACT __VA_ARGS__)

#define DIRE_FUNCTION_TYPEINFO(Name) \
	inline static DIRE_NS::TypedFunctionInfo<decltype(&Self::Name)> Name##_TYPEINFO{ &Self::Name, DIRE_STRINGIZE(Name)};

#define DIRE_FUNCTION(RetType, Name, ...) \
	RetType Name DIRE_LPAREN DIRE_UNPAREN(__VA_ARGS__) DIRE_RPAREN ; \
	DIRE_FUNCTION_TYPEINFO(Name);


#define DIRE_DECLARE_TYPEINFO() \
	struct DIRE_SelfTypeTag{}; \
    constexpr auto DIRE_SelfTypeHelper() -> decltype(::DIRE_NS::SelfType::Writer<DIRE_SelfTypeTag, decltype(this)>{}, void()) {} \
    using Self = ::DIRE_NS::SelfType::Read<DIRE_SelfTypeTag>;\
	inline static DIRE_NS::TypedTypeInfo<Self, false>	DIRE_TypeInfo{""};\
	static DIRE_NS::TypeInfo const&	GetTypeInfo()\
	{\
		return DIRE_TypeInfo;\
	}\
	static DIRE_NS::TypeInfo&	EditTypeInfo()\
	{\
		return DIRE_TypeInfo;\
	}
