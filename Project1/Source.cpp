
#include <any>
#include <vector>
#include <string>
#include <string_view>

class ReflectableTypeInfo;

enum class Type
{
	Int,
	Float,
	Bool,
	Class
};

template <Type T>
struct FromEnumTypeToActualType;

template <typename T>
struct FromActualTypeToEnumType;

#define DECLARE_TYPE_TRANSLATOR(TheEnumType, TheActualType) \
	template <> \
	struct FromEnumTypeToActualType<TheEnumType> \
	{ \
		using ActualType = TheActualType; \
	}; \
	template <> \
	struct FromActualTypeToEnumType<TheActualType> \
	{ \
		static const Type EnumType = TheEnumType; \
	};

DECLARE_TYPE_TRANSLATOR(Type::Int, int)
DECLARE_TYPE_TRANSLATOR(Type::Float, float)
DECLARE_TYPE_TRANSLATOR(Type::Bool, bool)

template <typename T>
class IntrusiveListNode
{
public:
	IntrusiveListNode() = default;

	IntrusiveListNode(IntrusiveListNode& pNext)
	{
		Next = pNext;
	}

	operator T* ()
	{
		return reinterpret_cast<T*>(this);
	}

	operator T const* ()
	{
		return reinterpret_cast<T const*>(this);
	}

	IntrusiveListNode* Next = nullptr;
};

template <typename T>
class IntrusiveLinkedList
{
public:

	class iterator
	{
	public:
		iterator(IntrusiveListNode<T>* pItNode = nullptr) :
			myNode(pItNode)
		{}

		iterator& operator ++()
		{
			myNode = myNode->Next;
			return *this;
		}

		iterator operator ++(int)
		{
			iterator it(myNode);
			this->operator++();
			return it;
		}

		operator bool() const
		{
			return (myNode != nullptr);
		}

		bool operator!=(iterator& other) const
		{
			return myNode != other.myNode;
		}

		T& operator*() const
		{
			return *((T*)myNode);
		}

	private:

		IntrusiveListNode<T>* myNode;
	};

	class const_iterator
	{
	public:
		const_iterator(IntrusiveListNode<T> const* pItNode = nullptr) :
			myNode(pItNode)
		{}

		const_iterator& operator ++()
		{
			myNode = myNode->Next;
			return *this;
		}

		const_iterator operator ++(int)
		{
			const_iterator it(myNode);
			this->operator++();
			return it;
		}

		operator bool() const
		{
			return (myNode != nullptr);
		}

		bool operator!=(const_iterator& other) const
		{
			return myNode != other.myNode;
		}

		T const& operator*() const
		{
			return *((T const*)myNode);
		}

	private:

		IntrusiveListNode<T> const* myNode;
	};

	iterator begin()
	{
		return iterator(Head);
	}

	iterator end()
	{
		return nullptr;
	}

	const_iterator begin() const
	{
		return const_iterator(Head);
	}

	const_iterator end() const
	{
		return nullptr;
	}

	void	PushFrontNewNode(IntrusiveListNode<T>& pNewNode)
	{
		pNewNode.Next = Head;
		Head = &pNewNode;
	}

private:

	IntrusiveListNode<T>* Head = nullptr;
};

struct TypeInfo2 : IntrusiveListNode<TypeInfo2>
{
	TypeInfo2(const char* pName, Type pType, std::ptrdiff_t pOffset) :
		name(pName), type(pType), offset(pOffset)
	{}

	std::string		name;
	Type			type;
	std::ptrdiff_t	offset;
};

struct IFunctionTypeInfo
{
public:
	virtual ~IFunctionTypeInfo() = default;

	virtual std::any	Invoke(void* pObject, std::any const& pInvokeParams) const = 0;
};


// Inspired by https://stackoverflow.com/a/59604594/1987466
template<typename method_t>
struct is_const_method;

template<typename CClass, typename ReturnType, typename ...ArgType>
struct is_const_method< ReturnType(CClass::*)(ArgType...)> {
	static constexpr bool value = false;
	using ClassType = CClass;

};

template<typename CClass, typename ReturnType, typename ...ArgType>
struct is_const_method< ReturnType(CClass::*)(ArgType...) const> {
	static constexpr bool value = true;
	using ClassType = std::add_const_t<CClass>;
};

template<typename method_t>
struct SelfTypeDetector;

template<typename CClass, typename ReturnType, typename ...ArgType>
struct SelfTypeDetector< ReturnType(CClass::*)(ArgType...)>
{
	using Self = CClass;
};

// Note: this macro does not support templates.
#define DECLARE_ATTORNEY(MemberFunc) \
	using ClientType = SelfTypeDetector<decltype(&MemberFunc)>::Self;\
	struct CONCAT(MemberFunc, _Attorney)\
	{\
		template <typename... Args>\
		auto MemberFunc(is_const_method<decltype(&ClientType::MemberFunc)>::ClassType& pClient, Args&&... pArgs)\
		{\
			return pClient.MemberFunc(std::forward<Args>(pArgs)...);\
		}\
	};\
	friend CONCAT(MemberFunc, _Attorney);



struct FunctionInfo : IntrusiveListNode<FunctionInfo>, IFunctionTypeInfo
{

	FunctionInfo(const char* pName, Type pReturnType) :
		name(pName), returnType(pReturnType)
	{}

	// Not using perfect forwarding here because std::any cannot hold references: https://stackoverflow.com/questions/70239155/how-to-cast-stdtuple-to-stdany
	// Sad but true, gotta move forward with our lives.
	template <typename... Args>
	std::any	InvokeFunc(void* pCallerObject, Args... pFuncArgs) const
	{
		using ArgumentPackTuple = std::tuple<Args...>;
		std::any packedArgs;
		packedArgs.emplace<ArgumentPackTuple>(std::forward_as_tuple(pFuncArgs...));
		std::any result = Invoke(pCallerObject, packedArgs);
		return result;
	}
	template <typename Ret, typename... Args>
	Ret	TInvokeFunc(void* pCallerObject, Args... pFuncArgs) const
	{
		std::any result = InvokeFunc(pCallerObject, pFuncArgs...);
		return std::any_cast<Ret>(result);
	}


public:

	std::string	name;
	Type	returnType{ Type::Int };
	std::vector<Type> parameters;

protected:

	void	PushBackParameterType(Type pNewParamType)
	{
		parameters.push_back(pNewParamType);
	}


};



