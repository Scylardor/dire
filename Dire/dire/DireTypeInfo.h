#pragma once

#include "DireIntrusiveList.h"
#include "DireString.h"
#include "DireTypeHandlers.h"
#include "DireTypes.h"
#include <any>
#include <vector>

namespace DIRE_NS
{
	struct TypeInfo : IntrusiveListNode<TypeInfo>
	{
		using CopyConstructorPtr = void (*)(void* pDestAddr, const void * pOther, size_t pOffset);

		TypeInfo(const char * pName, const std::ptrdiff_t pOffset, const size_t pSize, const CopyConstructorPtr pCopyCtor = nullptr) :
			Name(pName), TypeEnum(Type::Unknown), Offset(pOffset), Size(pSize), CopyCtor(pCopyCtor)
		{}

		const ArrayDataStructureHandler* GetArrayHandler() const
		{
			return DataStructurePropertyHandler.GetArrayHandler();
		}

		const MapDataStructureHandler* GetMapHandler() const
		{
			return DataStructurePropertyHandler.GetMapHandler();
		}

		const DataStructureHandler& GetDataStructureHandler() const
		{
			return DataStructurePropertyHandler;
		}

#if DIRE_USE_SERIALIZATION
		virtual void	SerializeAttributes(class ISerializer& pSerializer) const = 0;

		struct SerializationState
		{
			bool	IsSerializable = false;
			bool	HasAttributesToSerialize = false;
		};

		virtual SerializationState	GetSerializableState() const = 0;
#endif

		DIRE_STRING		Name; // TODO: try storing a string view?
		Type			TypeEnum;
		std::ptrdiff_t	Offset;
		std::size_t		Size;
		DataStructureHandler	DataStructurePropertyHandler; // Useful for array-like or associative data structures, will stay null for other types.
		CopyConstructorPtr CopyCtor = nullptr; // if null : this type is not copy-constructible

		// TODO: storing it here is kind of a hack to go quick.
		// I guess we should have a map of Type->ClassID to be able to easily find the class ID...
		// without duplicating this information in all the type info structures of every property of the same type.
		unsigned		reflectableID = (unsigned) -1;

	protected:

		void	SetType(const Type pType)
		{
			TypeEnum = pType;
		}
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
		FunctionInfo(const char* pName, Type pReturnType) :
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

		const DIRE_STRING& GetName() const { return Name; }
		Type	GetReturnType() const { return ReturnType; }
		const std::vector<Type>& GetParametersTypes() const { return Parameters; }

	protected:

		void	PushBackParameterType(Type pNewParamType)
		{
			Parameters.push_back(pNewParamType);
		}

	private:

		DIRE_STRING	Name;
		Type		ReturnType{ Type::Void };
		std::vector<Type> Parameters;

	};

}