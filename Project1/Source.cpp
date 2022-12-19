
#include <any>
#include <cassert>
#include <charconv>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <string>
#include <unordered_map>
#include <fstream>
#include <map>
#include <sstream>

#define USE_SERIALIZE_API 1
#define RAPIDJSON_REFLECTION_SERIALIZER 1

#if USE_SERIALIZE_API
# if RAPIDJSON_REFLECTION_SERIALIZER
struct RapidJsonReflectorSerializer;
typedef RapidJsonReflectorSerializer REFLECTOR_SERIALIZER_TYPE;
# endif
#endif

struct MapPropertyCRUDHandler;
struct ArrayPropertyCRUDHandler;
struct Reflectable2;
class ReflectableTypeInfo;

enum class Type
{
	Unknown,
	Void,
	Bool,
	Int,
	Uint,
	Int64,
	Uint64,
	Float,
	Double,
	String,
	Array,
	Map,
	Enum,
	Object
};
// TODO: what about char, short? String is not covered either?

template <Type T>
struct FromEnumTypeToActualType;

template <typename T>
struct FromActualTypeToEnumType
{
	// If the type was not registered, by default it's unknown
	static const Type EnumType = Type::Unknown;
};

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

DECLARE_TYPE_TRANSLATOR(Type::Void, void)
DECLARE_TYPE_TRANSLATOR(Type::Bool, bool)
DECLARE_TYPE_TRANSLATOR(Type::Int, int)
DECLARE_TYPE_TRANSLATOR(Type::Uint, unsigned)
DECLARE_TYPE_TRANSLATOR(Type::Int64, int64_t)
DECLARE_TYPE_TRANSLATOR(Type::Uint64, uint64_t)
DECLARE_TYPE_TRANSLATOR(Type::Float, float)
DECLARE_TYPE_TRANSLATOR(Type::Double, double)


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

struct AbstractPropertyHandler
{
	// TODO: try to find a way to merge these two,
	// because a property can either be an array or map, but not both...
	ArrayPropertyCRUDHandler const* ArrayHandler = nullptr;
	MapPropertyCRUDHandler const* MapHandler = nullptr;
} ;


// TODO: put in proper place
static unsigned const INVALID_REFLECTABLE_ID = UINT32_MAX;

struct TypeInfo2 : IntrusiveListNode<TypeInfo2>
{
	using CopyConstructorPtr = void (*)(void* pDestAddr, void const* pOther, size_t pOffset);

#if USE_SERIALIZE_API
	using SerializeFptr = void(REFLECTOR_SERIALIZER_TYPE::*)(TypeInfo2 const& pSerializedTypeInfo, void const* pPropPtr);
#endif

	TypeInfo2(char const* pName, std::ptrdiff_t const pOffset, size_t const pSize, CopyConstructorPtr const pCopyCtor = nullptr) :
		name(pName), type(Type::Unknown), offset(pOffset), size(pSize), CopyCtor(pCopyCtor)
	{}

	ArrayPropertyCRUDHandler const*	GetArrayHandler() const
	{
		return DataStructurePropertyHandler.ArrayHandler;
	}

	MapPropertyCRUDHandler const* GetMapHandler() const
	{
		return DataStructurePropertyHandler.MapHandler;
	}

	AbstractPropertyHandler const&	GetDataStructureHandler() const
	{
		return DataStructurePropertyHandler;
	}

	std::string		name; // TODO: try storing a string view?
	Type			type;
	std::ptrdiff_t	offset;
	std::size_t		size;
	AbstractPropertyHandler	DataStructurePropertyHandler; // Useful for array-like or associative data structures, will stay null for other types.
	CopyConstructorPtr CopyCtor = nullptr; // if copy constructor ptr is null : this type is not copy-constructible

#if USE_SERIALIZE_API
	SerializeFptr	SerializerFunction = nullptr;
#endif

	// TODO: storing it here is kind of a hack to go quick.
	// I guess we should have a map of Type->ClassID to be able to easily find the class ID...
	// without duplicating this information in all the type info structures of every property of the same type.
	unsigned		reflectableID{ INVALID_REFLECTABLE_ID };

protected:

	void	SetType(Type const pType)
	{
		type = pType;
	}
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
struct Reflector3 : Singleton<Reflector3>, AutomaticTypeCounter<Reflector3, unsigned>
{
	inline static const int REFLECTOR_VERSION = 0;

public:

	Reflector3() = default;

	[[nodiscard]] size_t	GetTypeInfoCount() const
	{
		return myReflectableTypeInfos.size();
	}

	[[nodiscard]] ReflectableTypeInfo const* GetTypeInfo(unsigned classID) const
	{
		if (classID < myReflectableTypeInfos.size())
		{
			return myReflectableTypeInfos[classID];
		}

		return nullptr;
	}

	[[nodiscard]] ReflectableTypeInfo* EditTypeInfo(unsigned classID)
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

	[[nodiscard]] Reflectable2*	TryInstantiate(unsigned pClassID, std::any const& pAnyParameterPack) const
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

	bool	ExportTypeInfoSettings(std::string_view pWrittenSettingsFile) const;
	bool	ImportTypeInfoSettings(std::string_view pReadSettingsFile);

private:
	friend ReflectableTypeInfo;

	[[nodiscard]] unsigned	RegisterTypeInfo(ReflectableTypeInfo* pTypeInfo)
	{
		myReflectableTypeInfos.push_back(pTypeInfo);
		return GetTypeInfoCount() - 1;
	}

	std::vector<ReflectableTypeInfo*>	myReflectableTypeInfos;
	ReflectableFactory	myInstantiateFactory;
};

class ReflectableTypeInfo
{
public:

	ReflectableTypeInfo() = default; // TODO: remove

	ReflectableTypeInfo(char const* pTypename) :
		TypeName(pTypename)
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

	void	AddParentClass(ReflectableTypeInfo* pParent)
	{
		ParentClasses.push_back(pParent);
	}

	void	AddChildClass(ReflectableTypeInfo* pChild)
	{
		ChildrenClasses.push_back(pChild);
	}

	[[nodiscard]] bool	IsParentOf(unsigned pChildClassID) const
	{
		return (std::find_if(ChildrenClasses.begin(), ChildrenClasses.end(),
			[pChildClassID](ReflectableTypeInfo* pChildTypeInfo)
			{
				return pChildTypeInfo->ReflectableID == pChildClassID;
			}) != ChildrenClasses.end());
	}

	using ParentPropertyInfo = std::pair< ReflectableTypeInfo const*, TypeInfo2 const*>;
	[[nodiscard]] ParentPropertyInfo	FindParentClassProperty(std::string_view const& pName) const
	{
		for (ReflectableTypeInfo const* parentClass : ParentClasses)
		{
			TypeInfo2 const* foundProp = parentClass->FindProperty(pName);
			if (foundProp != nullptr)
			{
				return { parentClass, foundProp };
			}
		}

		return { nullptr, nullptr };
	}

	[[nodiscard]] TypeInfo2 const*	FindProperty(std::string_view const& pName) const
	{
		for (auto const& prop : Properties)
		{
			if (prop.name == pName)
			{
				return &prop;
			}
		}

		return nullptr;
	}