template <class T>
class Singleton
{
public:

	static T& EditSingleton()
	{
		static T theSingleton; // Thread-safe since C++11!
		return theSingleton;
	}

	static T const& GetSingleton()
	{
		return EditSingleton();
	}

	template <typename... Args>
	T& ReinitializeSingleton(Args&&... pCtorParams)
	{
		EditSingleton() = T(std::forward<Args>(pCtorParams)...);
		return EditSingleton();
	}

protected:
	Singleton() = default;
	~Singleton() = default;

private:

	Singleton(Singleton const&) = delete;   // Copy construct
	Singleton(Singleton&&) = delete;   // Move construct
	Singleton& operator=(Singleton const&) = delete;   // Copy assign
	Singleton& operator=(Singleton&&) = delete;   // Move assign

};


template <typename T, typename IDType = int>
class AutomaticTypeCounter
{
public:
	AutomaticTypeCounter() = default;

	static IDType	TakeNextID()
	{
		return theCounter++;
	}

	static IDType	PeekNextID()
	{
		return theCounter;
	}

private:
	inline static IDType theCounter = 0;
};

// TODO: replace all the unsigned by a unified typedef
struct Reflector3 : Singleton<Reflector3>, AutomaticTypeCounter<Reflector3, unsigned>
{
public:

	Reflector3() = default;

	[[nodiscard]] size_t	GetTypeInfoCount() const
	{
		return myReflectableTypeInfos.size();
	}

	ReflectableTypeInfo const* GetTypeInfo(unsigned classID) const
	{
		if (classID < myReflectableTypeInfos.size())
		{
			return myReflectableTypeInfos[classID];
		}

		return nullptr;
	}


private:

	friend ReflectableTypeInfo;

	[[nodiscard]] unsigned	RegisterTypeInfo(ReflectableTypeInfo* pTypeInfo)
	{
		myReflectableTypeInfos.push_back(pTypeInfo);
		return GetTypeInfoCount() - 1;
	}

	std::vector<ReflectableTypeInfo*>	myReflectableTypeInfos;
};

class ReflectableTypeInfo
{
public:

	ReflectableTypeInfo() = default; // TODO: remove

	ReflectableTypeInfo(char const* pTypename) :
		Typename(pTypename)
	{
		ReflectableID = Reflector3::EditSingleton().RegisterTypeInfo(this);
	}

	void	PushTypeInfo(TypeInfo2& pNewTypeInfo)
	{
		Properties.PushFrontNewNode(pNewTypeInfo);
	}

	void	PushFrontFunctionInfo(FunctionInfo& pNewFunctionInfo)
	{
		MemberFunctions.PushFrontNewNode(pNewFunctionInfo);
	}

	unsigned							ReflectableID;
	std::string_view					Typename;
	IntrusiveLinkedList<TypeInfo2>		Properties;
	IntrusiveLinkedList<FunctionInfo>	MemberFunctions;

};



template <typename Ty >
struct FunctionTypeInfo; // not defined

template <typename Class, typename Ret, typename ... Args >
struct FunctionTypeInfo<Ret(Class::*)(Args...)> : FunctionInfo
{
	using MemberFunctionSignature = Ret(Class::*)(Args...);

	template <typename Last>
	void	AddParameters()
	{
		PushBackParameterType(FromActualTypeToEnumType<Last>::EnumType);
	}

	template <typename First, typename Second, typename ...Rest>
	void	AddParameters()
	{
		PushBackParameterType(FromActualTypeToEnumType<First>::EnumType);
		AddParameters<Second, Rest...>();
	}


	FunctionTypeInfo(MemberFunctionSignature memberFunc, const char* methodName) :
		FunctionInfo(methodName, FromActualTypeToEnumType<Ret>::EnumType),
		myMemberFunctionPtr(memberFunc)
	{
		if constexpr (sizeof...(Args) > 0)
		{
			AddParameters<Args...>();
		}

		ReflectableTypeInfo& theClassReflector = Class::GetReflector2();
		theClassReflector.PushFrontFunctionInfo(*this);
		//memberFunc;
	}

	// The idea of using std::apply has been stolen from here : https://stackoverflow.com/a/36656413/1987466
	std::any	Invoke(void* pObject, std::any const& pInvokeParams) const override
	{
		using ArgumentsTuple = std::tuple<Args...>;
		ArgumentsTuple const* argPack = std::any_cast<ArgumentsTuple>(&pInvokeParams);

		auto f = [this, pObject](Args... pMemFunArgs)
		{
			Class* theActualObject = static_cast<Class*>(pObject);
			return (theActualObject->*myMemberFunctionPtr)(pMemFunArgs...);
		};
		std::any result = std::apply(f, *argPack);
		return result;
	}

	MemberFunctionSignature	myMemberFunctionPtr = nullptr;
};



/* Stolen from https://stackoverflow.com/a/26685339/1987466 */
/*
 * Count the number of arguments passed to ASSERT, very carefully
 * tiptoeing around an MSVC bug where it improperly expands __VA_ARGS__ as a
 * single token in argument lists.  See these URLs for details:
 *
 *   http://connect.microsoft.com/VisualStudio/feedback/details/380090/variadic-macro-replacement
 *   http://cplusplus.co.il/2010/07/17/variadic-macro-to-count-number-of-arguments/#comment-644
 */
#ifdef _MSC_VER // Microsoft compilers

#define EXPAND(x) x
#define __NARGS(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, VAL, ...) VAL
#define NARGS_1(...) EXPAND(__NARGS(__VA_ARGS__, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0))

#define AUGMENTER(...) unused, __VA_ARGS__
#define NARGS(...) NARGS_1(AUGMENTER(__VA_ARGS__))

#else // Others

#define NARGS(...) __NARGS(0, ## __VA_ARGS__, 14, 13, 12, 11, 10, 9, 8, 7, 6,5,4,3,2,1,0)
#define __NARGS(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 15, N,...) N

#endif

#define GLUE(a, b) a b
#define GLUE3(a,b,c)
#define COMMA ,

