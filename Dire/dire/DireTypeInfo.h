#pragma once

#include "DireIntrusiveList.h"
#include "DireString.h"
#include "DireTypeHandlers.h"
#include "DireTypes.h"
#include <any>
#include <vector>

namespace DIRE_NS
{
	class PropertyTypeInfo : public IntrusiveListNode<PropertyTypeInfo>
	{
		using CopyConstructorPtr = void (*)(void* pDestAddr, const void * pOther, size_t pOffset);

	public:
		PropertyTypeInfo(const char * pName, const std::ptrdiff_t pOffset, const size_t pSize, const CopyConstructorPtr pCopyCtor = nullptr) :
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

		const DIRE_STRING& GetName() const { return Name; }

#if DIRE_USE_SERIALIZATION
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
			if constexpr (HasArraySemantics_v<TProp>)
			{
				SetType(Type::Array);
			}
			else if constexpr (HasMapSemantics_v<TProp>)
			{
				SetType(Type::Map);
			}
			else if constexpr (std::is_class_v<TProp> && !IsEnum<TProp>)
			{
				SetType(Type::Object);
			}
			else if constexpr (std::is_enum_v<TProp>)
			{
				SetType(FromEnumToUnderlyingType<TProp>());
			}
			else if constexpr (std::is_base_of_v<Enum, TProp>)
			{
				SetType(Type::Enum);
			}
			else // primary type
			{
				SetType(FromActualTypeToEnumType<TProp>::EnumType);
			}
		}

		template <typename TProp>
		void	PopulateDataStructureHandler()
		{
			if constexpr (HasMapSemantics_v<TProp>)
			{
				DataStructurePropertyHandler.MapHandler = &TypedMapDataStructureHandler<TProp>::GetInstance();
			}
			else if constexpr (HasArraySemantics_v<TProp>)
			{
				DataStructurePropertyHandler.ArrayHandler = &TypedArrayDataStructureHandler<TProp>::GetInstance();
			}
			else if constexpr (std::is_base_of_v<Enum, TProp>)
			{
				DataStructurePropertyHandler.EnumHandler = &TypedEnumDataStructureHandler<TProp>::GetInstance();
			}
		}

		template <typename TProp>
		void	SetReflectableID()
		{
			if constexpr (std::is_base_of_v<Reflectable2, TProp>)
			{
				reflectableID = TProp::GetClassReflectableTypeInfo().GetID();
			}
			else
			{
				reflectableID = (unsigned)-1;
			}
		}

		template <typename TProp>
		void	InitializeReflectionCopyConstructor()
		{
			if constexpr (std::is_trivially_copyable_v<TProp>)
			{
				CopyCtor = [](void* pDestAddr, void const* pSrc, size_t pOffset)
				{
					memcpy((std::byte*)pDestAddr + pOffset, (std::byte const*)pSrc + pOffset, sizeof(TProp));
				};
			}
			else if constexpr (std::is_assignable_v<TProp, TProp> && !std::is_trivially_copy_assignable_v<TProp>)
			{ // class has an overloaded operator=
				CopyCtor = [](void* pDestAddr, void const* pSrc, size_t pOffset)
				{
					TProp* actualDestination = (TProp*)((std::byte*)pDestAddr + pOffset);
					TProp const* actualSrc = (TProp const*)((std::byte const*)pSrc + pOffset);
					(*actualDestination) = *actualSrc;
				};
			}
			else // C-style arrays, for example...
			{
				CopyCtor = [](void* pDestAddr, void const* pOther, size_t pOffset)
				{
					TProp* actualDestination = (TProp*)((std::byte*)pDestAddr + pOffset);
					TProp const* actualSrc = (TProp const*)((std::byte const*)pOther + pOffset);

					std::copy(std::begin(*actualSrc), std::end(*actualSrc), std::begin(*actualDestination));
				};
			}
		}

		void	SetType(const Type pType)
		{
			TypeEnum = pType;
		}

	private:

		DIRE_STRING		Name; // TODO: try storing a string view?
		Type			TypeEnum;
		std::ptrdiff_t	Offset;
		std::size_t		Size;
		DataStructureHandler	DataStructurePropertyHandler; // Useful for array-like or associative data structures, will stay null for other types.
		CopyConstructorPtr CopyCtor = nullptr; // if null : this type is not copy-constructible

		// TODO: storing it here is kind of a hack to go quick.
		// I guess we should have a map of Type->ClassID to be able to easily find the class ID...
		// without duplicating this information in all the type info structures of every property of the same type.
		unsigned		reflectableID = (unsigned)-1;

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