	[[nodiscard]] TypeInfo2 const* FindPropertyInHierarchy(std::string_view const& pName) const
	{
		for (auto const& prop : Properties)
		{
			if (prop.name == pName)
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

	[[nodiscard]] FunctionInfo const* FindFunction(std::string_view const& pFuncName) const
	{
		for (FunctionInfo const& aFuncInfo : MemberFunctions)
		{
			if (aFuncInfo.name == pFuncName)
			{
				return &aFuncInfo;
			}
		}

		return nullptr;
	}

	using ParentFunctionInfo = std::pair< ReflectableTypeInfo const*, FunctionInfo const*>;
	[[nodiscard]] ParentFunctionInfo	FindFunctionInHierarchy(std::string_view const& pFuncName) const
	{
		for (ReflectableTypeInfo const* parentClass : ParentClasses)
		{
			FunctionInfo const* foundFunc = parentClass->FindFunction(pFuncName);
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
		for (auto const& prop : Properties)
		{
			pVisitorFunction(prop);
		}

		for (ReflectableTypeInfo const* parentClass : ParentClasses)
		{
			for (auto const& prop : parentClass->Properties)
			{
				pVisitorFunction(prop);
			}
		}
	}


	unsigned							ReflectableID{INVALID_REFLECTABLE_ID};
	int									VirtualOffset{ 0 }; // for polymorphic classes
	std::string_view					TypeName;
	IntrusiveLinkedList<TypeInfo2>		Properties;
	IntrusiveLinkedList<FunctionInfo>	MemberFunctions;
	std::vector<ReflectableTypeInfo*>	ParentClasses;
	std::vector<ReflectableTypeInfo*>	ChildrenClasses;
};


bool Reflector3::ExportTypeInfoSettings(std::string_view pWrittenSettingsFile) const
{
	// This follows a very simple binary serialization process right now. It encodes:
	// - file version
	// - total number of encoded types
	// - the reflectable type ID
	// - the length of typename string
	// - the typename string
	std::string writeBuffer;
	auto nbTypeInfos = (unsigned) myReflectableTypeInfos.size();
	// TODO: unsigned -> ReflectableID
	writeBuffer.resize(sizeof(REFLECTOR_VERSION) + sizeof(nbTypeInfos) + sizeof(size_t) * myReflectableTypeInfos.size());
	size_t offset = 0;

	// TODO: it's possible to compress the data even more if we're willing to make the logic more complex:
	// - use a unsigned int instead of size_t (4 billions reflectable IDs are probably enough)
	// - adding a size_t to flag each string length allows us to avoid doing Strlen's to figure out the length of each string.
	//		But it's probably not worth it as it stores 8 bytes for a size_t, and the type names are usually short (less than 32 chars).
	//		It then would make sense to stop storing the length and store a single '\0' character at the end of each name instead.
	//		It needs us to perform 1 strlen per typename but since these are supposed to be short it should be ok and allows us to save on memory.

	memcpy(writeBuffer.data() + offset, (const char*)&REFLECTOR_VERSION, sizeof(REFLECTOR_VERSION));
	offset += sizeof(REFLECTOR_VERSION);

	memcpy(writeBuffer.data() + offset, (const char*)&nbTypeInfos, sizeof(nbTypeInfos));
	offset += sizeof(nbTypeInfos);

	for (ReflectableTypeInfo const* typeInfo : myReflectableTypeInfos)
	{
		auto nameLen = (unsigned) typeInfo->TypeName.length();
		memcpy(writeBuffer.data() + offset, (const char*)&nameLen, sizeof(nameLen));
		offset += sizeof(nameLen);

		writeBuffer.resize(writeBuffer.size() + typeInfo->TypeName.length());
		memcpy(writeBuffer.data() + offset, typeInfo->TypeName.data(), typeInfo->TypeName.length());
		offset += typeInfo->TypeName.length();
	}

	std::ofstream openFile{ pWrittenSettingsFile.data(), std::ios::binary };
	if (openFile.bad())
	{
		return false;
	}

	// TODO: check for exception?
	openFile.write(writeBuffer.data(), writeBuffer.size());
	return true;
}

bool Reflector3::ImportTypeInfoSettings(std::string_view pReadSettingsFile) 
{
	// Importing settings is trickier than exporting them because all the static initialization
	// is already done at import time. This gives a lot of opportunities to mess up!
	// For example : types can have changed names, the same type can now have a different reflectable ID,
	// there can be "twin types" (types with the same names, how to differentiate them?),
	// "orphaned types" (types that were imported but are not in the executable anymore)...
	// This function tries to "patch the holes" as best as it can.

	std::string readBuffer;
	{
		std::ifstream openFile(pReadSettingsFile.data(), std::ios::binary);
		if (openFile.bad())
		{
			return false;
		}

		openFile.seekg(0, std::ios::end);
		readBuffer.resize(openFile.tellg());
		openFile.seekg(0, std::ios::beg);
		openFile.read(readBuffer.data(), readBuffer.size());
	}

	size_t offset = 0;

	int& fileVersion = *reinterpret_cast<int*>(readBuffer.data() + offset);
	offset += sizeof(fileVersion);

	if (fileVersion != REFLECTOR_VERSION)
	{
		return false; // Let's assume that reflector is not backward compatible for now.
	}

	unsigned& nbTypeInfos = *reinterpret_cast<unsigned*>(readBuffer.data() + offset);
	offset += sizeof(nbTypeInfos);
	std::vector<ExportedTypeInfoData> theReadData(nbTypeInfos);

	int iTypeInfo = 0;
	while (iTypeInfo < nbTypeInfos)
	{
		ExportedTypeInfoData& curData = theReadData[iTypeInfo];

		curData.ReflectableID = iTypeInfo;

		unsigned& typeNameLen = *reinterpret_cast<unsigned*>(readBuffer.data() + offset);
		offset += sizeof(typeNameLen);

		curData.TypeName.resize(typeNameLen);
		memcpy(curData.TypeName.data(), readBuffer.data() + offset, typeNameLen);
		offset += typeNameLen;

		iTypeInfo++;
	}

	// Walk our registry and see if we find the imported types. Patch the reflectable type ID if necessary.
	std::vector<ExportedTypeInfoData*> orphanedTypes; // Types that were "lost" - cannot find them in the registry. Will have to "guess" if an existing type matches them or not.

	// Always resize the registry to at least the size of data we read,
	// because we absolutely have to keep the same ID for imported types.
	if (myReflectableTypeInfos.size() < theReadData.size())
	{
		myReflectableTypeInfos.resize(theReadData.size());
	}

	for (int iReg = 0; iReg < myReflectableTypeInfos.size(); ++iReg)
	{
		auto& registeredTypeInfo = myReflectableTypeInfos[iReg];

		if (iReg >= theReadData.size())
		{
			// We reached the end of imported types : all the next ones are new types
			break;
		}

		auto& readTypeInfo = theReadData[iReg];

		// There can be null type infos due to a previous resize if there are more imported types than registered types.
		if (registeredTypeInfo == nullptr || readTypeInfo.TypeName != registeredTypeInfo->TypeName)
		{
			// mismatch: try to find the read typeinfo in the registered data
			auto it = std::find_if(myReflectableTypeInfos.begin(), myReflectableTypeInfos.end(),
				[readTypeInfo](ReflectableTypeInfo const* pTypeInfo)
				{
					return pTypeInfo != nullptr && pTypeInfo->TypeName == readTypeInfo.TypeName;
				});
			if (it != myReflectableTypeInfos.end())
			{
				std::iter_swap(myReflectableTypeInfos.begin() + iReg, it);
				myReflectableTypeInfos[iReg]->ReflectableID = readTypeInfo.ReflectableID;
			}
			else
			{
				orphanedTypes.push_back(&readTypeInfo);
				if (registeredTypeInfo != nullptr)
				{
					// If we are here, the registered type is new one (not found in imported types),
					// so push it back to fix its reflectable ID later. We should let the not found type block "empty".
					myReflectableTypeInfos.push_back(registeredTypeInfo);
					registeredTypeInfo = nullptr;
				}
			}
		}
		else if (readTypeInfo.ReflectableID != registeredTypeInfo->ReflectableID)
		{
			registeredTypeInfo->ReflectableID = readTypeInfo.ReflectableID;
		}
	}

	// In case of new types - make sure their IDs are correct, otherwise fix them
	for (int iReg = theReadData.size(); iReg < myReflectableTypeInfos.size(); ++iReg)
	{
		auto& registeredTypeInfo = myReflectableTypeInfos[iReg];
		if (registeredTypeInfo != nullptr && registeredTypeInfo->ReflectableID != iReg)
		{
			registeredTypeInfo->ReflectableID = iReg;
		}
	}

	return true;
}



template <typename T, bool>
struct TypedReflectableTypeInfo;

template <typename T>
struct TypeInfoHelper
{};

struct Reflectable2;

template <typename T, bool B, typename Parent>
void RecursiveRegisterParentClasses(TypedReflectableTypeInfo <T, B> & pRegisteringClass)
{
	if constexpr (!std::is_same_v<Parent, Reflectable2>)
	{
		ReflectableTypeInfo& parentTypeInfo = Parent::EditClassReflectableTypeInfo();
		parentTypeInfo.AddChildClass(&pRegisteringClass);
		pRegisteringClass.AddParentClass(&parentTypeInfo);
		RecursiveRegisterParentClasses<T, B, Parent::Super>(pRegisteringClass);
	}
}

template <typename T, bool UseDefaultCtorForInstantiate>
struct TypedReflectableTypeInfo : ReflectableTypeInfo
{
public:
	TypedReflectableTypeInfo(char const* pTypename) :
		ReflectableTypeInfo(pTypename)
	{
		RecursiveRegisterParentClasses <T, UseDefaultCtorForInstantiate, typename T::Super>(*this);
		if constexpr (UseDefaultCtorForInstantiate && std::is_default_constructible_v<T>)
		{
			static_assert(std::is_base_of_v<Reflectable2, T>, "This class is only supposed to be used as a member variable of a Reflectable-derived class.");
			Reflector3::EditSingleton().RegisterInstantiateFunction(T::GetClassReflectableTypeInfo().ReflectableID,
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

};

template <typename Ty >
struct FunctionTypeInfo; // not defined

// TODO: figure out the proper constructor/operator rain dance
template <typename Class, typename Ret, typename ... Args >
struct FunctionTypeInfo<Ret(Class::*)(Args...)> : FunctionInfo
{
	using MemberFunctionSignature = Ret(Class::*)(Args...);

	FunctionTypeInfo(MemberFunctionSignature memberFunc, const char* methodName) :
		FunctionInfo(methodName, FromActualTypeToEnumType<Ret>::EnumType),
		myMemberFunctionPtr(memberFunc)
	{
		if constexpr (sizeof...(Args) > 0)
		{
			AddParameters<Args...>();
		}

		ReflectableTypeInfo& theClassReflector = Class::EditClassReflectableTypeInfo();
		theClassReflector.PushFrontFunctionInfo(*this);
	}

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

	// The idea of using std::apply has been stolen from here : https://stackoverflow.com/a/36656413/1987466
	std::any	Invoke(void* pObject, std::any const& pInvokeParams) const override
	{
		using ArgumentsTuple = std::tuple<Args&&...>;
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


#define GLUE_UNDERSCORE_ARGS1(a1) a1##_INSTANTIATOR
#define GLUE_UNDERSCORE_ARGS2(a1, a2) a1##_##a2##_INSTANTIATOR
#define GLUE_UNDERSCORE_ARGS3(a1, a2, a3) a1##_##a2##_##a3##_INSTANTIATOR
#define GLUE_UNDERSCORE_ARGS4(a1,a2,a3,a4) a1##_##a2##_##a3##_##a4##_INSTANTIATOR
#define GLUE_UNDERSCORE_ARGS5(a1,a2,a3,a4,a5) a1##_##a2##_##a3##_##a4##_##a5##_INSTANTIATOR
#define GLUE_UNDERSCORE_ARGS6(a1,a2,a3,a4,a5,a6) a1##_##a2##_##a3##_##a4##_##a5##_##a6##_INSTANTIATOR
#define GLUE_UNDERSCORE_ARGS7(a1,a2,a3,a4,a5,a6,a7) a1##_##a2##_##a3##_##a4##_##a5##_##a6##_##a7##_INSTANTIATOR
#define GLUE_UNDERSCORE_ARGS8(a1,a2,a3,a4,a5,a6,a7,a8) a1##_##a2##_##a3##_##a4##_##a5##_##a6##_##a7##_##a8##_INSTANTIATOR

#define GLUE_ARGS0()
#define GLUE_ARGS1(a1) a1
#define GLUE_ARGS2(a1, a2) a1 a2
#define GLUE_ARGS3(a1, a2, a3) a1 a2 COMMA a3
#define GLUE_ARGS4(a1,a2,a3,a4) a1 a2 COMMA a3 a4
#define GLUE_ARGS5(a1,a2,a3,a4,a5) a1 a2 COMMA a3 a4 COMMA a5
#define GLUE_ARGS6(a1,a2,a3,a4,a5,a6) a1 a2 COMMA a3 a4 COMMA a5 a6
#define GLUE_ARGS7(a1,a2,a3,a4,a5,a6,a7) a1 a2 COMMA a3 a4 COMMA a5 a6 COMMA a7
#define GLUE_ARGS8(a1,a2,a3,a4,a5,a6,a7,a8) a1 a2 COMMA a3 a4 COMMA a5 a6 COMMA a7 a8
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
#define COMMA_ARGS2(a1, a2) a1 COMMA a2
#define COMMA_ARGS3(a1, a2, a3) a1 COMMA a2 COMMA a3
#define COMMA_ARGS4(a1, a2, a3, a4) a1 COMMA a2 COMMA a3 COMMA a4
#define COMMA_ARGS5(a1, a2, a3, a4, a5) a1 COMMA a2 COMMA a3 COMMA a4 COMMA a5

#define BRACES_STRINGIZE_COMMA_VALUE(a) { STRINGIZE(a) COMMA a  } COMMA

#define BRACES_STRINGIZE_COMMA_VALUE1(a1) BRACES_STRINGIZE_COMMA_VALUE(a1)
#define BRACES_STRINGIZE_COMMA_VALUE2(a1, a2) BRACES_STRINGIZE_COMMA_VALUE(a1) BRACES_STRINGIZE_COMMA_VALUE(a2)
#define BRACES_STRINGIZE_COMMA_VALUE3(a1, a2, a3) BRACES_STRINGIZE_COMMA_VALUE(a1) BRACES_STRINGIZE_COMMA_VALUE(a2) BRACES_STRINGIZE_COMMA_VALUE(a3)
#define BRACES_STRINGIZE_COMMA_VALUE4(a1, a2, a3, a4) BRACES_STRINGIZE_COMMA_VALUE(a1) BRACES_STRINGIZE_COMMA_VALUE(a2) BRACES_STRINGIZE_COMMA_VALUE(a3)  BRACES_STRINGIZE_COMMA_VALUE(a4)
#define BRACES_STRINGIZE_COMMA_VALUE5(a1, a2, a3, a4, a5) BRACES_STRINGIZE_COMMA_VALUE(a1) BRACES_STRINGIZE_COMMA_VALUE(a2) BRACES_STRINGIZE_COMMA_VALUE(a3)  BRACES_STRINGIZE_COMMA_VALUE(a4)  BRACES_STRINGIZE_COMMA_VALUE(a5)


#define EVAL_1(...) __VA_ARGS__
#define CONCAT(a, b) a ## b
#define CONCAT2(a, b) CONCAT(a, b)

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

// inspired by https://stackoverflow.com/a/4298441/1987466

template<typename T>
struct GetParenthesizedType;

template<typename R, typename P1>
struct GetParenthesizedType<R(P1)>
{
	typedef P1 Type;
};

#define PROPERTY(type, name, value) \
	struct Typeinfo_##name {};

// This offset trick was inspired by https://stackoverflow.com/a/4938266/1987466
#define DECLARE_PROPERTY(type, name, ...) \
	struct name##_tag\
	{ \
		static ptrdiff_t Offset()\
		{ \
			return offsetof(Self, name); \
		} \
    }; \
	GetParenthesizedType<void LPAREN UNPAREN(type) RPAREN>::Type name{__VA_ARGS__};\
	inline static ReflectProperty3<Self, UNPAREN(type)> name##_TYPEINFO_PROPERTY{STRINGIZE(name), name##_tag::Offset() };

// inspired by https://www.mikeash.com/pyblog/friday-qa-2015-03-20-preprocessor-abuse-and-optional-parentheses.html
#define EXTRACT(...) EXTRACT __VA_ARGS__
#define NOTHING_EXTRACT
#define PASTE(x, ...) x ## __VA_ARGS__
#define EVALUATING_PASTE(x, ...) PASTE(x, __VA_ARGS__)
#define UNPAREN(x) EVALUATING_PASTE(NOTHING_, EXTRACT x)

// inspired from https://stackoverflow.com/a/18523890/1987466
#define Paste(a, b)     a ## _ ## b
#define Helper(a, b)    Paste(a, b)
#define Merge(a, b)   Helper(a, b)


#define DECLARE_PROPERTY3(type, name, ...) \
	struct name##_tag\
	{ \
		static ptrdiff_t Offset()\
		{ \
			return offsetof(Self, name); \
		} \
    }; \
	GetParenthesizedType<void LPAREN UNPAREN(type) RPAREN>::Type name{__VA_ARGS__};\
	inline static ReflectProperty3<Self, UNPAREN(type)> name##_TYPEINFO_PROPERTY{STRINGIZE(name), name##_tag::Offset() };


#define DECLARE_ARRAY_PROPERTY(type, name, size, ...) \
	struct name##_tag\
	{ \
		static ptrdiff_t Offset()\
		{ \
			return offsetof(Self, name); \
		} \
	}; \
	GetParenthesizedType<void LPAREN UNPAREN(type) RPAREN>::Type name##size{ __VA_ARGS__ }; \
	inline static ReflectProperty3<Self, UNPAREN(type) ## size> name##_TYPEINFO_PROPERTY{ STRINGIZE(name), name##_tag::Offset() };


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
constexpr std::enable_if_t<std::is_base_of_v<Reflectable2, T>, unsigned>
FindClassReflectableID()
{
	return T::GetClassReflectableTypeInfo().ReflectableID;
}


template <typename T>
constexpr std::enable_if_t<!std::is_base_of_v<Reflectable2, T>, unsigned>
FindClassReflectableID()
{
	return INVALID_REFLECTABLE_ID;
}

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


// Inspired by https://stackoverflow.com/a/62061759/1987466
//template <typename... Ts>
//using void_t = void;
//template <template <class...> class Trait, class AlwaysVoid, class... Args>
//struct detector : std::false_type {};
//template <template <class...> class Trait, class... Args>
//struct detector<Trait, void_t<Trait<Args...>>, Args...> : std::true_type {};


// Inspired by https://stackoverflow.com/a/71574097/1987466
// Useful for member function names like "operator[]" that would break the build if used as is
#define MEMBER_FUNCTION_ALIAS_DETECTOR_PARAM1(Alias, FunctionName, Param1) \
	template <typename ContainerType>\
	using Alias##_method_t = decltype(std::declval< ContainerType >().FunctionName(Param1));\
	template <typename ContainerType COMMA typename = std::void_t<>>\
	struct has_##Alias : std::false_type {};\
	template <typename ContainerType>\
	struct has_##Alias<ContainerType COMMA std::void_t<Alias##_method_t< ContainerType >>> :\
		std::true_type {};\
	template <typename ContainerType>\
	inline constexpr bool has_##Alias##_v = has_##Alias<ContainerType>::value;

#define MEMBER_FUNCTION_DETECTOR_PARAM1(FunctionName, Param1) \
	template <typename ContainerType>\
	using FunctionName##_method_t = decltype(std::declval< ContainerType >().FunctionName(Param1));\
	template <typename ContainerType COMMA typename = std::void_t<>>\
	struct has_##FunctionName : std::false_type {};\
	template <typename ContainerType>\
	struct has_##FunctionName<ContainerType COMMA std::void_t<FunctionName##_method_t< ContainerType >>> :\
		std::true_type {};\
	template <typename ContainerType>\
	inline constexpr bool has_##FunctionName##_v = has_##FunctionName<ContainerType>::value;

#define MEMBER_FUNCTION_DETECTOR_PARAM2(FunctionName, Param1, Param2) \
	template <typename ContainerType>\
	using FunctionName##_method_t = decltype(std::declval< ContainerType >().FunctionName(Param1, Param2));\
	template <typename ContainerType COMMA typename = std::void_t<>>\
	struct has_##FunctionName : std::false_type {};\
	template <typename ContainerType>\
	struct has_##FunctionName<ContainerType COMMA std::void_t<FunctionName##_method_t< ContainerType >>> :\
		std::true_type {};\
	template <typename ContainerType>\
	inline constexpr bool has_##FunctionName##_v = has_##FunctionName<ContainerType>::value;

#define MEMBER_FUNCTION_DETECTOR(FunctionName) \
	template <typename ContainerType>\
	using FunctionName##_method_t = decltype(std::declval< ContainerType >().FunctionName());\
	template <typename ContainerType COMMA typename = std::void_t<>>\
	struct has_##FunctionName : std::false_type {};\
	template <typename ContainerType>\
	struct has_##FunctionName<ContainerType COMMA std::void_t<FunctionName##_method_t< ContainerType >>> :\
		std::true_type {};\
	template <typename ContainerType>\
	inline constexpr bool has_##FunctionName##_v = has_##FunctionName<ContainerType>::value;

MEMBER_FUNCTION_ALIAS_DETECTOR_PARAM1(Brackets, operator[], 0)
MEMBER_FUNCTION_DETECTOR_PARAM2(insert, std::declval<ContainerType>().begin(), std::declval<ContainerType>()[0])
MEMBER_FUNCTION_DETECTOR_PARAM1(erase, std::declval<ContainerType>().begin())
MEMBER_FUNCTION_ALIAS_DETECTOR_PARAM1(MapErase, erase, typename ContainerType::key_type())
MEMBER_FUNCTION_DETECTOR(clear)
MEMBER_FUNCTION_DETECTOR(size)

// Inspired by https://stackoverflow.com/a/62203640/1987466

#define DECLARE_HAS_TYPE_DETECTOR(DetectorName, DetectedType) \
template <class T, class = void>\
struct DetectorName\
{\
	using type = std::false_type;\
};\
template <class T>\
struct DetectorName <T, std::void_t<typename T::DetectedType >>\
{\
	using type = std::true_type;\
};\
template <class T>\
inline constexpr bool DetectorName##_v = DetectorName<T>::type::value;

DECLARE_HAS_TYPE_DETECTOR(HasValueType, value_type)
DECLARE_HAS_TYPE_DETECTOR(HasKeyType, key_type)
DECLARE_HAS_TYPE_DETECTOR(HasMappedType, mapped_type)

template<typename T>
inline constexpr bool HasMapSemantics_v = has_Brackets_v<T> && HasKeyType_v<T> && HasValueType_v<T> && HasMappedType_v<T>;

template<typename T>
using HasMapSemantics_t = std::enable_if_t<HasMapSemantics_v<T> >;

template<typename T>
inline constexpr bool HasArraySemantics_v = std::is_array_v<T> || has_Brackets_v<T> && HasValueType_v<T> && !HasMapSemantics_v<T>;

template<typename T>
using HasArraySemantics_t = std::enable_if_t<HasArraySemantics_v<T>>;



#if USE_SERIALIZE_API && RAPIDJSON_REFLECTION_SERIALIZER
# include <rapidjson/rapidjson.h>
# include <rapidjson/stringbuffer.h>	// wrapper of C stream for prettywriter as output
# include <rapidjson/prettywriter.h>	// for stringify JSON

#define SERIALIZE_VALUE_SPECIALIZATION(Type, JsonFunc) \
template <>\
void	SerializeValue<Type>(void const* pPropPtr, AbstractPropertyHandler const* /*unused: pHandler*/)\
{\
	myJsonWriter.JsonFunc(*static_cast<Type const*>(pPropPtr));\
}

#define SERIALIZE_PROPERTY_SPECIALIZATION(Type) \
template <>\
void	SerializeProperty<Type>(TypeInfo2 const& pSerializedTypeInfo, void const* pPropPtr)\
{\
	myJsonWriter.String(pSerializedTypeInfo.name.data(), pSerializedTypeInfo.name.size());\
	SerializeValue<Type>(pPropPtr, &pSerializedTypeInfo.GetDataStructureHandler());\
}


namespace ReflectorConversions
{
	// inspired by https://stackoverflow.com/a/33399801/1987466
	namespace ADL
	{
		template<class T>
		std::string AsString(T&& t)
		{
			using std::to_string;
			return to_string(std::forward<T>(t));
		}
	}
	template<class T>
	std::string to_string(T&& t)
	{
		return ADL::AsString(std::forward<T>(t));
	}
}

struct RapidJsonReflectorSerializer
{
	RapidJsonReflectorSerializer() :
		myJsonWriter(myBuffer)
	{}

	using SerializeWriter = rapidjson::Writer<rapidjson::StringBuffer>;

	const char*	Serialize(Reflectable2 const& serializedObject);

	void	Deserialize(Reflectable2 const& serializedObject)
	{

	}

	using SerializeValueFptr = void	(RapidJsonReflectorSerializer::*)(void const*);

	template <typename T>
	void	SerializeValue(void const* pPropPtr, AbstractPropertyHandler const* pHandler = nullptr)
	{
		if constexpr (HasArraySemantics_v<T>)
		{
			if (pHandler != nullptr)
			{
				SerializeArrayValue(pPropPtr, pHandler->ArrayHandler);
			}
		}
		else if constexpr (HasMapSemantics_v<T>)
		{
			if (pHandler != nullptr)
			{
				SerializeMapValue(pPropPtr, pHandler->MapHandler);
			}
		}
		else if constexpr (std::is_class_v<T>)
		{
			SerializeCompoundValue(pPropPtr);
		}
	}

	SERIALIZE_VALUE_SPECIALIZATION(bool, Bool)
	SERIALIZE_VALUE_SPECIALIZATION(int, Int)
	SERIALIZE_VALUE_SPECIALIZATION(unsigned, Uint)
	SERIALIZE_VALUE_SPECIALIZATION(int64_t, Int64)
	SERIALIZE_VALUE_SPECIALIZATION(uint64_t, Uint64)
	SERIALIZE_VALUE_SPECIALIZATION(float, Double)
	SERIALIZE_VALUE_SPECIALIZATION(double, Double)

	void	SerializeArrayValue(void const* pPropPtr, ArrayPropertyCRUDHandler const* pArrayHandler);

	void	SerializeMapValue(void const* pPropPtr, MapPropertyCRUDHandler const* pMapHandler);

	template <typename K, typename V>
	void	SerializeMapKeyValuePair(K const& pKey, V const& pVal, AbstractPropertyHandler const& pValueHandler)
	{
		auto const keyStr = ReflectorConversions::to_string(pKey);
		myJsonWriter.String(keyStr.data(), keyStr.length());
		SerializeValue<V>(&pVal, &pValueHandler);
	}

	void	SerializeCompoundValue(void const* pPropPtr);

	template <typename T>
	void	SerializeProperty(TypeInfo2 const& pSerializedTypeInfo, void const* pPropPtr)
	{
		// The generic version has to analyze the type of prop to know what to do.
		switch (pSerializedTypeInfo.type)
		{
		case Type::Array:
			SerializeArrayProperty(pSerializedTypeInfo, pPropPtr);
			break;
		case Type::Map:
			SerializeMapProperty(pSerializedTypeInfo, pPropPtr);
			break;
		case Type::Object:
			SerializeCompoundProperty(pSerializedTypeInfo, pPropPtr);
			break;
		default:
			assert(false); // not supposed to happen
		}
	}

	SERIALIZE_PROPERTY_SPECIALIZATION(bool)
	SERIALIZE_PROPERTY_SPECIALIZATION(int)
	SERIALIZE_PROPERTY_SPECIALIZATION(unsigned)
	SERIALIZE_PROPERTY_SPECIALIZATION(int64_t)
	SERIALIZE_PROPERTY_SPECIALIZATION(uint64_t)
	SERIALIZE_PROPERTY_SPECIALIZATION(float)
	SERIALIZE_PROPERTY_SPECIALIZATION(double)

	void	SerializeCompoundProperty(TypeInfo2 const& pSerializedTypeInfo, void const* pPropPtr);

	void	SerializeArrayProperty(TypeInfo2 const& pSerializedTypeInfo, void const* pPropPtr)
	{
		myJsonWriter.String(pSerializedTypeInfo.name.data(), pSerializedTypeInfo.name.size());

		SerializeArrayValue(pPropPtr, pSerializedTypeInfo.DataStructurePropertyHandler.ArrayHandler);
	}

	void	SerializeMapProperty(TypeInfo2 const& pSerializedTypeInfo, void const* pPropPtr)
	{
		myJsonWriter.String(pSerializedTypeInfo.name.data(), pSerializedTypeInfo.name.size());

		SerializeMapValue(pPropPtr, pSerializedTypeInfo.DataStructurePropertyHandler.MapHandler);
	}


private:
	rapidjson::StringBuffer	myBuffer;
	rapidjson::Writer<rapidjson::StringBuffer>	myJsonWriter;
};

typedef RapidJsonReflectorSerializer REFLECTOR_SERIALIZER_TYPE;

#endif


struct MapPropertyCRUDHandler
{
	using MapReadFptr = void const* (*)(void const*, std::string_view);
	using MapUpdateFptr = void (*)(void*, std::string_view, void const* );
	using MapCreateFptr = void (*)(void*, std::string_view, void const*);
	using MapEraseFptr = void	(*)(void*, std::string_view);
	using MapClearFptr = void	(*)(void*);
	using MapSizeFptr = size_t(*)(void const*);
	using MapValueHandlerFptr = AbstractPropertyHandler (*)();
	using MapElementReflectableIDFptr = unsigned (*)();

	MapReadFptr		Read = nullptr;
	MapUpdateFptr	Update = nullptr;
	MapCreateFptr	Create = nullptr;
	MapEraseFptr	Erase = nullptr;
	MapClearFptr	Clear = nullptr;
	MapSizeFptr		Size = nullptr;
	MapValueHandlerFptr		ValueHandler = nullptr;
	MapElementReflectableIDFptr	ValueReflectableID = nullptr;

#if USE_SERIALIZE_API
	using MapSerializeFptr = void (*)(void const*, REFLECTOR_SERIALIZER_TYPE&);
	MapSerializeFptr KeyValueSerializer = nullptr;
#endif
};

// CRUD = Create, Read, Update, Delete
template <typename T, typename = void>
struct TypedMapPropertyCRUDHandler2
{};

// CRUD = Create, Read, Update, Delete
template <typename T, typename = void>
struct TypedArrayPropertyCRUDHandler2
{};

// Default conversion version to be overloaded by custom types if need be.
template <typename T>
T	FromChars(std::string_view const& pChars);

// Base class for reflectable properties that have brackets operator to be able to make operations on the underlying array
template <typename T>
struct TypedMapPropertyCRUDHandler2<T,
	typename std::enable_if_t<HasMapSemantics_v<T>>> : MapPropertyCRUDHandler
{
	using KeyType = typename T::key_type;
	using ValueType = typename T::mapped_type;

	TypedMapPropertyCRUDHandler2()
	{
		Read = &MapRead;
		Update = &MapUpdate;
		Create = &MapCreate;
		Erase = &MapErase;
		Clear = &MapClear;
		Size = &MapSize;
		ValueHandler = &MapValueHandler;
		ValueReflectableID = &MapElementReflectableID;

#if USE_SERIALIZE_API
		KeyValueSerializer = &MapSerialize;
#endif
	}

	static_assert(has_MapErase_v<T>
		&& has_clear_v<T>
		&& has_size_v<T>,
		"In order to use Map-style reflection access, your type has to implement the following members with the same signature-style as std::map:"
		"a key_type typedef, a mapped_type typedef, operator[], begin, end, find, an std::pair-like iterator type, erase, clear, size.");

	static KeyType	GetKeyFromString(std::string_view const& pKey)
	{
		if constexpr (std::is_arithmetic_v<KeyType>)
		{
			KeyType result;
			if constexpr (std::is_same_v<KeyType, bool>)
			{
				// bool is an arithmetic type but from_chars doesn't support it so we need a special case to handle it
				result = (pKey == "true" || pKey == "1");
			}
			else
			{
				auto [_, error] { std::from_chars(pKey.data(), pKey.data() + pKey.size(), result) };
			}
			// TODO: check for errors
			return result;
		}
		else
		{
			KeyType key = FromChars<KeyType>(pKey);
			return key;
		}
	}

	static void const* MapRead(void const* pMap, std::string_view pKey)
	{
		T const* thisMap = static_cast<T const*>(pMap);
		if (thisMap == nullptr)
		{
			return nullptr;
		}

		KeyType const key = GetKeyFromString(pKey);
		auto it = thisMap->find(key);
		if (it == thisMap->end())
		{
			return nullptr;
		}
		return &it->second;
	}

	static void	MapUpdate(void* pMap, std::string_view pKey, void const* pNewData)
	{
		if (pMap == nullptr)
		{
			return;
		}

		ValueType* valuePtr = const_cast<ValueType*>((ValueType const*)MapRead(pMap, pKey));
		if (valuePtr == nullptr) // key not found
		{
			return;
		}

		auto* newValue = static_cast<ValueType const*>(pNewData);
		(*valuePtr) = *newValue;
	}

	static void	MapCreate(void* pMap, std::string_view pKey, void const* pInitData)
	{
		return MapUpdate(pMap, pKey, pInitData);
	}

	static void	MapErase(void* pMap, std::string_view pKey)
	{
		if (pMap != nullptr)
		{
			T* thisMap = static_cast<T*>(pMap);
			KeyType const key = GetKeyFromString(pKey);

			thisMap->erase(key);
		}
	}

	static void	MapClear(void* pMap)
	{
		if (pMap != nullptr)
		{
			T* thisMap = static_cast<T*>(pMap);
			thisMap->clear();
		}
	}

	static size_t	MapSize(void const* pMap)
	{
		if (pMap != nullptr)
		{
			T const* thisMap = static_cast<T const*>(pMap);
			return thisMap->size();
		}

		return 0;
	}

	static AbstractPropertyHandler MapValueHandler()
	{
		AbstractPropertyHandler handler;
		handler.ArrayHandler = nullptr;
		if constexpr (HasMapSemantics_v<ValueType>)
		{
			handler.MapHandler = &TypedMapPropertyCRUDHandler2<ValueType>::GetInstance();
		}
		else if constexpr(HasArraySemantics_v<ValueType>)
		{
			handler.ArrayHandler = &TypedArrayPropertyCRUDHandler2<ValueType>::GetInstance();
		}
		return handler;
	}

	static unsigned MapElementReflectableID() // TODO: ReflectableID
	{
		if constexpr (std::is_base_of_v<Reflectable2, ValueType>)
		{
			return ValueType::GetClassReflectableTypeInfo().ReflectableID;
		}
		else
		{
			return INVALID_REFLECTABLE_ID;
		}
	}

#if USE_SERIALIZE_API
	static void	MapSerialize(void const* pMap, REFLECTOR_SERIALIZER_TYPE& pSerializer)
	{
		if (pMap != nullptr)
		{
			T const* thisMap = static_cast<T const*>(pMap);
			AbstractPropertyHandler mapValueHandler = MapValueHandler();
			for (auto it = thisMap->begin(); it != thisMap->end(); ++it)
			{
				pSerializer.SerializeMapKeyValuePair(it->first, it->second, mapValueHandler);
			}
		}
	}
#endif

	static TypedMapPropertyCRUDHandler2 const& GetInstance()
	{
		static TypedMapPropertyCRUDHandler2 instance{};
		return instance;
	}
};



struct ArrayPropertyCRUDHandler
{
	using ArrayReadFptr = void const* (*)(void const* , size_t );
	using ArrayUpdateFptr = void (*)(void* , size_t , void const* );
	using ArrayCreateFptr = void (*)(void* , size_t , void const* );
	using ArrayEraseFptr = void	(*)(void* , size_t );
	using ArrayClearFptr = void	(*)(void* );
	using ArraySizeFptr = size_t(*)(void const*);
	using ArrayElementHandlerFptr = AbstractPropertyHandler (*)();
	using ArrayElementReflectableIDFptr = unsigned (*)();
#if USE_SERIALIZE_API
	using SerializeValueFptr = void(REFLECTOR_SERIALIZER_TYPE::*)(void const* pPropPtr, AbstractPropertyHandler const* pHandler);
#endif

	ArrayReadFptr	Read = nullptr;
	ArrayUpdateFptr Update = nullptr;
	ArrayCreateFptr	Create = nullptr;
	ArrayEraseFptr	Erase = nullptr;
	ArrayClearFptr	Clear = nullptr;
	ArraySizeFptr	Size = nullptr;
	ArrayElementHandlerFptr	ElementHandler = nullptr;
	ArrayElementReflectableIDFptr ElementReflectableID = nullptr;
#if USE_SERIALIZE_API
	SerializeValueFptr ElementSerializer = nullptr;
#endif

};

// Base class for reflectable properties that have brackets operator to be able to make operations on the underlying array
template <typename T>
struct TypedArrayPropertyCRUDHandler2<T,
	typename std::enable_if_t<!std::is_array_v<T> && has_Brackets_v<T>>> : ArrayPropertyCRUDHandler
{
	using ElementType = decltype(std::declval<T>()[0]);
	// Decaying removes the references and fixes the compile error "pointer to reference is illegal"
	using ElementValueType = std::decay_t<ElementType>;

	TypedArrayPropertyCRUDHandler2()
	{
		Read = &ArrayRead;
		Update = &ArrayUpdate;
		Create = &ArrayCreate;
		Erase = &ArrayErase;
		Clear = &ArrayClear;
		Size = &ArraySize;
		ElementHandler = &ArrayElementHandler;
		ElementReflectableID = &ArrayElementReflectableID;
#if USE_SERIALIZE_API
		ElementSerializer = &REFLECTOR_SERIALIZER_TYPE::template SerializeValue<ElementValueType>;
#endif
	}

	static_assert(has_Brackets_v<T>
		&& has_insert_v<T>
		&& has_erase_v<T>
		&& has_clear_v<T>
		&& has_size_v<T>,
		"In order to use Array-style reflection indexing, your type has to implement the following member functions with the same signature-style as std::vector:"
			" operator[], begin, insert(iterator, value), erase, clear, size, resize.");

	static void const* ArrayRead(void const* pArray, size_t pIndex)
	{
		if (pArray == nullptr)
		{
			return nullptr;
		}

		if (pIndex >= ArraySize(pArray))
		{
			ArrayCreate(const_cast<void*>(pArray), pIndex, nullptr);
		}

		T const* thisArray = static_cast<T const*>(pArray);
		return &(*thisArray)[pIndex];
	}

	static void	ArrayUpdate(void* pArray, size_t pIndex, void const* pNewData)
	{
		if (pArray == nullptr)
		{
			return;
		}

		T* thisArray = static_cast<T*>(pArray);
		if (pIndex >= ArraySize(pArray))
		{
			thisArray->resize(pIndex + 1);
		}

		ElementValueType const* actualData = static_cast<ElementValueType const*>(pNewData);
		(*thisArray)[pIndex] = actualData ? *actualData : ElementValueType();
	}

	static void	ArrayCreate(void* pArray, size_t pAtIndex, void const* pInitData)
	{
		return ArrayUpdate(pArray, pAtIndex, pInitData);
	}

	static void	ArrayErase(void* pArray, size_t pAtIndex)
	{
		if (pArray != nullptr)
		{
			T* thisArray = static_cast<T*>(pArray);
			thisArray->erase(thisArray->begin() + pAtIndex);
		}
	}

	static void	ArrayClear(void* pArray)
	{
		if (pArray != nullptr)
		{
			T* thisArray = static_cast<T*>(pArray);
			thisArray->clear();
		}
	}

	static size_t	ArraySize(void const* pArray)
	{
		if (pArray != nullptr)
		{
			T const* thisArray = static_cast<T const*>(pArray);
			return thisArray->size();
		}

		return 0;
	}

	static AbstractPropertyHandler	ArrayElementHandler()
	{
		AbstractPropertyHandler handler;
		handler.ArrayHandler = nullptr;
		if constexpr (HasMapSemantics_v<ElementValueType>)
		{
			handler.MapHandler = &TypedMapPropertyCRUDHandler2<ElementValueType>::GetInstance();
		}
		else if constexpr (HasArraySemantics_v<ElementValueType>)
		{
			handler.ArrayHandler = &TypedArrayPropertyCRUDHandler2<ElementValueType>::GetInstance();
		}
		return handler;
	}

	static unsigned ArrayElementReflectableID() // TODO: ReflectableID
	{
		if constexpr (std::is_base_of_v<Reflectable2, T>)
		{
			return T::GetClassReflectableTypeInfo().ReflectableID;
		}
		else
		{
			return INVALID_REFLECTABLE_ID;
		}
	}

	static TypedArrayPropertyCRUDHandler2 const&	GetInstance()
	{
		static TypedArrayPropertyCRUDHandler2 instance{};
		return instance;
	}
};

// Base class for reflectable properties that are C-arrays to be able to make operations on the underlying array
template <typename T>
struct TypedArrayPropertyCRUDHandler2<T,
	typename std::enable_if_t<std::is_array_v<T>>> : ArrayPropertyCRUDHandler
{
	using ElementType = decltype(std::declval<T>()[0]);
	// remove_reference_t removes the references and fixes the compile error "pointer to reference is illegal"
	using ElementValueType = std::remove_reference_t<ElementType>;

	inline static const size_t ARRAY_SIZE = std::extent_v<T>;

	TypedArrayPropertyCRUDHandler2()
	{
		Read = &ArrayRead;
		Update = &ArrayUpdate;
		Create = &ArrayCreate;
		Erase = &ArrayErase;
		Clear = &ArrayClear;
		Size = &ArraySize;
		ElementHandler = &ArrayElementHandler;
		ElementReflectableID = &ArrayElementReflectableID;
#if USE_SERIALIZE_API
		ElementSerializer = &REFLECTOR_SERIALIZER_TYPE::template SerializeValue<ElementValueType>;
#endif
	}

	static void const* ArrayRead(void const* pArray, size_t pIndex)
	{
		if (pArray == nullptr)
		{
			return nullptr;
		}
		T const* thisArray = static_cast<T const*>(pArray);
		return &(*thisArray)[pIndex];
	}

	static void	ArrayUpdate(void* pArray, size_t pIndex, void const* pNewData)
	{
		if (pArray == nullptr || pNewData == nullptr)
		{
			return;
		}

		// Check if the element is assignable to make arrays of arrays work
		if constexpr (std::is_assignable_v<ElementValueType, ElementValueType>)
		{
			T* thisArray = static_cast<T*>(pArray);
			ElementValueType const* actualData = static_cast<ElementValueType const*>(pNewData);
			(*thisArray)[pIndex] = *actualData;
		}
	}

	static void	ArrayCreate(void* pArray, size_t pAtIndex, void const* pInitData)
	{
		if (pArray == nullptr)
		{
			return;
		}

		// TODO: throw exception if exceptions are enabled
		assert(pAtIndex < ARRAY_SIZE);

		// Check if the element is assignable to make arrays of arrays work
		if constexpr (std::is_assignable_v<ElementValueType, ElementValueType>)
		{
			T* thisArray = static_cast<T*>(pArray);
			ElementValueType const* actualInitData = (pInitData ? static_cast<ElementValueType const*>(pInitData) : nullptr);
			(*thisArray)[pAtIndex] = actualInitData ? *actualInitData : ElementValueType();
		}
	}

	static void	ArrayErase(void* pArray, size_t pAtIndex)
	{
		if (pArray == nullptr || pAtIndex >= ARRAY_SIZE)
		{
			return;
		}

		// Check if the element is assignable to make arrays of arrays work
		if constexpr (std::is_assignable_v<ElementValueType, ElementValueType>)
		{
			T* thisArray = static_cast<T*>(pArray);
			(*thisArray)[pAtIndex] = ElementValueType();
		}
	}

	static void	ArrayClear(void* pArray)
	{
		if (pArray == nullptr)
		{
			return;
		}

		// I don't know if we can do something smarter in this case...
		// Check if the element is assignable to make arrays of arrays work
		if constexpr (std::is_assignable_v<ElementValueType, ElementValueType>)
		{
			T* thisArray = static_cast<T*>(pArray);
			for (auto i = 0u; i < ARRAY_SIZE; ++i)
			{
				(*thisArray)[i] = ElementValueType();
			}
		}
	}

	static size_t	ArraySize(void const* /*pArray*/)
	{
		return ARRAY_SIZE;
	}

	// TODO: Refactor because there is code duplication between the two handler types
	static AbstractPropertyHandler	ArrayElementHandler()
	{
		AbstractPropertyHandler handler;
		handler.ArrayHandler = nullptr;
		if constexpr (HasMapSemantics_v<ElementValueType>)
		{
			handler.MapHandler = &TypedMapPropertyCRUDHandler2<ElementValueType>::GetInstance();
		}
		else if constexpr (HasArraySemantics_v<ElementValueType>)
		{
			handler.ArrayHandler = &TypedArrayPropertyCRUDHandler2<ElementValueType>::GetInstance();
		}
		return handler;
	}

	static unsigned ArrayElementReflectableID() // TODO: ReflectableID
	{
		if constexpr (std::is_base_of_v<Reflectable2, ElementValueType>)
		{
			return ElementValueType::GetClassReflectableTypeInfo().ReflectableID;
		}
		else
		{
			return INVALID_REFLECTABLE_ID;
		}
	}

	static TypedArrayPropertyCRUDHandler2 const& GetInstance()
	{
		static TypedArrayPropertyCRUDHandler2 instance{};
		return instance;
	}
};

template <typename T, typename TProp>
struct ReflectProperty3 : TypeInfo2
{
	ReflectProperty3(const char* pName, std::ptrdiff_t pOffset) :
		TypeInfo2(pName, pOffset, sizeof(TProp))
	{
		if constexpr (HasArraySemantics_v<TProp>)
		{
			SetType(Type::Array);
		}
		else if constexpr (HasMapSemantics_v<TProp>)
		{
			SetType(Type::Map);
		}
		else if constexpr (std::is_class_v<TProp>)
		{
			SetType(Type::Object);
		}
		// TODO: that doesnt manage custom enums
		else
		{
			// primary type
			SetType(FromActualTypeToEnumType<TProp>::EnumType);
		}

		ReflectableTypeInfo& typeInfo = T::EditClassReflectableTypeInfo(); // TODO: maybe just give it as a parameter to the function
		typeInfo.PushTypeInfo(*this);

		if constexpr (std::is_base_of_v<Reflectable2, TProp>)
		{
			reflectableID = TProp::GetClassReflectableTypeInfo().ReflectableID;
		}
		else
		{
			reflectableID = INVALID_REFLECTABLE_ID;
		}

		if constexpr (HasMapSemantics_v<TProp>)
		{
			DataStructurePropertyHandler.MapHandler = &TypedMapPropertyCRUDHandler2<TProp>::GetInstance();
		}
		else if constexpr (HasArraySemantics_v<TProp>)
		{
			DataStructurePropertyHandler.ArrayHandler = &TypedArrayPropertyCRUDHandler2<TProp>::GetInstance();
		}

		if constexpr (std::is_trivially_copyable_v<TProp>)
		{
			CopyCtor = [](void* pDestAddr, void const* pOther, size_t pOffset)
			{
				memcpy((std::byte*)pDestAddr + pOffset, (std::byte const*)pOther + pOffset, sizeof(TProp));
			};
		}
		else if constexpr (!std::is_trivially_copy_assignable_v<TProp>)
		{
			CopyCtor = [](void* pDestAddr, void const* pOther, size_t pOffset)
			{
				TProp* actualDestination = (TProp*)((std::byte*)pDestAddr + pOffset);
				TProp const* actualOther = (TProp const*)((std::byte const*)pOther + pOffset);
				(*actualDestination) = *actualOther;
			};
		}

#if USE_SERIALIZE_API
		SetupSerializerFunction();
#endif
	}

#if USE_SERIALIZE_API
	void	SetupSerializerFunction();
#endif
};


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
	inline static FunctionTypeInfo<decltype(&Name)> Name##_TYPEINFO{&Name, STRINGIZE(Name)}

#define MOE_METHOD(RetType, Name, ...) \
	RetType Name(GLUE_ARGUMENT_PAIRS(__VA_ARGS__)); \
	inline static FunctionTypeInfo<decltype(&Name)> Name##_TYPEINFO{&Name, STRINGIZE(Name)}

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

	[[nodiscard]] ClassID	GetReflectableClassID() const
	{
		return myReflectableClassID;
	}

	template <typename TRefl>
	[[nodiscard]] bool	IsA() const
	{
		static_assert(std::is_base_of_v<Reflectable2, TRefl>, "IsA only works with Reflectable-derived class types.");

		// Special case to manage IsA<Reflectable>, because since all Reflectable are derived from it *implicitly*, IsParentOf would return false.
		if constexpr (std::is_same_v<TRefl, Reflectable2>)
		{
			return true;
		}

		ReflectableTypeInfo const& parentTypeInfo = TRefl::GetClassReflectableTypeInfo();
		return (parentTypeInfo.ReflectableID == myReflectableClassID || parentTypeInfo.IsParentOf(myReflectableClassID));
	}

	template <typename T = Reflectable2>
	T*	Clone()
	{
		static_assert(std::is_base_of_v<Reflectable2, T>, "Clone only works with Reflectable-derived class types.");

		ReflectableTypeInfo const* thisTypeInfo = Reflector3::GetSingleton().GetTypeInfo(myReflectableClassID);
		if (thisTypeInfo == nullptr)
		{
			return nullptr;
		}

		Reflectable2* clone = Reflector3::GetSingleton().TryInstantiate(myReflectableClassID, {});
		if (clone == nullptr)
		{
			return nullptr;
		}

		auto const& parents = thisTypeInfo->ParentClasses;
		for (ReflectableTypeInfo const* parent : parents)
		{
			if (parent == nullptr)
				continue;

			CloneProperties(this, parent, clone);
		}

		CloneProperties(this, thisTypeInfo, clone);

		return (T*)clone;
	}

	void	CloneProperties(Reflectable2 const* pCloned, ReflectableTypeInfo const* pClonedTypeInfo, Reflectable2* pClone)
	{
		for (TypeInfo2 const& prop : pClonedTypeInfo->Properties)
		{
			auto* propPtr = pCloned->GetProperty<void const*>(prop.name);
			if (propPtr != nullptr)
			{
				auto actualOffset = ((std::byte const*)propPtr - (std::byte const*)pCloned); // accounting for vptr etc.
				if (prop.CopyCtor != nullptr)
				{
					prop.CopyCtor(pClone, pCloned, actualOffset);
				}
			}
		}
	}


	[[nodiscard]] IntrusiveLinkedList<TypeInfo2> const& GetProperties() const
	{
		return Reflector3::GetSingleton().GetTypeInfo(myReflectableClassID)->Properties;
	}

	[[nodiscard]] IntrusiveLinkedList<FunctionInfo> const& GetMemberFunctions() const
	{
		return Reflector3::GetSingleton().GetTypeInfo(myReflectableClassID)->MemberFunctions;
	}

	template <typename T>
	T const*	GetMemberAtOffset(ptrdiff_t pOffset) const
	{
		return reinterpret_cast<T const*>(reinterpret_cast<std::byte const*>(this) + pOffset);
	}

	template <typename T>
	T* EditMemberAtOffset(ptrdiff_t pOffset)
	{
		return reinterpret_cast<T*>(reinterpret_cast<std::byte*>(this) + pOffset);
	}

	template <typename TProp = void>
	[[nodiscard]] TProp const* GetProperty(std::string_view pName) const
	{
		ReflectableTypeInfo const* thisTypeInfo = Reflector3::GetSingleton().GetTypeInfo(myReflectableClassID);

		// Account for the vtable pointer offset in case our type is polymorphic (aka virtual)
		std::byte const* propertyAddr = reinterpret_cast<std::byte const*>(this) - thisTypeInfo->VirtualOffset;

		// Test for either array, map or compound access and branch into the one seen first
		// compound property syntax uses the standard "." accessor
		// array and map property syntax uses the standard "[]" array subscript operator
		size_t const dotPos = pName.find('.');
		size_t const leftBrackPos = pName.find('[');
		if (dotPos != pName.npos && dotPos < leftBrackPos) // it's a compound
		{
			std::string_view compoundPropName = std::string_view(pName.data(), dotPos);
			return GetCompoundProperty<TProp>(thisTypeInfo, compoundPropName, pName, propertyAddr);
		}
		else if (leftBrackPos != pName.npos && leftBrackPos < dotPos) // it's an array or map
		{
			size_t const rightBrackPos = pName.find(']');
			// If there is a mismatched bracket, or the closing bracket is before the opening one,
			// or there is nothing between the two brackets, the expression has to be ill-formed!
			if (rightBrackPos == pName.npos || rightBrackPos < leftBrackPos || rightBrackPos == leftBrackPos + 1)
			{
				return nullptr;
			}

			std::string_view propName = std::string_view(pName.data(), leftBrackPos);

			if (TypeInfo2 const* thisProp = thisTypeInfo->FindPropertyInHierarchy(propName))
			{
				if (thisProp->type == Type::Array)
				{
					int const propIndex = atoi(pName.data() + leftBrackPos + 1); // TODO: use own atoi to avoid multi-k-LOC dependency!
					pName.remove_prefix(rightBrackPos + 1);
					return GetArrayProperty<TProp>(thisTypeInfo, propName, pName, propIndex, propertyAddr);
				}
				else if (thisProp->type == Type::Map)
				{
					auto const keyStartPos = leftBrackPos + 1;
					std::string_view key(pName.data() + keyStartPos, rightBrackPos - keyStartPos);
					pName.remove_prefix(rightBrackPos + 1);
					return GetArrayMapProperty<TProp>(thisTypeInfo, propName, pName, key, propertyAddr);
				}
			}
			return nullptr; // Didn't find the property or it's of a type that we don't know how to handle in this context.
		}

		// Otherwise: search for a "normal" property
		for (TypeInfo2 const& prop : thisTypeInfo->Properties)
		{
			if (prop.name == pName)
			{
				return reinterpret_cast<TProp const*>(propertyAddr + prop.offset);
			}
		}

		// Maybe in the parent classes?
		auto [parentClass, parentProperty] = thisTypeInfo->FindParentClassProperty(pName);
		if (parentClass != nullptr && parentProperty != nullptr) // A parent property was found
		{
			return reinterpret_cast<TProp const*>(propertyAddr + parentProperty->offset);
		}

		return nullptr;
	}

	[[nodiscard]] bool EraseProperty(std::string_view pName)
	{
		ReflectableTypeInfo const* thisTypeInfo = Reflector3::GetSingleton().GetTypeInfo(myReflectableClassID);

		return ErasePropertyImpl(thisTypeInfo, pName);
	}


	[[nodiscard]] bool ErasePropertyImpl(ReflectableTypeInfo const* pTypeInfo, std::string_view pName)
	{
		// Account for the vtable pointer offset in case our type is polymorphic (aka virtual)
		std::byte* propertyAddr = reinterpret_cast<std::byte*>(this) - pTypeInfo->VirtualOffset;

		// Test for either array or compound access and branch into the one seen first
		// compound property syntax uses the standard "." accessor
		// array property syntax uses the standard "[]" array subscript operator
		size_t const dotPos = pName.find('.');
		size_t const leftBrackPos = pName.find('[');
		if (dotPos != pName.npos && dotPos < leftBrackPos) // it's a compound
		{
			std::string_view compoundPropName = std::string_view(pName.data(), dotPos);
			return EraseInCompoundProperty(pTypeInfo, compoundPropName, pName, propertyAddr);
		}
		else if (leftBrackPos != pName.npos && leftBrackPos < dotPos) // it's an array
		{
			size_t const rightBrackPos = pName.find(']');
			// If there is a mismatched bracket, or the closing bracket is before the opening one,
			// or there is no number between the two brackets, the expression has to be ill-formed!
			if (rightBrackPos == pName.npos || rightBrackPos < leftBrackPos || rightBrackPos == leftBrackPos + 1)
			{
				return false;
			}
			int propIndex = atoi(pName.data() + leftBrackPos + 1); // TODO: use own atoi to avoid multi-k-LOC dependency!
			std::string_view propName = std::string_view(pName.data(), leftBrackPos);
			pName.remove_prefix(rightBrackPos + 1);
			return EraseArrayProperty(pTypeInfo, propName, pName, propIndex, propertyAddr);
		}

		// Otherwise: maybe in the parent classes?
		for (ReflectableTypeInfo const* parentClass : pTypeInfo->ParentClasses)
		{
			if (ErasePropertyImpl(parentClass, pName))
			{
				return true;
			}
		}

		return false;
	}

	[[nodiscard]] bool EraseArrayProperty(ReflectableTypeInfo const* pTypeInfoOwner, std::string_view pName, std::string_view pRemainingPath, int pArrayIdx, std::byte* pPropPtr)
	{
		// First, find our array property
		TypeInfo2 const* thisProp = pTypeInfoOwner->FindPropertyInHierarchy(pName);
		if (thisProp == nullptr)
		{
			return false; // There was no property with the given name.
		}

		pPropPtr += thisProp->offset;
		ArrayPropertyCRUDHandler const* arrayHandler = thisProp->GetArrayHandler();

		return RecurseEraseArrayProperty(arrayHandler, pName, pRemainingPath, pArrayIdx, pPropPtr);
	}

	[[nodiscard]] bool RecurseEraseArrayProperty(ArrayPropertyCRUDHandler const* pArrayHandler, std::string_view pName, std::string_view pRemainingPath, int pArrayIdx, std::byte* pPropPtr)
	{
		if (pArrayHandler == nullptr || pArrayIdx >= pArrayHandler->Size(pPropPtr))
		{
			return false; // not an array or index is out-of-bounds
		}

		if (pRemainingPath.empty()) // this was what we were looking for : return
		{
			pArrayHandler->Erase(pPropPtr, pArrayIdx);
			return true;
		}

		// There is more to eat: find out if we're looking for an array in an array or a compound
		size_t dotPos = pRemainingPath.find('.');
		size_t const leftBrackPos = pRemainingPath.find('[');
		if (dotPos == pRemainingPath.npos && leftBrackPos == pRemainingPath.npos)
		{
			return false; // it's not an array neither a compound? I don't know how to read it!
		}
		if (dotPos != pRemainingPath.npos && dotPos < leftBrackPos) // it's a compound in an array
		{
			ReflectableTypeInfo const* arrayElemTypeInfo = Reflector3::GetSingleton().GetTypeInfo(pArrayHandler->ElementReflectableID());
			if (arrayElemTypeInfo == nullptr) // compound type that is not Reflectable...
			{
				return false;
			}
			pRemainingPath.remove_prefix(1); // strip the leading dot
			// Lookup the next dot (if any) to know if we should search for a compound or an array.
			dotPos = pRemainingPath.find('.');
			// Use leftBrackPos-1 because it was computed before stripping the leading dot, so it is off by 1
			auto suffixPos = (dotPos != pRemainingPath.npos || leftBrackPos != pRemainingPath.npos ? std::min(dotPos, leftBrackPos - 1) : 0);
			pName = std::string_view(pRemainingPath.data() + dotPos + 1, pRemainingPath.length() - (dotPos + 1));
			dotPos = pName.find('.');
			if (dotPos < leftBrackPos) // next entry is a compound
				return EraseInCompoundProperty(arrayElemTypeInfo, pName, pRemainingPath, pPropPtr);
			else // next entry is an array
			{
				size_t const rightBrackPos = pRemainingPath.find(']');
				if (rightBrackPos == pRemainingPath.npos || rightBrackPos < leftBrackPos || rightBrackPos == leftBrackPos + 2) //+2 because leftBrackPos is off by 1
				{
					return false;
				}
				int propIndex = atoi(pRemainingPath.data() + leftBrackPos); // TODO: use own atoi to avoid multi-k-LOC dependency!
				pName = std::string_view(pRemainingPath.data(), pRemainingPath.length() - suffixPos + 1);
				pRemainingPath.remove_prefix(rightBrackPos + 1);
				return EraseArrayProperty(arrayElemTypeInfo, pName, pRemainingPath, propIndex, pPropPtr);
			}
		}
		else if (leftBrackPos != pRemainingPath.npos && leftBrackPos < dotPos) // it's an array in an array
		{
			size_t const rightBrackPos = pRemainingPath.find(']');
			// If there is a mismatched bracket, or the closing bracket is before the opening one,
			// or there is no number between the two brackets, the expression has to be ill-formed!
			if (rightBrackPos == pRemainingPath.npos || rightBrackPos < leftBrackPos || rightBrackPos == leftBrackPos + 1)
			{
				return false;
			}

			ArrayPropertyCRUDHandler const* elementHandler = pArrayHandler->ElementHandler().ArrayHandler;

			if (elementHandler == nullptr)
			{
				return false; // Tried to access a property type that is not an array or not a Reflectable
			}

			int propIndex = atoi(pRemainingPath.data() + leftBrackPos + 1); // TODO: use own atoi to avoid multi-k-LOC dependency!
			pRemainingPath.remove_prefix(rightBrackPos + 1);
			return RecurseEraseArrayProperty(elementHandler, pName, pRemainingPath, propIndex, pPropPtr);
		}

		assert(false);
		return false; // Never supposed to arrive here!
	}

	template <typename TProp>
	[[nodiscard]] TProp const* GetArrayProperty(ReflectableTypeInfo const* pTypeInfoOwner, std::string_view pName, std::string_view pRemainingPath, int pArrayIdx, std::byte const* pPropPtr) const
	{
		// First, find our array property
		TypeInfo2 const* thisProp = pTypeInfoOwner->FindPropertyInHierarchy(pName);
		if (thisProp == nullptr)
		{
			return nullptr; // There was no property with the given name.
		}

		pPropPtr += thisProp->offset;
		ArrayPropertyCRUDHandler const* arrayHandler = thisProp->GetArrayHandler();

		return RecurseFindArrayProperty<TProp>(arrayHandler, pName, pRemainingPath, pArrayIdx, pPropPtr);
	}


	template <typename TProp>
	[[nodiscard]] TProp const* RecurseFindArrayProperty(ArrayPropertyCRUDHandler const* pArrayHandler, std::string_view pName, std::string_view pRemainingPath, int pArrayIdx, std::byte const* pPropPtr) const
	{
		if (pArrayHandler == nullptr)
		{
			return nullptr; // not an array or index is out-of-bounds
		}

		pPropPtr = (std::byte const*)pArrayHandler->Read(pPropPtr, pArrayIdx);
		if (pRemainingPath.empty()) // this was what we were looking for : return
		{
			return reinterpret_cast<TProp const*>(pPropPtr);
		}

		// There is more to eat: find out if we're looking for an array in an array or a compound
		size_t dotPos = pRemainingPath.find('.');
		size_t const leftBrackPos = pRemainingPath.find('[');
		if (dotPos == pRemainingPath.npos && leftBrackPos == pRemainingPath.npos)
		{
			return nullptr; // it's not an array neither a compound? I don't know how to read it!
		}
		if (dotPos != pRemainingPath.npos && dotPos < leftBrackPos) // it's a compound in an array
		{
			ReflectableTypeInfo const* arrayElemTypeInfo = Reflector3::GetSingleton().GetTypeInfo(pArrayHandler->ElementReflectableID());
			if (arrayElemTypeInfo == nullptr) // compound type that is not Reflectable...
			{
				return nullptr;
			}
			pRemainingPath.remove_prefix(1); // strip the leading dot
			// Lookup the next dot (if any) to know if we should search for a compound or an array.
			dotPos = pRemainingPath.find('.');
			// Use leftBrackPos-1 because it was computed before stripping the leading dot, so it is off by 1
			auto suffixPos = (dotPos != pRemainingPath.npos || leftBrackPos != pRemainingPath.npos ? std::min(dotPos, leftBrackPos - 1) : 0);
			pName = std::string_view(pRemainingPath.data() + dotPos + 1, pRemainingPath.length() - (dotPos + 1));
			dotPos = pName.find('.');
			if (dotPos < leftBrackPos) // next entry is a compound
				return GetCompoundProperty<TProp>(arrayElemTypeInfo, pName, pRemainingPath, pPropPtr);
			else // next entry is an array
			{
				size_t const rightBrackPos = pRemainingPath.find(']');
				if (rightBrackPos == pRemainingPath.npos || rightBrackPos < leftBrackPos || rightBrackPos == leftBrackPos + 2) //+2 because leftBrackPos is off by 1
				{
					return nullptr;
				}
				int propIndex = atoi(pRemainingPath.data() + leftBrackPos); // TODO: use own atoi to avoid multi-k-LOC dependency!
				pName = std::string_view(pRemainingPath.data(), pRemainingPath.length() - suffixPos + 1);
				pRemainingPath.remove_prefix(rightBrackPos + 1);
				return GetArrayProperty<TProp>(arrayElemTypeInfo, pName, pRemainingPath, propIndex, pPropPtr);
			}
		}
		else if (leftBrackPos != pRemainingPath.npos && leftBrackPos < dotPos) // it's an array in an array
		{
			size_t const rightBrackPos = pRemainingPath.find(']');
			// If there is a mismatched bracket, or the closing bracket is before the opening one,
			// or there is no number between the two brackets, the expression has to be ill-formed!
			if (rightBrackPos == pRemainingPath.npos || rightBrackPos < leftBrackPos || rightBrackPos == leftBrackPos + 1)
			{
				return nullptr;
			}

			ArrayPropertyCRUDHandler const* elementHandler = pArrayHandler->ElementHandler().ArrayHandler;

			if (elementHandler == nullptr)
			{
				return nullptr; // Tried to access a property type that is not an array or not a Reflectable
			}

			int propIndex = atoi(pRemainingPath.data() + leftBrackPos + 1); // TODO: use own atoi to avoid multi-k-LOC dependency!
			pRemainingPath.remove_prefix(rightBrackPos + 1);
			return RecurseFindArrayProperty<TProp>(elementHandler, pName, pRemainingPath, propIndex, pPropPtr);
		}

		assert(false);
		return nullptr; // Never supposed to arrive here!
	}

	template <typename TProp>
	[[nodiscard]] TProp const* RecurseArrayMapProperty(AbstractPropertyHandler const& pDataStructureHandler, std::byte const* pPropPtr, std::string_view pRemainingPath, std::string_view pKey) const
	{
		// Find out the type of handler we are interested in
		void const* value = nullptr;
		if (pDataStructureHandler.ArrayHandler)
		{
			size_t index;
			std::from_chars(pKey.data(), pKey.data() + pKey.size(), index); // TODO: check for errors
			value = pDataStructureHandler.ArrayHandler->Read(pPropPtr, index);
		}
		else if (pDataStructureHandler.MapHandler)
		{
			value = pDataStructureHandler.MapHandler->Read(pPropPtr, pKey);
		}

		if (value == nullptr)
		{
			return nullptr; // there was no explorable data structure found: probably a syntax error
		}

		if (pRemainingPath.empty()) // this was what we were looking for : return
		{
			return static_cast<TProp const*>(value);
		}

		// there is more: we have to find if it's a compound, an array or a map...
		auto* valueAsBytes = static_cast<std::byte const*>(value);

		if (pRemainingPath[0] == '.') // it's a nested structure
		{
			unsigned valueID = pDataStructureHandler.ArrayHandler ? pDataStructureHandler.ArrayHandler->ElementReflectableID() : pDataStructureHandler.MapHandler->ValueReflectableID();
			ReflectableTypeInfo const* valueTypeInfo = Reflector3::GetSingleton().GetTypeInfo(valueID);
			auto nextDelimiterPos = pRemainingPath.find_first_of(".[", 1);
			std::string_view propName = pRemainingPath.substr(1, nextDelimiterPos == pRemainingPath.npos ? nextDelimiterPos : nextDelimiterPos - 1);
			if (nextDelimiterPos == pRemainingPath.npos)
			{
				pRemainingPath.remove_prefix(1);
				Reflectable2 const* reflectable = reinterpret_cast<Reflectable2 const*>(valueAsBytes);
				return reflectable->GetProperty<TProp>(pRemainingPath);
			}
			if (pRemainingPath[nextDelimiterPos] == '.')
			{
				pRemainingPath.remove_prefix(1);
				return GetCompoundProperty<TProp>(valueTypeInfo, propName, pRemainingPath, valueAsBytes);
			}
			else
			{
				size_t rightBracketPos = pRemainingPath.find(']', 1);
				if (rightBracketPos == pRemainingPath.npos || rightBracketPos == 1)
				{
					return nullptr; // syntax error: no right bracket or nothing between the brackets
				}
				pKey = pRemainingPath.substr(nextDelimiterPos + 1, rightBracketPos - nextDelimiterPos - 1);
				pRemainingPath.remove_prefix(rightBracketPos + 1);

				return GetArrayMapProperty<TProp>(valueTypeInfo, propName, pRemainingPath, pKey, valueAsBytes);
			}

		}

		if (pRemainingPath[0] == '[')
		{
			size_t rightBracketPos = pRemainingPath.find(']', 1);
			if (rightBracketPos == pRemainingPath.npos || rightBracketPos == 1)
			{
				return nullptr; // syntax error: no right bracket or nothing between the brackets
			}
			pKey = pRemainingPath.substr(1, rightBracketPos - 1);
			pRemainingPath.remove_prefix(rightBracketPos + 1);

			if (pDataStructureHandler.ArrayHandler)
			{
				return RecurseArrayMapProperty<TProp>(pDataStructureHandler.ArrayHandler->ElementHandler(), valueAsBytes, pRemainingPath, pKey);
			}
			else if (pDataStructureHandler.MapHandler)
			{
				return RecurseArrayMapProperty<TProp>(pDataStructureHandler.MapHandler->ValueHandler(), valueAsBytes, pRemainingPath, pKey);
			}
		}

		return nullptr;
	}

	template <typename TProp>
	[[nodiscard]] TProp const* GetArrayMapProperty(ReflectableTypeInfo const* pTypeInfoOwner, std::string_view pName, std::string_view pRemainingPath, std::string_view pKey, std::byte const* pPropPtr) const
	{
		// First, find our array property
		TypeInfo2 const* thisProp = pTypeInfoOwner->FindPropertyInHierarchy(pName);
		if (thisProp == nullptr)
		{
			return nullptr; // There was no property with the given name.
		}

		pPropPtr += thisProp->offset;
		// Find out the type of handler we are interested in
		AbstractPropertyHandler const& dataStructureHandler = thisProp->GetDataStructureHandler();
		return RecurseArrayMapProperty<TProp>(dataStructureHandler, pPropPtr, pRemainingPath, pKey);
	}

	// TODO: I think pTotalOffset can drop the reference
	template <typename TProp>
	[[nodiscard]] TProp const*	GetCompoundProperty(ReflectableTypeInfo const* pTypeInfoOwner, std::string_view pName, std::string_view pFullPath, std::byte const* propertyAddr) const
	{
		// First, find our compound property
		TypeInfo2 const* thisProp = pTypeInfoOwner->FindPropertyInHierarchy(pName);
		if (thisProp == nullptr)
		{
			return nullptr; // There was no property with the given name.
		}

		// We found our compound property: consume its name from the "full path"
		// +1 to also remove the '.', except if we are at the last nested property.
		pFullPath.remove_prefix(std::min(pName.size() + 1, pFullPath.size()));

		// Now, add the offset of the compound to the total offset
		propertyAddr += (int)thisProp->offset;

		// Do we need to go deeper?
		if (pFullPath.empty())
		{
			// No: just return the variable at the current offset
			return reinterpret_cast<TProp const*>(propertyAddr);
		}

		// Yes: recurse one more time, this time using this property's type info (needs to be Reflectable)
		ReflectableTypeInfo const* nestedTypeInfo = Reflector3::GetSingleton().GetTypeInfo(thisProp->reflectableID);
		if (nestedTypeInfo != nullptr)
		{
			pTypeInfoOwner = nestedTypeInfo;
		}

		// Update the next property's name to search for it.
		// Check if we are looking for an array in the compound...
		size_t const nextDelimiterPos = pFullPath.find_first_of(".[");

		pName = pFullPath.substr(0, nextDelimiterPos);

		if (nextDelimiterPos != pFullPath.npos && pFullPath[nextDelimiterPos] == '[') // it's an array
		{
			size_t const rightBrackPos = pFullPath.find(']');
			// If there is a mismatched bracket, or the closing bracket is before the opening one,
			// or there is no number between the two brackets, the expression has to be ill-formed!
			if (rightBrackPos == pFullPath.npos || rightBrackPos < nextDelimiterPos || rightBrackPos == nextDelimiterPos + 1)
			{
				return nullptr;
			}
			pName = std::string_view(pFullPath.data(), nextDelimiterPos);
			std::string_view key = pFullPath.substr(nextDelimiterPos + 1, rightBrackPos - nextDelimiterPos - 1);
			pFullPath.remove_prefix(rightBrackPos + 1);
			return GetArrayMapProperty<TProp>(pTypeInfoOwner, pName, pFullPath, key, propertyAddr);
		}

		return GetCompoundProperty<TProp>(pTypeInfoOwner, pName, pFullPath, propertyAddr);

	}

	// TODO: I think pTotalOffset can drop the reference
	[[nodiscard]] bool EraseInCompoundProperty(ReflectableTypeInfo const* pTypeInfoOwner, std::string_view pName, std::string_view pFullPath, std::byte* propertyAddr)
	{
		// First, find our compound property
		TypeInfo2 const* thisProp = pTypeInfoOwner->FindPropertyInHierarchy(pName);
		if (thisProp == nullptr)
		{
			return false; // There was no property with the given name.
		}

		// We found our compound property: consume its name from the "full path"
		// +1 to also remove the '.', except if we are at the last nested property.
		pFullPath.remove_prefix(std::min(pName.size() + 1, pFullPath.size()));

		// Now, add the offset of the compound to the total offset
		propertyAddr += (int)thisProp->offset;

		// Can we go deeper?
		if (pFullPath.empty())
		{
			// No: we were unable to find the target array property
			return false;
		}

		// Yes: recurse one more time, this time using this property's type info (needs to be Reflectable)
		pTypeInfoOwner = Reflector3::GetSingleton().GetTypeInfo(thisProp->reflectableID);
		if (pTypeInfoOwner == nullptr)
		{
			return false; // This type isn't reflectable? Nothing I can do for you...
		}

		// Update the next property's name to search for it.
		size_t const dotPos = pFullPath.find('.');
		if (dotPos != pFullPath.npos)
		{
			pName = std::string_view(pFullPath.data(), dotPos);
		}
		else
		{
			pName = pFullPath;
		}

		// Check if we are looking for an array in the compound...
		size_t const leftBrackPos = pName.find('[');
		if (leftBrackPos != pName.npos) // it's an array
		{
			size_t const rightBrackPos = pName.find(']');
			// If there is a mismatched bracket, or the closing bracket is before the opening one,
			// or there is no number between the two brackets, the expression has to be ill-formed!
			if (rightBrackPos == pName.npos || rightBrackPos < leftBrackPos || rightBrackPos == leftBrackPos + 1)
			{
				return false;
			}
			int propIndex = atoi(pName.data() + leftBrackPos + 1); // TODO: use own atoi to avoid multi-k-LOC dependency!
			std::string_view propName = std::string_view(pName.data(), leftBrackPos);
			pFullPath.remove_prefix(rightBrackPos + 1);
			return EraseArrayProperty(pTypeInfoOwner, propName, pFullPath, propIndex, propertyAddr);
		}
		else
		{
			return EraseInCompoundProperty(pTypeInfoOwner, pName, pFullPath, propertyAddr);
		}
	}

	/* Version that returns a reference for when you are 100% confident this property exists */
	template <typename TProp>
	[[nodiscard]] TProp const& GetSafeProperty(std::string_view pName) const
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
	bool SetProperty(std::string_view pName, TProp&& pSetValue)
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

	[[nodiscard]] FunctionInfo const* GetFunction(std::string_view pMemberFuncName) const
	{
		ReflectableTypeInfo const* thisTypeInfo = Reflector3::GetSingleton().GetTypeInfo(myReflectableClassID);
		for (FunctionInfo const& aFuncInfo : thisTypeInfo->MemberFunctions)
		{
			if (aFuncInfo.name == pMemberFuncName)
			{
				return &aFuncInfo;
			}
		}

		// Maybe in parent classes?
		auto [_, functionInfo] = thisTypeInfo->FindFunctionInHierarchy(pMemberFuncName);
		if (functionInfo != nullptr)
		{
			return functionInfo;
		}

		return nullptr;
	}

	[[nodiscard]] FunctionInfo const* GetFunctionInParents(std::string_view const& pMemberFuncName) const
	{

		for (FunctionInfo const& aFuncInfo : Reflector3::GetSingleton().GetTypeInfo(myReflectableClassID)->MemberFunctions)
		{
			if (aFuncInfo.name == pMemberFuncName)
			{
				return &aFuncInfo;
			}
		}

		return nullptr;
	}

	template <typename... Args>
	std::any	InvokeFunction(std::string_view pMemberFuncName, Args&&... pFuncArgs)
	{
		FunctionInfo const* theFunction = GetFunction(pMemberFuncName);
		if (theFunction == nullptr)
		{
			return {};
		}

		return theFunction->InvokeWithArgs(this, std::forward<Args>(pFuncArgs)...);
	}

	template <typename Ret, typename... Args>
	Ret	TypedInvokeFunction(std::string_view pMemberFuncName, Args&&... pFuncArgs)
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

	template <typename T>
	friend struct ReflectableClassIDSetter2;

protected:
	void	SetReflectableClassID(ClassID newClassID)
	{
		myReflectableClassID = newClassID;
	}

public:

	DECLARE_ATTORNEY(SetReflectableClassID);

private:
	ClassID	myReflectableClassID{ 0x2a2a };
};


#if USE_SERIALIZE_API
template <typename T, typename TProp>
void ReflectProperty3<T, TProp>::SetupSerializerFunction()
{
	if constexpr (HasArraySemantics_v<TProp>)
	{
		SerializerFunction = &REFLECTOR_SERIALIZER_TYPE::SerializeArrayProperty;
	}
	else if constexpr (HasMapSemantics_v<TProp>)
	{
		SerializerFunction = &REFLECTOR_SERIALIZER_TYPE::SerializeMapProperty;
	}
	else if constexpr (std::is_class_v<TProp>)
	{
		SerializerFunction = &REFLECTOR_SERIALIZER_TYPE::SerializeCompoundProperty;
	}
	// TODO: that doesnt manage custom enums
	else
	{
		SerializerFunction = &REFLECTOR_SERIALIZER_TYPE::template SerializeProperty<TProp>;
	}
}
#endif


template<typename... TypeList>
constexpr size_t SizeofSumOfAllTypes()
{
	return (0 + ... + sizeof(TypeList));
}

// https://stackoverflow.com/a/56720174/1987466
template<class T1, class... Ts>
constexpr bool IsOneOf() noexcept {
	return (std::is_same_v<T1, Ts> || ...);
}

// https://stackoverflow.com/a/1234781/1987466
template <typename MostDerived, typename C, typename M>
ptrdiff_t my_offsetof(M C::* member)
{
	MostDerived d;
	return reinterpret_cast<char*> (&(d.*member)) - reinterpret_cast<char*> (&d);
}


//https://stackoverflow.com/a/12812237/1987466
template <typename T, typename M> M get_member_type(M T::*);
template <typename T, typename M> T get_class_type(M T::*);

template <typename T,
	typename R,
	R T::* M
>
constexpr std::size_t my_offsetof2()
{
	return reinterpret_cast<std::size_t>(&(((T*)0)->*M));
}


template <typename T, typename... TOtherBases>
struct ReflectableClassIDSetter
{
	ReflectableClassIDSetter()
	{
		static_assert(std::is_base_of_v<Reflectable2, T>, "This class is only supposed to be used as a member variable of a Reflectable-derived class.");

		const size_t theReflectableOffset = (
			IsOneOf<Reflectable2, TOtherBases...>() ?
			SizeofSumOfAllTypes<TOtherBases...>() :
			SizeofSumOfAllTypes<TOtherBases...>()
			);
		std::byte* ptr = reinterpret_cast<std::byte*>(this) - theReflectableOffset;
		unsigned* theClassID = reinterpret_cast<unsigned*>(ptr);
		unsigned myClassID = T::theTypeInfo.ReflectableID;
		*theClassID = myClassID;
	}
};


template <typename T>
struct ReflectableClassIDSetter2 final
{
	ReflectableClassIDSetter2(T* pOwner)
	{
		static_assert(std::is_base_of_v<Reflectable2, T>, "This class is only supposed to be used as a member variable of a Reflectable-derived class.");
		pOwner->SetReflectableClassID(T::theTypeInfo.ReflectableID);
	}
};


template <typename T, typename... Args>
struct ClassInstantiator final
{
	ClassInstantiator()
	{
		static_assert(std::is_base_of_v<Reflectable2, T>, "ClassInstantiator is only meant to be used as a member of Reflectable-derived classes.");
		static_assert(std::is_constructible_v<T, Args...>,
			"No constructor associated with the parameter types of declared instantiator. Please keep instantiator and constructor synchronized.");
		Reflector3::EditSingleton().RegisterInstantiateFunction(T::GetClassReflectableTypeInfo().ReflectableID, &Instantiate);
	}

	static Reflectable2*	Instantiate(std::any const& pCtorParams)
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
	inline static ClassInstantiator<Self, __VA_ARGS__> VA_MACRO(GLUE_UNDERSCORE_ARGS, __VA_ARGS__);



#define FIRST_TYPE_0() Reflectable2
#define FIRST_TYPE_1(a, ...) a
#define FIRST_TYPE_2(a, ...) a
#define FIRST_TYPE_3(a, ...) a
#define FIRST_TYPE_4(a, ...) a

// TODO: maybe try to use IF_ELSE here
// Important to use VA_MACRO here otherwise MSVC will expand whole va_args in a single argument
#define INHERITANCE_LIST_0(...) Reflectable2
#define INHERITANCE_LIST_1(...) VA_MACRO(COMMA_ARGS, __VA_ARGS__)
#define INHERITANCE_LIST_2(...) VA_MACRO(COMMA_ARGS, __VA_ARGS__)
#define INHERITANCE_LIST_3(...) VA_MACRO(COMMA_ARGS, __VA_ARGS__)
#define INHERITANCE_LIST_4(...) VA_MACRO(COMMA_ARGS, __VA_ARGS__)
#define INHERITANCE_LIST_5(...) VA_MACRO(COMMA_ARGS, __VA_ARGS__)

#define FORWARD_INERHITANCE_LIST0() Reflectable2
#define FORWARD_INERHITANCE_LIST1(a1) a1
#define FORWARD_INERHITANCE_LIST2(a1, a2, a3, a4) a1 COMMA a2
#define FORWARD_INERHITANCE_LIST3(a1, a2, a3) a1 COMMA a2 COMMA a3
#define FORWARD_INERHITANCE_LIST4(a1, a2, a3, a4) a1 COMMA a2 COMMA a3 COMMA a4
#define FORWARD_INERHITANCE_LIST5(a1, a2, a3, a4, a5) a1 COMMA a2 COMMA a3 COMMA a5

#ifndef USE_DEFAULT_CONSTRUCTOR_INSTANTIATE
# define USE_DEFAULT_CONSTRUCTOR_INSTANTIATE() true
#endif

#define reflectable_class(classname, ...) class classname INHERITANCE_LIST(__VA_ARGS__)
#define reflectable_struct(structname, ...) \
	struct structname;\
	template <>\
	struct TypeInfoHelper<structname>\
	{\
		using Super = VA_MACRO(FIRST_TYPE_, __VA_ARGS__);\
		inline static char const* TypeName = STRINGIZE(structname);\
	};\
	struct structname : VA_MACRO(INHERITANCE_LIST_, __VA_ARGS__)

#define DECLARE_REFLECTABLE_INFO() \
	struct _self_type_tag {}; \
    constexpr auto _self_type_helper() -> decltype(::SelfType::Writer<_self_type_tag, decltype(this)>{}); \
    using Self = ::SelfType::Read<_self_type_tag>;\
	using Super = TypeInfoHelper<Self>::Super;\
	inline static TypedReflectableTypeInfo<Self, USE_DEFAULT_CONSTRUCTOR_INSTANTIATE()> theTypeInfo{TypeInfoHelper<Self>::TypeName};\
	static ReflectableTypeInfo const&	GetClassReflectableTypeInfo()\
	{\
		return theTypeInfo;\
	}\
	static ReflectableTypeInfo&	EditClassReflectableTypeInfo()\
	{\
		return theTypeInfo;\
	}\
	mutable ReflectableClassIDSetter2<Self> structname##_TYPEINFO_ID_SETTER{this}; // TODO: fix this "structname"
	// I had to use mutable, otherwise the MapUpdate function breaks compilation because we try to copy assign. Maybe try to find a better way


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


reflectable_struct(LogCopyable)
{
	DECLARE_REFLECTABLE_INFO()

	DECLARE_PROPERTY(float, aUselessProp, 4.f);

	LogCopyable() = default;
	LogCopyable(LogCopyable const& pOther)
	{
		printf("Hello copy\n");
		aUselessProp = pOther.aUselessProp;
	}

	LogCopyable& operator=(LogCopyable const& pOther)
	{
		printf("Hello equal\n");
		aUselessProp = pOther.aUselessProp;
		return *this;
	}

};

reflectable_struct(SuperCompound)
{
	DECLARE_REFLECTABLE_INFO()

	DECLARE_ARRAY_PROPERTY(int, titi, [5])

};


reflectable_struct(testcompound3)
{
	DECLARE_REFLECTABLE_INFO()

	DECLARE_PROPERTY((std::map<int, bool>), aBoolMap);

	DECLARE_PROPERTY((std::map<int, SuperCompound>), aSuperMap);

	testcompound3() = default;
};

reflectable_struct(testcompound2)
{
	DECLARE_REFLECTABLE_INFO()

	DECLARE_PROPERTY(int, leet, 1337);
	DECLARE_PROPERTY(LogCopyable, copyable);

	testcompound2() = default;
};

reflectable_struct(testcompound)
{
	DECLARE_REFLECTABLE_INFO()

	DECLARE_PROPERTY(int, compint, 0x2a2a)
	DECLARE_PROPERTY(testcompound2, compleet)

	testcompound() = default;
};


reflectable_struct(MegaCompound)
{
	DECLARE_REFLECTABLE_INFO()

	DECLARE_PROPERTY(int, compint, 0x2a2a)
	DECLARE_PROPERTY(testcompound2, compleet)
	DECLARE_ARRAY_PROPERTY(SuperCompound, toto, [3])

	MegaCompound() = default;
};

reflectable_struct(UltraCompound)
{
	DECLARE_REFLECTABLE_INFO()

	DECLARE_PROPERTY(MegaCompound, mega)

	UltraCompound() = default;
};

reflectable_struct(a)
{
	DECLARE_REFLECTABLE_INFO()

	int test()
	{
		printf("I'm just a test!\n");
		return 42;
	}
	MOE_METHOD_TYPEINFO(test);

	virtual ~a() = default;

	virtual void print()
	{
		printf("a\n");
	}

	DECLARE_PROPERTY(bool, atiti, 0)
	DECLARE_PROPERTY(float, atoto, 0)
};

struct FatOfTheLand
{
	bool bibi;
	float fat[4];
};


reflectable_struct(yolo, a, FatOfTheLand) // an example of multiple inheritance
{
	DECLARE_REFLECTABLE_INFO()
	void Roger()
	{
		printf("Roger that sir\n");
	}
	MOE_METHOD_TYPEINFO(Roger);

	void stream(int) {}

	 void print() override
	{
		printf("yolo\n");
	}

	DECLARE_PROPERTY(double, bdouble, 0)

	DECLARE_PROPERTY(testcompound, compvar)
	char bpouiet[10];

};


reflectable_struct(c,yolo)
{
	DECLARE_REFLECTABLE_INFO()

	DECLARE_PROPERTY(unsigned, ctoto, 0xDEADBEEF)


	DECLARE_PROPERTY(std::vector<int>, aVector, 1, 2, 3)
	DECLARE_ARRAY_PROPERTY(int, anArray, [10])
	DECLARE_ARRAY_PROPERTY(int, aMultiArray, [10][10])

	DECLARE_PROPERTY(MegaCompound, mega);
	DECLARE_PROPERTY(UltraCompound, ultra);

	c() = default;

	c(int, bool)
	{
		printf("bonjour int bool\n");
	}

	DECLARE_INSTANTIATOR(int, bool)

	void print() override
	{
		printf("c\n");
	}

};


reflectable_struct(d,c)
{
	DECLARE_REFLECTABLE_INFO()

	DECLARE_PROPERTY((std::map<int, bool>), aMap);

	DECLARE_PROPERTY(int, xp, 42)
	DECLARE_PROPERTY((std::map<int, bool>), aBoolMap);
	DECLARE_PROPERTY((std::map<int, testcompound2>), aFatMap);
	DECLARE_PROPERTY((std::map<int, std::map<bool, int>>), aMapInMap);

	DECLARE_PROPERTY(testcompound3, aStruct);
};

//reflectable_struct(x,c)
//{
//	DECLARE_REFLECTABLE_INFO()
//
//
//};

reflectable_struct(y,c)
{
	DECLARE_REFLECTABLE_INFO()


};

reflectable_struct(z,c)
{
	DECLARE_REFLECTABLE_INFO()
};


reflectable_struct(mapType)
{
	DECLARE_REFLECTABLE_INFO()
	DECLARE_PROPERTY((std::map<int, bool>), aEvenOddMap);
};

template <typename T>
struct Subclass
{
	static_assert(std::is_base_of_v<Reflectable2, T>, "Subclass only works for Reflectable-derived classes.");

	Subclass() = default;

	[[nodiscard]] bool	IsValid() const
	{
		ReflectableTypeInfo const& parentTypeInfo = T::GetClassReflectableTypeInfo();
		return (parentTypeInfo.IsParentOf(SubClassID));
	}

	void	SetClass(unsigned pNewID)
	{
		SubClassID = pNewID;
	}

	unsigned	GetClassID() const
	{
		return SubClassID;
	}

	template <typename Ret = T, typename... Args>
	[[nodiscard]] Ret*	Instantiate(Args&&... pCtorArgs)
	{
		static_assert(std::is_base_of_v<T, Ret>, "Instantiate return type must be derived from the subclass type parameter!");

		if (!IsValid())
		{
			return nullptr;
		}

		Reflectable2* newInstance;

		if constexpr (sizeof...(Args) != 0)
		{
			using ArgumentPackTuple = std::tuple<Args...>;
			std::any packedArgs(std::in_place_type<ArgumentPackTuple>, std::forward_as_tuple(std::forward<Args>(pCtorArgs)...));
			newInstance = Reflector3::GetSingleton().TryInstantiate(SubClassID, packedArgs);
		}
		else
		{
			newInstance = Reflector3::GetSingleton().TryInstantiate(SubClassID, {});
		}

		return static_cast<Ret*>(newInstance);
	}

private:

	unsigned	SubClassID{};
};


class noncopyable
{
public:
	noncopyable() = default;
	noncopyable(noncopyable const&) = delete;
	noncopyable& operator=(noncopyable const&) = delete;

};


//TODO LIST:
// - fonctions virtuelles
// - inline methods 
// - hint file for reflectable_struct and friends
reflectable_struct(Test)
{
	DECLARE_REFLECTABLE_INFO()

	DECLARE_PROPERTY(int, titi, 4444);
	DECLARE_PROPERTY(int, bobo, 1234);
	DECLARE_PROPERTY(float, arrrr, 42.f);

	MOE_METHOD_NAMED_PARAMS(int, tititest, int, tata);
	MOE_METHOD(int, grostoto, bool);
	void zdzdzdz(float bibi);
	MOE_METHOD_TYPEINFO(zdzdzdz);
	MOE_METHOD(void, passbyref, int&);

	MOE_METHOD(void, passbyref_tricky, noncopyable&);
};

void Test::passbyref(int& test)
{
	test = 42;
}

void Test::passbyref_tricky(noncopyable& test)
{
	test;
}


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


void testfloat(float atest)
{
	atest;
}



int main()
{
	auto& info = a::GetClassReflectableTypeInfo();
	auto& info2 = yolo::GetClassReflectableTypeInfo();
	auto& info3 = c::GetClassReflectableTypeInfo();
	Property<int> test = 0;
	test = 42;
	Test tata;
	int titi = test;
	if (test == 0)
	{
		printf("yes\n");
	}
	auto& refl = Test::GetClassReflectableTypeInfo();
	for (auto const& t : refl.Properties)
	{
		printf("t : %s\n", t.name.c_str());
	}
	auto sz = sizeof(Reflectable2);
	auto const& myTiti = tata.GetSafeProperty<int>("titi");
	auto const& myTiti2 = tata.GetSafeProperty<int>("titi");
	tata.SetProperty<int>("titi", 42);
	tata.SetProperty<int>("titi", 1337);

	int nbArgs = NARGS(1, 2, 3, 4, 5, 6, 7, 8, 9);
	int nbArgs2 = NARGS(1, 2);
	int nbArgs3 = NARGS();
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

	tata.TypedInvokeFunction<void, float>("zdzdzdz", 3.f);

	auto* funcInfo = tata.GetFunction("zdzdzdz");
	funcInfo->TypedInvokeWithArgs<void>(&tata, 3.f);

	int tets = 1;
	tata.TypedInvokeFunction<void>("passbyref", tets);
	noncopyable nocopy;
	tata.TypedInvokeFunction<void>("passbyref_tricky", nocopy);


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
	Reflectable2* aRefl = &pouet;
	for (auto it = c::GetClassReflectableTypeInfo().Properties.begin(); it; ++it)
	{
		printf("name: %s, type: %d, offset: %lu\n", (*it).name.c_str(), (*it).type, (*it).offset);
	}
	for (auto it = aRefl->GetProperties().begin(); it; ++it)
	{
		printf("name: %s, type: %d, offset: %lu\n", (*it).name.c_str(), (*it).type, (*it).offset);
	}

	Subclass<a> aSubClass;
	aSubClass.SetClass(c::GetClassReflectableTypeInfo().ReflectableID);
	a* instance = aSubClass.Instantiate();
	c* anotherInstance = aSubClass.Instantiate<c>(42, true); // TODO: try that with noncopyable...
	aSubClass.SetClass(yolo::GetClassReflectableTypeInfo().ReflectableID);
	a* aYolo = aSubClass.Instantiate();
	bool isAYolo = anotherInstance->IsA<yolo>();

	a* myA = static_cast<a*>(anotherInstance);
	yolo* myB = static_cast<yolo*>(anotherInstance);
	FatOfTheLand* myF = static_cast<FatOfTheLand*>(anotherInstance);
	printf("a: %p b: %p f: %p c: %p\n", myA, myB, myF, anotherInstance);

	auto ap1 = anotherInstance->GetClassReflectableTypeInfo().FindPropertyInHierarchy("atiti");
	auto bp1 = anotherInstance->GetClassReflectableTypeInfo().FindPropertyInHierarchy("bdouble");
	auto cp1 = anotherInstance->GetClassReflectableTypeInfo().FindPropertyInHierarchy("ctoto");

	class Interface {};
	class Parent : public Interface { int a; };
	class Child { int b; };
	struct GrandChild : public Parent, public Child { int a, b, c; };

	GrandChild GC;
	std::cout << "GrandChild's address is at : " << &GC << std::endl;
	std::cout << "Child's address is at : " << static_cast<Child*>(&GC) << std::endl;
	std::cout << "Parent's address is at : " << static_cast<Parent*>(&GC) << std::endl;
	std::cout << "Child's Interface address is at : " << reinterpret_cast<Interface*>(static_cast<Child*>(&GC)) << std::endl;

	aYolo->InvokeFunction("test");
	constexpr size_t szcomp = sizeof(testcompound);
	auto offcomp = offsetof(testcompound, compint);
	auto compoff = offsetof(c, compvar);
	testcompound const* monComp = (testcompound const*) (reinterpret_cast<std::byte const*>(anotherInstance) + 80);

	int const* monCompint = (int const*)(reinterpret_cast<std::byte const*>(anotherInstance) + 88);
	int const* compint = anotherInstance->GetProperty<int>("compvar.compint");
	assert(*compint == 0x2a2a);

	int const* compleet = anotherInstance->GetProperty<int>("compvar.compleet.leet");
	assert(*compleet == 1337);

	int const& compleetRef = anotherInstance->GetSafeProperty<int>("compvar.compleet.leet");
	assert(compleetRef == 1337);

	bool edited = anotherInstance->SetProperty<int>("compvar.compleet.leet", 42);
	assert(edited && anotherInstance->compvar.compleet.leet == 42);

	unsigned const* totoRef = anotherInstance->GetProperty<unsigned>("ctoto");
	assert(*totoRef == 0xdeadbeef);

	edited = anotherInstance->SetProperty<unsigned>("ctoto", 1);
	assert(edited && anotherInstance->ctoto == 1);
	edited = anotherInstance->SetProperty<int>("compvar.compleet.leet", 0x2a2a);
	assert(edited && anotherInstance->compvar.compleet.leet == 0x2a2a);

	constexpr bool hasBrackets1 = has_Brackets_v< std::string>;
	constexpr bool hasBrackets2 = has_Brackets_v< std::vector<int>>;
	constexpr bool hasBrackets3 = has_Brackets_v<int[]>;

	constexpr bool hasINsert4 = has_insert_v< std::string>;
	constexpr bool hasINsert5 = has_insert_v< std::vector<int>>;
	constexpr bool hasINsert6 = has_insert_v<int[]>;

	constexpr bool hasErase1 = has_erase_v< std::string>;
	constexpr bool hasErase2 = has_erase_v< std::vector<int>>;
	constexpr bool hasErase3 = has_erase_v<int[]>;

	constexpr bool hasClear1 = has_clear_v< std::string>;
	constexpr bool hasClear2 = has_clear_v< std::vector<int>>;
	constexpr bool hasClear3 = has_clear_v<int[]>;

	constexpr bool hasSize1 = has_size_v< std::string>;
	constexpr bool hasSize2 = has_size_v< std::vector<int>>;
	constexpr bool hasSize3 = has_size_v<int[]>;

	std::vector<int> testvec{ 1,2,3 };
	TypedArrayPropertyCRUDHandler2<std::vector<int>> vechandler;
	int first = *(int*)vechandler.ArrayRead(&testvec, 0);

	int rint = 42;
	vechandler.ArrayUpdate(&testvec, 0, &rint);
	rint = 1337;
	vechandler.ArrayCreate(&testvec, 3, &rint);

	vechandler.ArrayErase(&testvec, 1);

	auto szvec = vechandler.ArraySize(&testvec);
	vechandler.ArrayClear(&testvec);
	szvec = vechandler.ArraySize(&testvec);

	int testarray[]{ 1, 2, 3 };
	TypedArrayPropertyCRUDHandler2<decltype(testarray)> arrayHandler;

	first = *(int*)arrayHandler.ArrayRead(&testarray, 0);
	arrayHandler.ArrayUpdate(&testarray, 0, &rint);
	rint = 42;
	arrayHandler.ArrayCreate(&testarray, 2, &rint);

	// will assert/throw
	//arrayHandler.ArrayCreate(&testarray, 3, &rint);

	arrayHandler.ArrayErase(&testarray, 1);

	arrayHandler.ArrayClear(&testarray);

	auto szarr = arrayHandler.ArraySize(&testarray);

	constexpr bool testpb = has_size_v<std::vector<int>>;
	constexpr bool teste =  has_erase_v<std::vector<int>>;
	constexpr bool testi =  has_insert_v<std::vector<int>>;


	int const* vec0 = anotherInstance->GetProperty<int>("aVector[0]");

	anotherInstance->anArray[5] = 42;
	int const* arr0 = anotherInstance->GetProperty<int>("anArray[5]");
	assert(*arr0 == 42);

	anotherInstance->aMultiArray[1][2] = 1337;
	int const* arr1 = anotherInstance->GetProperty<int>("aMultiArray[1][2]");
	assert(*arr1 == 1337);
	bool modified = anotherInstance->SetProperty<int>("aMultiArray[1][2]", 5656);
	assert(modified && *arr1 == 5656);

	anotherInstance->mega.toto[2].titi[3] = 9999;
	int const* arr2 = anotherInstance->GetProperty<int>("mega.toto[2].titi[3]");
	assert(*arr2 == 9999);
	modified = anotherInstance->SetProperty<int>("mega.toto[2].titi[3]", 5678);
	assert(modified && *arr2 == 5678);

	anotherInstance->ultra.mega.toto[0].titi[2] = 0x7f7f;
	int const* arr3 = anotherInstance->GetProperty<int>("ultra.mega.toto[0].titi[2]");
	assert(*arr3 == 0x7f7f);
	modified = anotherInstance->SetProperty<int>("ultra.mega.toto[0].titi[2]", 0xabcd);
	assert(modified && *arr3 == 0xabcd);

	vec0 = anotherInstance->GetProperty<int>("aVector[4]");
	bool erased = anotherInstance->EraseProperty("aVector[4]");
	assert(erased);

	// test compound + array property in parent class
	d aD;
	modified = aD.SetProperty<int>("ultra.mega.toto[0].titi[2]", 0x1234);
	assert(modified && aD.ultra.mega.toto[0].titi[2] == 0x1234);
	erased = aD.EraseProperty("aVector[4]");
	assert(!erased);
	modified = aD.SetProperty<int>("aVector[4]", 0x42);
	assert(modified && aD.aVector[4] == 0x42);
	erased = aD.EraseProperty("aVector[4]");
	assert(erased);


	//Reflector3::EditSingleton().ExportTypeInfoSettings("reflector2.bin");

	//Reflector3::EditSingleton().ImportTypeInfoSettings("reflector2.bin");

	int testarray2[10];
	std::map<int, bool> testmap;
	std::unordered_map<int, bool> testhashmap;
	constexpr bool arrayhassemantics = HasArraySemantics_v<decltype(testarray2)>;
	static_assert(arrayhassemantics);

	constexpr bool vechassemantics = HasArraySemantics_v<decltype(testvec)>;
	static_assert(vechassemantics);

	constexpr bool maphassemantics = HasMapSemantics_v<decltype(testmap)>;
	static_assert(maphassemantics);

	constexpr bool hashmaphassemantics = HasMapSemantics_v<decltype(testhashmap)>;
	static_assert(hashmaphassemantics);

	constexpr bool arraynomapsemantics = HasMapSemantics_v<decltype(testarray2)>;
	static_assert(!arraynomapsemantics);

	constexpr bool vecnomapsemantics = HasMapSemantics_v<decltype(testvec)>;
	static_assert(!vecnomapsemantics);

	constexpr bool mapnoarraysemantics = HasArraySemantics_v<decltype(testmap)>;
	static_assert(!mapnoarraysemantics);

	constexpr bool hashmapnoarraysemantics = HasArraySemantics_v<decltype(testhashmap)>;
	static_assert(!hashmapnoarraysemantics);

	constexpr bool customnomapsemantics = HasMapSemantics_v<MegaCompound>;
	static_assert(!customnomapsemantics);

	constexpr bool customnnoarraysemantics = HasArraySemantics_v<MegaCompound>;
	static_assert(!customnnoarraysemantics);

	GetParenthesizedType<void(std::map<int, bool>)>::Type mymap;


	assert(aD.xp == 42);
	aD.aBoolMap[0] = false;

	bool const* mapbool = aD.GetProperty<bool>("aBoolMap[0]");
	assert(mapbool == &aD.aBoolMap[0]);
	bool bval = aD.GetSafeProperty<bool>("aBoolMap[0]");
	assert(bval == aD.aBoolMap[0]);
	aD.aBoolMap[1] = true;
	bval = aD.GetSafeProperty<bool>("aBoolMap[0]");
	assert(bval == aD.aBoolMap[0]);

	aD.aFatMap[0].leet = 4242;
	int const* mapint = aD.GetProperty<int>("aFatMap[0].leet");
	assert(mapint == &aD.aFatMap[0].leet);

	aD.aMapInMap[0][true] = 9999;
	mapint = aD.GetProperty<int>("aMapInMap[0][true]");
	assert(mapint == &aD.aMapInMap[0][true]);

	aD.aStruct.aBoolMap[1] = true;
	mapbool = aD.GetProperty<bool>("aStruct.aBoolMap[1]");
	assert(mapbool == &aD.aStruct.aBoolMap[1]);

	aD.aStruct.aSuperMap[42].titi[0] = 1;
	aD.aStruct.aSuperMap[42].titi[1] = 2;
	aD.aStruct.aSuperMap[42].titi[2] = 3;

	mapint = aD.GetProperty<int>("aStruct.aSuperMap[42].titi[1]");
	assert(mapint == &aD.aStruct.aSuperMap[42].titi[1] && *mapint == 2);

	testcompound2 clonedComp;
	clonedComp.leet = 123456789;
	testcompound2* clone = clonedComp.Clone<testcompound2>();
	assert(clone->leet == clonedComp.leet && &clone->leet != &clonedComp.leet);

	// test serializing an object
#if USE_SERIALIZE_API && RAPIDJSON_REFLECTION_SERIALIZER
	RapidJsonReflectorSerializer jsonSerializer;
	std::string serialized = jsonSerializer.Serialize(clonedComp);
	assert(serialized == "{\"copyable\":{\"aUselessProp\":4.0},\"leet\":123456789}");

	// test serializing an array
	SuperCompound seriaArray;
	for (int i = 0; i < 5; ++i)
		seriaArray.titi[i] = i;
	serialized = jsonSerializer.Serialize(seriaArray);
	assert(serialized == "{\"titi\":[0,1,2,3,4]}");

	// test serializing objects, arrays, and array of objects
	MegaCompound megaSerialized;
	for (int i = 0; i < 3; i++)
	{
		for (int j = 0; j < 5; ++j)
		{
			megaSerialized.toto[i].titi[j] = i + j;
		}
	}

	serialized = jsonSerializer.Serialize(megaSerialized);
	assert(serialized == "{\"toto\":[{\"titi\":[0,1,2,3,4]},{\"titi\":[1,2,3,4,5]},{\"titi\":[2,3,4,5,6]}],\"compleet\":{\"copyable\":{\"aUselessProp\":4.0},\"leet\":1337},\"compint\":10794}");

	// test serializing std::vector (among other things)
	c serializedC;
	serializedC.aVector = { 42,43,44,45,46 };
	for (int i = 0; i < 10; i++)
	{
		serializedC.anArray[i] = i + 1;
		for (int j = 0; j < 10; ++j)
		{
			serializedC.aMultiArray[i][j] = i + j;
		}
	}
	serialized = jsonSerializer.Serialize(serializedC);
	assert(serialized == 
	"{\"ultra\":{\"mega\":{\"toto\":\
[{\"titi\":[0,0,0,0,0]},{\"titi\":[0,0,0,0,0]},{\"titi\":[0,0,0,0,0]}],\
\"compleet\":{\"copyable\":{\"aUselessProp\":4.0},\"leet\":1337},\"compint\":10794}},\
\"mega\":{\"toto\":[{\"titi\":[0,0,0,0,0]},{\"titi\":[0,0,0,0,0]},{\"titi\":[0,0,0,0,0]}],\
\"compleet\":{\"copyable\":{\"aUselessProp\":4.0},\"leet\":1337},\"compint\":10794},\
\"aMultiArray\":[[0,1,2,3,4,5,6,7,8,9],[1,2,3,4,5,6,7,8,9,10],[2,3,4,5,6,7,8,9,10,11],[3,4,5,6,7,8,9,10,11,12],[4,5,6,7,8,9,10,11,12,13],[5,6,7,8,9,10,11,12,13,14],[6,7,8,9,10,11,12,13,14,15],[7,8,9,10,11,12,13,14,15,16],[8,9,10,11,12,13,14,15,16,17],[9,10,11,12,13,14,15,16,17,18]],\
\"anArray\":[1,2,3,4,5,6,7,8,9,10],\"aVector\":[42,43,44,45,46],\"ctoto\":3735928559,\
\"compvar\":{\"compleet\":{\"copyable\":{\"aUselessProp\":4.0},\"leet\":1337},\"compint\":10794},\"bdouble\":0.0,\"atoto\":0.0,\"atiti\":false}");

	// test serializing std::map (among other things)
	mapType serializedMap;
	for (int i = 0; i < 10; i++)
	{
		serializedMap.aEvenOddMap[i] = (i % 2 == 0);
	}
	serialized = jsonSerializer.Serialize(serializedMap);
	assert(serialized == "{\"aEvenOddMap\":{\"0\":true,\"1\":false,\"2\":true,\"3\":false,\"4\":true,\"5\":false,\"6\":true,\"7\":false,\"8\":true,\"9\":false}}");
#endif

	return 0;
}


#if USE_SERIALIZE_API && RAPIDJSON_REFLECTION_SERIALIZER
const char* RapidJsonReflectorSerializer::Serialize(Reflectable2 const& serializedObject)
{
	myBuffer.Clear(); // to clean any previously written information
	myJsonWriter.Reset(myBuffer); // to wipe the root of a previously serialized object

	myJsonWriter.StartObject();

	Reflector3::GetSingleton().GetTypeInfo(serializedObject.GetReflectableClassID())->ForEachPropertyInHierarchy([&serializedObject, this](TypeInfo2 const& pProperty)
	{
		void const* propPtr = serializedObject.GetProperty(pProperty.name);
		auto serializeFunc = pProperty.SerializerFunction;
		if (serializeFunc != nullptr)
		{
			(this->*serializeFunc)(pProperty, propPtr);
		}
	});

	myJsonWriter.EndObject();

	return myBuffer.GetString();
}

void RapidJsonReflectorSerializer::SerializeArrayValue(void const* pPropPtr,
	ArrayPropertyCRUDHandler const* pArrayHandler)
{
	myJsonWriter.StartArray();

	if (pArrayHandler != nullptr)
	{
		size_t arraySize = pArrayHandler->Size(pPropPtr);
		AbstractPropertyHandler elemHandler = pArrayHandler->ElementHandler();

		auto elementSerializer = pArrayHandler->ElementSerializer;
		if (elementSerializer != nullptr)
		{
			for (int iElem = 0; iElem < arraySize; ++iElem)
			{
				void const* elemVal = pArrayHandler->Read(pPropPtr, iElem);
				(this->*elementSerializer)(elemVal, &elemHandler);
			}
		}
	}

	myJsonWriter.EndArray();
}

void RapidJsonReflectorSerializer::SerializeMapValue(void const* pPropPtr, MapPropertyCRUDHandler const* pMapHandler)
{
	myJsonWriter.StartObject();

	if (pMapHandler != nullptr && pMapHandler->KeyValueSerializer != nullptr)
	{
		pMapHandler->KeyValueSerializer(pPropPtr, *this);
	}

	myJsonWriter.EndObject();
}

void RapidJsonReflectorSerializer::SerializeCompoundValue(void const* pPropPtr)
{
	// TODO: Casting to Reflectable feels easy here... Shouldn't there be a way to properly reflect structs without making them reflectable ?
	auto* reflectableProp = static_cast<Reflectable2 const*>(pPropPtr);
	ReflectableTypeInfo const* compTypeInfo = Reflector3::GetSingleton().GetTypeInfo(reflectableProp->GetReflectableClassID());
	assert(compTypeInfo != nullptr); // TODO: customize assert

	myJsonWriter.StartObject();

	compTypeInfo->ForEachPropertyInHierarchy([this, reflectableProp](TypeInfo2 const& pProperty)
		{
			void const* compProp = reflectableProp->GetProperty(pProperty.name);
			auto serializeFunc = pProperty.SerializerFunction;
			if (serializeFunc != nullptr)
			{
				(this->*serializeFunc)(pProperty, compProp);
			}
		});

	myJsonWriter.EndObject();
}


void RapidJsonReflectorSerializer::SerializeCompoundProperty(TypeInfo2 const& pSerializedTypeInfo, void const* pPropPtr)
{
	myJsonWriter.String(pSerializedTypeInfo.name.data(), pSerializedTypeInfo.name.size());

	SerializeCompoundValue(pPropPtr);
}
#endif

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


void Test::zdzdzdz(float bibi)
{
	bibi;
	printf("bonjour\n");
}

// References:
// https://stackoverflow.com/questions/41453/how-can-i-add-reflection-to-a-c-application
// http://msinilo.pl/blog2/post/p517/
// https://replicaisland.blogspot.com/2010/11/building-reflective-object-system-in-c.html
// https://medium.com/geekculture/c-inheritance-memory-model-eac9eb9c56b5