#define GLUE_ARGS0()
#define GLUE_ARGS1(a1) a1
#define GLUE_ARGS2(a1, a2) a1 a2
#define GLUE_ARGS3(a1, a2, a3) a1 a2 COMMA a3
#define GLUE_ARGS4(a1,a2,a3,a4) a1 a2 COMMA a3 a4
#define GLUE_ARGS5(a1,a2,a3,a4,a5) a1 a2 COMMA a3 a4 COMMA a5
#define GLUE_ARGS6(a1,a2,a3,a4,a5,a6) a1 a2 COMMA a3 a4  COMMA a5 a6
#define GLUE_ARGS7(a1,a2,a3,a4,a5,a6,a7) a1 a2 COMMA a3 a4  COMMA a5 a6 COMMA a7
#define GLUE_ARGS8(a1,a2,a3,a4,a5,a6,a7,a8) a1 a2 COMMA a3 a4  COMMA a5 a6 COMMA a8
#define GLUE_ARGUMENT_PAIRS(...) EXPAND(CONCAT2(GLUE_ARGS, NARGS(__VA_ARGS__)))(EXPAND(__VA_ARGS__)) // TODO : on dirait que EXPAND(__VA_ARGS__) c'est pete avec plus d'1 argument. verifier si il y a des warning msvc

#define COMMA_ARGS_MINUS_COUNTER1(a1) a1 = __COUNTER__ - OFFSET
#define COMMA_ARGS_MINUS_COUNTER2(a1, a2, a3, a4) a1 = __COUNTER__ - OFFSET COMMA a2 = __COUNTER__ - OFFSET
#define COMMA_ARGS_MINUS_COUNTER3(a1, a2, a3) a1= __COUNTER__ - OFFSET  COMMA a2= __COUNTER__ - OFFSET  COMMA a3= __COUNTER__ - OFFSET
#define COMMA_ARGS_MINUS_COUNTER4(a1, a2, a3, a4) a1= __COUNTER__ - OFFSET  COMMA a2= __COUNTER__ - OFFSET  COMMA a3= __COUNTER__ - OFFSET  COMMA a4= __COUNTER__ - OFFSET 
#define COMMA_ARGS_MINUS_COUNTER5(a1, a2, a3, a4, a5) a1= __COUNTER__ - OFFSET COMMA a2= __COUNTER__ - OFFSET COMMA a3= __COUNTER__ - OFFSET COMMA a4= __COUNTER__ - OFFSET  COMMA a5= __COUNTER__ - OFFSET

#define COMMA_ARGS_LSHIFT_COUNTER1(a1) a1 = 1 << (__COUNTER__ - COUNTER_BASE-OFFSET)
#define COMMA_ARGS_LSHIFT_COUNTER2(a1, a2, a3, a4) a1 = 1 << (__COUNTER__ - COUNTER_BASE-OFFSET) COMMA a2 = 1 << (__COUNTER__ - COUNTER_BASE-OFFSET)
#define COMMA_ARGS_LSHIFT_COUNTER3(a1, a2, a3) a1= 1 << (__COUNTER__ - COUNTER_BASE-OFFSET)  COMMA a2= 1 << (__COUNTER__ - COUNTER_BASE-OFFSET)  COMMA a3= 1 << (__COUNTER__ - COUNTER_BASE-OFFSET) 
#define COMMA_ARGS_LSHIFT_COUNTER4(a1, a2, a3, a4) a1= 1 << (__COUNTER__ - COUNTER_BASE-OFFSET)  COMMA a2= 1 << (__COUNTER__ - COUNTER_BASE-OFFSET)  COMMA a3= 1 << (__COUNTER__ - COUNTER_BASE-OFFSET)  COMMA a4= 1 << (__COUNTER__ - COUNTER_BASE-OFFSET) 
#define COMMA_ARGS_LSHIFT_COUNTER5(a1, a2, a3, a4, a5) a1= 1 << (__COUNTER__ - COUNTER_BASE-OFFSET) COMMA a2= 1 << (__COUNTER__ - COUNTER_BASE-OFFSET) COMMA a3= 1 << (__COUNTER__ - COUNTER_BASE-OFFSET) COMMA a4= 1 << (__COUNTER__ - COUNTER_BASE-OFFSET)  COMMA a5= 1 << (__COUNTER__ - COUNTER_BASE-OFFSET)

#define COMMA_ARGS1(a1) a1
#define COMMA_ARGS2(a1, a2, a3, a4) a1 COMMA a2
#define COMMA_ARGS3(a1, a2, a3) a1 COMMA a2 COMMA a3
#define COMMA_ARGS4(a1, a2, a3, a4) a1 COMMA a2 COMMA a3 COMMA a4
#define COMMA_ARGS5(a1, a2, a3, a4, a5) a1 COMMA a2 COMMA a3 COMMA a5

#define BRACES_STRINGIZE_COMMA_VALUE(a) { STRINGIZE(a) COMMA a  } COMMA

#define BRACES_STRINGIZE_COMMA_VALUE1(a1) BRACES_STRINGIZE_COMMA_VALUE(a1)
#define BRACES_STRINGIZE_COMMA_VALUE2(a1, a2) BRACES_STRINGIZE_COMMA_VALUE(a1) BRACES_STRINGIZE_COMMA_VALUE(a2)
#define BRACES_STRINGIZE_COMMA_VALUE3(a1, a2, a3) BRACES_STRINGIZE_COMMA_VALUE(a1) BRACES_STRINGIZE_COMMA_VALUE(a2) BRACES_STRINGIZE_COMMA_VALUE(a3)
#define BRACES_STRINGIZE_COMMA_VALUE4(a1, a2, a3, a4) BRACES_STRINGIZE_COMMA_VALUE(a1) BRACES_STRINGIZE_COMMA_VALUE(a2) BRACES_STRINGIZE_COMMA_VALUE(a3)  BRACES_STRINGIZE_COMMA_VALUE(a4)
#define BRACES_STRINGIZE_COMMA_VALUE5(a1, a2, a3, a4, a5) BRACES_STRINGIZE_COMMA_VALUE(a1) BRACES_STRINGIZE_COMMA_VALUE(a2) BRACES_STRINGIZE_COMMA_VALUE(a3)  BRACES_STRINGIZE_COMMA_VALUE(a4)  BRACES_STRINGIZE_COMMA_VALUE(a5)


#define EXTRACT_ODD_ARGS0()
#define EXTRACT_ODD_ARGS1(a1) a1
#define EXTRACT_ODD_ARGS2(a1, a2) a1
#define EXTRACT_ODD_ARGS3(a1, a2, a3) a1 COMMA a3
#define EXTRACT_ODD_ARGS4(a1,a2,a3,a4) a1 COMMA a3
#define EXTRACT_ODD_ARGS5(a1,a2,a3,a4,a5) a1 COMMA a3 COMMA a5
#define EXTRACT_ODD_ARGS6(a1,a2,a3,a4,a5,a6) a1 COMMA a3 COMMA a5
#define EXTRACT_ODD_ARGS7(a1,a2,a3,a4,a5,a6,a7) a1 COMMA a3 COMMA a5 COMMA a7
#define EXTRACT_ODD_ARGS8(a1,a2,a3,a4,a5,a6,a7,a8) a1 COMMA a3 COMMA a5 COMMA a7

#define EVAL_1(...) __VA_ARGS__
#define CONCAT(a, b) a ## b
#define CONCAT2(a, b) CONCAT(a, b)
#define EXTRACT_ODD_ARGUMENTS(...) EXPAND(CONCAT2(EXTRACT_ODD_ARGS, NARGS(__VA_ARGS__))(__VA_ARGS__)) // attention : ajouter le ##__VA_ARGS__ pour GCC sort un warning sur msvc, il faudra p-e le ifdef...


#define STRINGIZE(a) #a

 // Stolen from https://stackoverflow.com/questions/48710758/how-to-fix-variadic-macro-related-issues-with-macro-overloading-in-msvc-mic
 // It's the approach that works the best for our use case so far.
#define MSVC_BUG(MACRO, ARGS) MACRO ARGS  // name to remind that bug fix is due to MSVC :-)

#define VA_MACRO(MACRO, ...) MSVC_BUG(CONCAT2, (MACRO, NARGS(__VA_ARGS__)))(__VA_ARGS__)

// This idea of storing a "COUNTER_BASE" has been stolen from https://stackoverflow.com/a/52770279/1987466
#define LINEAR_ENUM(EnumName, UnderlyingType, ...) \
	struct EnumName\
	{\
		private:\
			using Underlying = UnderlyingType; \
			enum LocalCounter_t { COUNTER_BASE = __COUNTER__, OFFSET = COUNTER_BASE+1 };\
		public:\
			enum Type : UnderlyingType\
			{\
				VA_MACRO(COMMA_ARGS_MINUS_COUNTER, __VA_ARGS__)\
			};\
			Type Value{};\
			EnumName() = default;\
			EnumName(Type pInitVal) :\
				Value(pInitVal)\
			{}\
			EnumName& operator=(EnumName const& pOther)\
			{\
				Value = pOther.Value;\
				return *this;\
			}\
			EnumName& operator=(Type const pVal)\
			{\
				Value = pVal;\
				return *this;\
			}\
			bool operator!=(EnumName const& pOther)\
			{\
				return (Value != pOther.Value);\
			}\
			bool operator!=(Type const pVal)\
			{\
				return (Value == pVal);\
			}\
			bool operator==(EnumName const& pOther)\
			{\
				return !(*this != pOther);\
			}\
			bool operator==(Type const pVal)\
			{\
				return (Value == pVal);\
			}\
			operator Type() const\
			{\
				return Value;\
			}\
			private:\
			inline static std::pair<const char*, Type> nameEnumPairs[] {\
				VA_MACRO(BRACES_STRINGIZE_COMMA_VALUE, __VA_ARGS__)\
			};\
		public:\
			/* This one assumes that the parameter value is guaranteed to be within bounds and thus doesn't even bounds check. */\
			static char const* FromSafeEnum(Type enumValue) \
			{ \
				return nameEnumPairs[(int)enumValue].first; \
			}\
			/* We have the guarantee that linear enums values start from 0 and increase 1 by 1 so we can make this function O(1).*/ \
			static char const* FromEnum(Type enumValue) \
			{ \
				if( (int)enumValue >= NARGS(__VA_ARGS__))\
				{\
					return nullptr;\
				}\
				return FromSafeEnum(enumValue);\
			}\
			static Type const* FromString(char const* enumStr) \
			{ \
				for (auto const& pair : nameEnumPairs)\
				{\
					if (strcmp(pair.first, enumStr) == 0)\
						return &pair.second;\
				}\
				return nullptr;\
			}\
			static Type FromSafeString(char const* enumStr) \
			{ \
				for (auto const& pair : nameEnumPairs)\
				{\
					if (strcmp(pair.first, enumStr) == 0)\
						return pair.second;\
				}\
				return Type(); /* should never happen!*/ \
			}\
	};


#ifdef _MSC_VER // Microsoft compilers

#pragma intrinsic(_BitScanForward)

// https://learn.microsoft.com/en-us/cpp/intrinsics/bitscanforward-bitscanforward64?view=msvc-170
template <typename T>
constexpr std::enable_if_t<std::is_integral_v<T>, unsigned long>
FindFirstSetBit(T value)
{
	unsigned long firstSetBitIndex = 0;
	if constexpr (sizeof(T) > sizeof(unsigned long))
	{
		_BitScanForward64(&firstSetBitIndex, (unsigned __int64)value);
	}
	else
	{
		_BitScanForward(&firstSetBitIndex, (unsigned long)value);
	}

	return firstSetBitIndex;
}
#elif defined(__GNUG__) || defined(__clang__) // https://stackoverflow.com/questions/28166565/detect-gcc-as-opposed-to-msvc-clang-with-macro
// https://gcc.gnu.org/onlinedocs/gcc/Other-Builtins.html
// https://clang.llvm.org/docs/LanguageExtensions.html
// TODO: untested
template <typename T>
constexpr std::enable_if_t<std::is_integral_v<T>, unsigned long>
FindFirstSetBit(T value)
{
	unsigned long firstSetBitIndex = 0;
	if constexpr (sizeof(T) <= sizeof(int))
	{
		firstSetBitIndex = (unsigned long)__builtin_ffs((int)value);
	}
	else if constexpr (sizeof(T) <= sizeof(long)) // probably useless nowadays
	{
		firstSetBitIndex = (unsigned long)__builtin_ffsl((long)value);
	}
	else
	{
		firstSetBitIndex = (unsigned long)__builtin_ffsll((long long)value);
	}

	return firstSetBitIndex;
}
#endif

#define BITFLAGS_ENUM(EnumName, UnderlyingType, ...) \
	struct EnumName { \
			private:\
			using Underlying = UnderlyingType; \
			enum LocalCounter_t { COUNTER_BASE = __COUNTER__, OFFSET = (COUNTER_BASE == 0 ? 0 : 1) };\
		public:\
			enum Type : UnderlyingType\
			{\
				VA_MACRO(COMMA_ARGS_LSHIFT_COUNTER, __VA_ARGS__)\
			};\
			EnumName() = default;\
			EnumName(Type pInitVal) :\
				Value(pInitVal)\
			{}\
			EnumName& operator=(EnumName const& pOther)\
			{\
				Value = pOther.Value;\
				return *this;\
			}\
			EnumName& operator=(Type const pVal)\
			{\
				Value = pVal;\
				return *this;\
			}\
			bool operator!=(EnumName const& pOther)\
			{\
				return (Value != pOther.Value);\
			}\
			bool operator!=(Type const pVal)\
			{\
				return (Value == pVal);\
			}\
			bool operator==(EnumName const& pOther)\
			{\
				return !(*this != pOther);\
			}\
			bool operator==(Type const pVal)\
			{\
				return (Value == pVal);\
			}\
			operator Type() const\
			{\
				return Value;\
			}\
			Type	Value{};\
		private:\
			inline static std::pair<const char*, Type> nameEnumPairs[] {\
				VA_MACRO(BRACES_STRINGIZE_COMMA_VALUE, __VA_ARGS__)\
			};\
		public:\
			void Set(Type pSetBit)\
			{\
				Value = (Type)((Underlying)Value | (Underlying)pSetBit);\
			}\
			void Clear(Type pClearedBit)\
			{\
				Value = (Type)((Underlying)Value & ~((Underlying)pClearedBit));\
			}\
			bool IsSet(Type pBit) const\
			{\
				return ((Underlying)Value & (Underlying)pBit) == 0;\
			}\
			/*This one assumes that the parameter value is guaranteed to be within bounds and thus doesn't even bounds check. */\
			static char const* FromSafeEnum(Type pEnumValue) \
			{ \
				return nameEnumPairs[FindFirstSetBit((UnderlyingType)pEnumValue)].first; \
			}\
			/* We have the guarantee that bitflags enums values are powers of 2 so we can make this function O(1).*/ \
			static char const* FromEnum(Type pEnumValue)\
			{ \
				/* If value is 0 OR not a power of 2 OR bigger than our biggest power of 2, it cannot be valid */\
				const bool isPowerOf2 = ((UnderlyingType)pEnumValue & ((UnderlyingType)pEnumValue - 1)) == 0;\
				if ((UnderlyingType)pEnumValue == 0 || !isPowerOf2 || (UnderlyingType)pEnumValue >= (1 << NARGS(__VA_ARGS__)))\
				{\
					return nullptr;\
				}\
				return FromSafeEnum(pEnumValue);\
			}\
			static Type const* FromString(char const* enumStr) \
			{ \
				for (auto const& pair : nameEnumPairs)\
				{\
					if (strcmp(pair.first, enumStr) == 0)\
						return &pair.second;\
				}\
				return nullptr;\
			}\
			static Type FromSafeString(char const* enumStr) \
			{ \
				for (auto const& pair : nameEnumPairs)\
				{\
					if (strcmp(pair.first, enumStr) == 0)\
						return pair.second;\
				}\
				return Type(); /* should never happen!*/ \
			}\
	};


#define TYPED_ENUM(EnumName, UnderlyingType, ...) \
	struct EnumName { \
		using Underlying = UnderlyingType; \
		typedef enum Type : UnderlyingType { \
			VA_MACRO(COMMA_ARGS, __VA_ARGS__)\
		} Type; \
		inline static std::pair<const char*, Type> nameEnumPairs[] {\
			VA_MACRO(BRACES_STRINGIZE_COMMA_VALUE, __VA_ARGS__)\
		};\
	};

#define ENUM(EnumName, ...) TYPED_ENUM(EnumName, int, __VA_ARGS__)

LINEAR_ENUM(LinearEnum33, int, un, deux);
LINEAR_ENUM(LinearEnum, int, un, deux);


BITFLAGS_ENUM(BitEnum, int, un, deux, quatre, seize);

TYPED_ENUM(TestEnum, int, un, deux);


template <typename Class, typename Ret, typename... Args >
struct FunctionTypeInfo2
{
	FunctionTypeInfo2() = default;

	FunctionTypeInfo2(Ret(Class::* memberFnAddress)(Args...))
	{
		ReflectableTypeInfo& theClassReflector = Class::GetReflector2();
		memberFnAddress;
	}

	std::tuple<Args...> ParameterTypes;
};


struct TypeInfo
{
	struct toto
	{
		int test;
	};
	using MeType = TypeInfo;

	TypeInfo(const char* pName, Type pType, std::ptrdiff_t pOffset) :
		name(pName), type(pType), offset(pOffset)
	{}

	std::string		name;
	Type			type;
	std::ptrdiff_t	offset;
};

class Reflector
{
public:
	Reflector()
	{
		printf("reflector initialized\n");
	}

	void	EmplaceTypeInfo(const char* pName, Type pType, std::ptrdiff_t pOffset)
	{
		Properties.emplace_back(pName, pType, pOffset);
	}

	void test(int)
	{

	}

	std::vector<TypeInfo>	Properties;
};


template <typename T, typename TOwner = int>
class Property
{
public:
	using Owner = TOwner;

	Property(T&& defaultValue = T()) :
		myValue(std::forward<T>(defaultValue))
	{}

	operator T& ()
	{
		return myValue;
	}

	operator T const& () const
	{
		return myValue;
	}



private:
	T	myValue;
};

#define PROPERTY(type, name, value) \
	struct Typeinfo_##name {};

// This trick was inspired by https://stackoverflow.com/a/4938266/1987466
#define DECLARE_PROPERTY(type, name, value) \
	typedef struct name##_tag\
	{ \
		static ptrdiff_t Offset()\
		{ \
			return offsetof(Self, name); \
		} \
    }; \
	type name = value;\
	inline static ReflectProperty3<Self> name_TYPEINFO_PROPERTY{STRINGIZE(name), FromActualTypeToEnumType<type>::EnumType, name##_tag::Offset() };

#ifdef _MSC_VER // MSVC compilers
#define FUNCTION_NAME() __FUNCTION__
#else
#define FUNCTION_NAME() __func__

#endif

template <typename T>
struct Reflectable
{
	using ContainerType = T;
	static Reflector& GetReflector()
	{
		static Reflector theClassReflector;
		return theClassReflector;
	}

	static ReflectableTypeInfo& GetReflector2()
	{
		static ReflectableTypeInfo theClassReflector;
		return theClassReflector;
	}

	template <typename U>
	U const& GetProperty2(std::string_view pName) const
	{
		ReflectableTypeInfo& refl = T::GetReflector2();
		for (TypeInfo2 const& prop : refl.Properties)
		{
			if (prop.name == pName)
				return *reinterpret_cast<U const*>(this + prop.offset);
		}

		return U();
	}

	template <typename U>
	U const& GetProperty(std::string_view pName) const
	{
		Reflector& refl = T::GetReflector();
		for (auto const& prop : refl.Properties)
		{
			if (prop.name == pName)
				return *reinterpret_cast<U const*>(this + prop.offset);
		}

		return U();
	}

	template <typename U>
	void SetProperty(std::string_view pName, U&& pSetValue)
	{
		Reflector& refl = T::GetReflector();
		for (auto const& prop : refl.Properties)
		{
			if (prop.name == pName)
			{
				*reinterpret_cast<U*>(this + prop.offset) = std::forward<U>(pSetValue);
				return;
			}
		}

		// throw exception
	}


	template <typename U>
	void SetProperty2(std::string_view pName, U&& pSetValue)
	{
		ReflectableTypeInfo& refl = T::GetReflector2();
		for (TypeInfo2 const& prop : refl.Properties)
		{
			if (prop.name == pName)
			{
				*reinterpret_cast<U*>(this + prop.offset) = std::forward<U>(pSetValue);
				return;
			}
		}

		// throw exception
	}

	FunctionInfo const* GetFunction(std::string_view pMemberFuncName)
	{
		ReflectableTypeInfo& refl = T::GetReflector2();
		for (FunctionInfo const& funcInfo : refl.MemberFunctions)
		{
			if (funcInfo.name == pMemberFuncName)
			{
				return &funcInfo;
			}
		}

		return nullptr;
	}


	template <typename Ret, typename... Args>
	Ret
		CallFunction(std::string_view pMemberFuncName, Args&&... args)
	{
		ReflectableTypeInfo& refl = T::GetReflector2();
		for (FunctionInfo const& funcInfo : refl.MemberFunctions)
		{
			if (funcInfo.name == pMemberFuncName)
			{
				using MemberFunctionSignature = Ret(T::*)(Args...);
				auto& theTypedFuncInfo = static_cast<FunctionTypeInfo<MemberFunctionSignature> const&>(funcInfo);
				auto* theActualObject = static_cast<T*>(this);
				Ret result = (theActualObject->*theTypedFuncInfo.myMemberFunctionPtr)(std::forward<Args>(args)...);
				return result;
			}
		}

		return Ret(); // TODO what if Ret is not default constructible?
	}

};

template <typename T>
struct ReflectProperty
{
	ReflectProperty(const char* pName, Type pType, std::ptrdiff_t pOffset)
	{
		Reflector& reflector = T::GetReflector();
		reflector.EmplaceTypeInfo(pName, pType, pOffset);
	}
};

template <typename T>
struct ReflectProperty2 : TypeInfo2
{
	ReflectProperty2(const char* pName, Type pType, std::ptrdiff_t pOffset) :
		TypeInfo2(pName, pType, pOffset)
	{
		ReflectableTypeInfo& reflector = T::GetReflector2();
		reflector.PushTypeInfo(*this);
	}
};

template <typename T>
struct ReflectProperty3 : TypeInfo2
{
	ReflectProperty3(const char* pName, Type pType, std::ptrdiff_t pOffset) :
		TypeInfo2(pName, pType, pOffset)
	{
		ReflectableTypeInfo& typeInfo = T::EditClassReflectableTypeInfo();
		typeInfo.PushTypeInfo(*this);
	}
};

//
//template <class Ty>
//class Test; /* not defined */
//template <class Ret, class Arg0>
//class Test<Ret(Arg0)> { /* whatever */ };
//template <class Ret, class Arg0, class Arg1>
//class Test<Ret(Arg0, Arg1)> { /* whatever */ };
//template <class Ret, class Arg0, class Arg1, class Arg2>
//class Test<Ret(Arg0, Arg1, Arg2)> { /* whatever */ };

template <typename T>
struct ArgumentUnpacker
{
	using ArgumentPack = T;
};

template <typename Ty>
struct FunctionTypeDecomposer; /* not defined */

template <typename Ret, typename... Args>
struct FunctionTypeDecomposer<Ret(Args...)>
{
	using ReturnType = Ret;
	using ArgumentPackType = std::tuple<Args...>;
};


template <typename Ret, typename ClassPtr, typename... Args>
struct MethodTypeDecomposer
{
	using ReturnType = Ret;
	using ContainerPtr = ClassPtr;
	using ArgumentPackType = std::tuple<Args...>;

};

template <typename Ret, typename... Args>
struct MethodRetArgs
{
	using ReturnType = Ret;
	using ArgumentPackType = std::tuple<Args...>;
};

template <typename Ty>
struct FunctionTypeDecomposer2; /* not defined */

template <typename Ret, typename Class, typename... Args>
struct FunctionTypeDecomposer2<Ret(Class ::*)(Args...)> : MethodRetArgs<Ret, Args...>
{
	using ReturnType = Ret;
	using ContainerPtr = Class;
	using ArgumentPackType = std::tuple<Args...>;
};

//
template <>
struct FunctionTypeDecomposer<void (Reflector::*)(int)>
{
	using ReturnType = void;
	using Container = Reflector;
	using ArgumentPackType = std::tuple<int>;
};


#define LPAREN (
#define RPAREN )

#define MOE_METHOD_NAMED_PARAMS(RetType, Name, ...) \
	RetType Name(GLUE_ARGUMENT_PAIRS(__VA_ARGS__)); \
	inline static FunctionTypeInfo<RetType LPAREN ContainerType::* RPAREN LPAREN EXTRACT_ODD_ARGUMENTS(__VA_ARGS__) RPAREN> Name##_TYPEINFO{&Name, STRINGIZE(Name)}

#define MOE_METHOD(RetType, Name, ...) \
	RetType Name(GLUE_ARGUMENT_PAIRS(__VA_ARGS__)); \
	inline static FunctionTypeInfo<RetType LPAREN ContainerType::* RPAREN LPAREN EXTRACT_ODD_ARGUMENTS(__VA_ARGS__) RPAREN> Name##_TYPEINFO{&Name, STRINGIZE(Name)}

#define MOE_METHOD_TYPEINFO(Name) \
	inline static FunctionTypeInfo<decltype(&Name)> Name##_TYPEINFO{ &Name, STRINGIZE(Name)}

// https://stackoverflow.com/questions/64095320/mechanism-to-check-if-a-c-member-is-private#comment113453447_64095554

struct Foo {
	void operator,(bool) const { }
};

struct Bar { Foo foo; };

template<typename T>
constexpr auto has_public_foo(T const& t) -> decltype(t.foo, void(), true) { return true; }
constexpr auto has_public_foo(...) { return false; }



// inspired by https://stackoverflow.com/a/9154394/1987466
template<class>
struct sfinae_true : std::true_type {};

template<class T, class A0>
static auto test_stream(int)
->sfinae_true<decltype(std::declval<T>().REFLECTABLE_UNIQUE_IDENTIFIER())>;
template<class, class A0>
static auto test_stream(long)->std::false_type;

template<class T, class Arg>
struct has_stream : decltype(test_stream<T, Arg>(0)){};

// should be singleton
template <typename T, typename TID = unsigned int>
struct AutomaticIDIncrementer
{
	using IDType = TID;
	// TODO: constexpr may be useless
	static AutomaticIDIncrementer& GetInstance()
	{
		static AutomaticIDIncrementer instance;
		return instance;
	}

	IDType	GetNextID()
	{
		return s_CurrentID++;
	}

private:

	AutomaticIDIncrementer() = default;

	inline static IDType	s_CurrentID = 0;
};

struct Reflectable2
{
	using ClassID = unsigned;

	ClassID	GetReflectableClassID() const
	{
		return myReflectableClassID;
	}

	IntrusiveLinkedList<TypeInfo2> const& GetProperties() const
	{
		return Reflector3::GetSingleton().GetTypeInfo(myReflectableClassID)->Properties;
	}

	IntrusiveLinkedList<FunctionInfo> const& GetMemberFunctions() const
	{
		return Reflector3::GetSingleton().GetTypeInfo(myReflectableClassID)->MemberFunctions;
	}

protected:
	void	SetReflectableClassID(ClassID newClassID)
	{
		myReflectableClassID = newClassID;
	}

public:

	DECLARE_ATTORNEY(SetReflectableClassID);

private:
	ClassID	myReflectableClassID{};
};

template <typename T>
struct ReflectableClassIDSetter
{
	ReflectableClassIDSetter()
	{
		static_assert(std::is_base_of_v<Reflectable2, T>, "This class is only supposed to be used as a member variable of a Reflectable-derived class.");

		Reflectable2* thisReflectable = reinterpret_cast<Reflectable2*>(this);
		Reflectable2::SetReflectableClassID_Attorney setterAttorney;
		setterAttorney.SetReflectableClassID(*thisReflectable, T::theTypeInfo.ReflectableID);
	}
};


// This was inspired by https://stackoverflow.com/a/70701479/1987466 and https://gcc.godbolt.org/z/rrb1jdTrK
namespace SelfType
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

struct a : Reflectable<a>
{
	using Self = a;
	using Parent = Self;
	using Super = void;

	int test()
	{
		return 42;
	}
};

struct b : a
{
	using Parent = Self;
	using Super = a;


	static char const* REFLECTABLE_UNIQUE_IDENTIFIER()
	{
		static char const* uid = FUNCTION_NAME();
		return uid;
	}
	struct _self_type_tag {};
	constexpr auto _self_type_helper() -> decltype(::SelfType::Writer<_self_type_tag, decltype(this)>{});
	using Self = ::SelfType::Read<_self_type_tag>;

	//const char* REFLECTABLE_UNIQUE_IDENTIFIER()
	//{
	//	return nullptr;
	//}

	void Roger()
	{}

	void stream(int) {}

};

b::Self gloB;


#define FIRST_TYPE_0() Reflectable2
#define FIRST_TYPE_1(a, ...) a
#define FIRST_TYPE_2(a, ...) a
#define FIRST_TYPE_3(a, ...) a
#define FIRST_TYPE_4(a, ...) a

#define INHERITANCE_LIST_1(declaredTypeName, ...) : Reflectable2 COMMA private ReflectableClassIDSetter<declaredTypeName>
#define INHERITANCE_LIST_2(declaredTypeName, ...) : COMMA_ARGS1(__VA_ARGS__)
#define INHERITANCE_LIST_3(declaredTypeName, ...) : COMMA_ARGS2(__VA_ARGS__)
#define INHERITANCE_LIST_4(declaredTypeName, ...) : COMMA_ARGS3(__VA_ARGS__)
#define INHERITANCE_LIST_5(declaredTypeName, ...) : COMMA_ARGS4(__VA_ARGS__)
#define INHERITANCE_LIST(...) VA_MACRO(INHERITANCE_LIST_, __VA_ARGS__)

template <typename T>
struct TypeInfoHelper
{};

#define reflectable_class(classname, ...) class classname INHERITANCE_LIST(__VA_ARGS__)
#define reflectable_struct(structname, ...) \
	struct structname;\
	template <>\
	struct TypeInfoHelper<structname>\
	{\
		using Super = VA_MACRO(FIRST_TYPE_, __VA_ARGS__);\
		inline static char const* TypeName = STRINGIZE(structname);\
	};\
	struct structname INHERITANCE_LIST(structname, __VA_ARGS__)


#define DECLARE_REFLECTABLE_INFO() \
	struct _self_type_tag {}; \
    constexpr auto _self_type_helper() -> decltype(::SelfType::Writer<_self_type_tag, decltype(this)>{}); \
    using Self = ::SelfType::Read<_self_type_tag>;\
	using Super = TypeInfoHelper<Self>::Super;\
	inline static ReflectableTypeInfo theTypeInfo{TypeInfoHelper<Self>::TypeName};\
	static ReflectableTypeInfo const&	GetClassReflectableTypeInfo()\
	{\
		return theTypeInfo;\
	}\
	static ReflectableTypeInfo&	EditClassReflectableTypeInfo()\
	{\
		return theTypeInfo;\
	}




reflectable_struct(c, Reflectable2)
{
	DECLARE_REFLECTABLE_INFO()

	DECLARE_PROPERTY(int, toto, 0)
};


//TODO LIST:
//- SubclassOf
//- Create/Clone
//- class/structure members: du style SetProperty("maStruct.myInt", 1)
//- fonctions virtuelles
//- IsA

class Test : public Reflectable<Test>
{
	int titi = 4444;
	int bobo;
	float arrrr;
	friend Reflectable<Test>;

	MOE_METHOD_NAMED_PARAMS(int, tititest, int, tata);
	MOE_METHOD(int, grostoto, bool);
	int zdzdzdz(float bibi);
	MOE_METHOD_TYPEINFO(zdzdzdz);
	//using zdzdzdz_FUNCTYPE = decltype(&Reflectable<Test>::ContainerType::zdzdzdz);
	//zdzdzdz_FUNCTYPE test;
	//std::tuple<Test*> tri;
	//using MTD = MethodTypeDecomposer<void, Test*, float >::ReturnType;
	////using Ptr = FTD::ReturnType;
	////inline static FunctionTypeInfo<ContainerType, FunctionTypeDecomposer<Test>>::ReturnType, FunctionTypeDecomposer<zdzdzdz_FUNCTYPE>::ArgumentPackType> zdzdzdz_TYPEINFO{ &zdzdzdz };
	//FunctionTypeInfo2<Test, void, float> aaaaaaa;

	//using MRA = FunctionTypeDecomposer2<decltype(&Test::zdzdzdz)>;
	//inline static FunctionTypeInfo<decltype(&zdzdzdz)> zdzdzdz_TYPEINFO{ &zdzdzdz};
	//int tititest(GLUE_ARGUMENT_PAIRS(int, dzdefezfze)); inline static FunctionTypeInfo<ContainerType, int, EXTRACT_ODD_ARGUMENTS(int, tata)> Name_TYPEINFO{&tititest};
	//int grostoto(bool);
	//inline static FunctionTypeInfo<ContainerType, int, int> Name_TYPEINFO{ &tititest };;

//	int tititest(GLUE_ARGUMENT_PAIRS(int, tata)); inline static FunctionTypeInfo< int> Name_TYPEINFO{&tititest};
	//int tititest(int tata)
	//{
	//	return 0;
	//}
	//inline static FunctionTypeInfo < Test, int, int > Name_TYPEINFO{ &tititest };


};


int test333(int a, float b, char c)
{
	return 0;
}

int ia = __COUNTER__;
#if __COUNTER__ == 0
int i = __COUNTER__;
int j = __COUNTER__;
#define __COUNTER__ 0

int k = __COUNTER__;
#endif

int main()
{
	c::Self zfzfzfz;
	c::Super efefe;
	Property<int> test = 0;
	test = 42;
	Test tata;
	int titi = test;
	if (test == 0)
	{
		printf("yes\n");
	}
	auto& refl = Test::GetReflector();
	for (auto const& t : refl.Properties)
	{
		printf("t : %s\n", t.name.c_str());
	}

	auto const& myTiti = tata.GetProperty<int>("titi");
	auto const& myTiti2 = tata.GetProperty2<int>("titi");
	tata.SetProperty<int>("titi", 42);
	tata.SetProperty2<int>("titi", 1337);

	int nbArgs = NARGS(1, 2, 3, 4, 5, 6, 7, 8, 9);
	int nbArgs2 = NARGS(1, 2);
	int nbArgs3 = NARGS();
	EXTRACT_ODD_ARGUMENTS(int rrrr = 42, prout, pppp = 1111);
	// NARGS
	printf("%d\n", NARGS());          // Prints 0
	printf("%d\n", NARGS(1));         // Prints 1
	printf("%d\n", NARGS(1, 2));      // Prints 2
	fflush(stdout);

#ifdef _MSC_VER
	// NARGS minus 1
	printf("\n");
	printf("%d\n", NARGS_1(1));       // Prints 0
	printf("%d\n", NARGS_1(1, 2));    // Prints 1
	printf("%d\n", NARGS_1(1, 2, 3)); // Prints 2
#endif

	std::tuple<FunctionTypeDecomposer<decltype(test333)>::ArgumentPackType> testTuple;

	auto& refl2 = Test::GetReflector2();
	int ret = tata.CallFunction<int, float>("zdzdzdz", 3.f);

	auto* funcInfo = tata.GetFunction("zdzdzdz");
	int ret2 = funcInfo->TInvokeFunc<int>(&tata, 3.f);

	int tets = 1;
	int ffs = FindFirstSetBit(tets);

	BitEnum be = BitEnum::seize;

	const char* seize = be.FromEnum((BitEnum::Type)2);
	auto isset = be.IsSet(BitEnum::un);
	be.Set(BitEnum::seize);
	isset = be.IsSet(BitEnum::un) && be.IsSet(BitEnum::seize) && !be.IsSet(BitEnum::deux);
	be.Clear(BitEnum::seize);
	isset = be.IsSet(BitEnum::un) && !be.IsSet(BitEnum::seize) && !be.IsSet(BitEnum::deux);
	test333(1, 2, 3);

	LinearEnum33 linear2 = LinearEnum33::deux;
	LinearEnum linear;
	linear.Value = LinearEnum::deux;
	const char* ename = LinearEnum::FromEnum(linear.Value);
	LinearEnum::Type fromStr = LinearEnum::FromSafeString("deux");

	ename = LinearEnum33::FromEnum(linear2.Value);
	linear2 = LinearEnum33::un;
	ename = LinearEnum33::FromEnum(linear2.Value);

	Reflector3& aaaaaaaaaa = Reflector3::EditSingleton();

	c pouet;
	for (auto it = c::GetClassReflectableTypeInfo().Properties.begin(); it; ++it)
	{
		printf("name: %s, type: %d, offset: %lu", (*it).name.c_str(), (*it).type, (*it).offset);
	}

	return 0;

}


/**
 * \brief test
 * \param tata
 * \return
 */
int Test::tititest(int tata)
{
	return 0;
}

int Test::grostoto(bool)
{
	return 0;
}


int Test::zdzdzdz(float bibi)
{
	return 42;
}
