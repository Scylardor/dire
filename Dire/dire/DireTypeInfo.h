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
			myName(pName), myMetatype(Type::Unknown), myOffset(pOffset), mySize(pSize), myCopyCtor(pCopyCtor)
		{}

		[[nodiscard]] const ArrayDataStructureHandler* GetArrayHandler() const
		{
			return myDataStructurePropertyHandler.GetArrayHandler();
		}

		[[nodiscard]] const MapDataStructureHandler* GetMapHandler() const
		{
			return myDataStructurePropertyHandler.GetMapHandler();
		}

		[[nodiscard]] const DataStructureHandler& GetDataStructureHandler() const
		{
			return myDataStructurePropertyHandler;
		}

		[[nodiscard]] const DIRE_STRING& GetName() const { return myName; }

		[[nodiscard]] Type		GetMetatype() const { return myMetatype; }

		[[nodiscard]] size_t	GetOffset() const { return myOffset; }

		[[nodiscard]] size_t	GetSize() const { return mySize; }


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
				myDataStructurePropertyHandler.MapHandler = &TypedMapDataStructureHandler<TProp>::GetInstance();
			}
			else if constexpr (HasArraySemantics_v<TProp>)
			{
				myDataStructurePropertyHandler.ArrayHandler = &TypedArrayDataStructureHandler<TProp>::GetInstance();
			}
			else if constexpr (std::is_base_of_v<Enum, TProp>)
			{
				myDataStructurePropertyHandler.EnumHandler = &TypedEnumDataStructureHandler<TProp>::GetInstance();
			}
		}

		template <typename TProp>
		void	SetReflectableID()
		{
			if constexpr (std::is_base_of_v<Reflectable2, TProp>)
			{
				myReflectableID = TProp::GetClassReflectableTypeInfo().GetID();
			}
			else
			{
				myReflectableID = (unsigned)-1;
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

		void	SetType(const Type pType)
		{
			myMetatype = pType;
		}

	private:

		DIRE_STRING		myName; // TODO: try storing a string view?
		Type			myMetatype;
		std::ptrdiff_t	myOffset;
		std::size_t		mySize;
		DataStructureHandler	myDataStructurePropertyHandler; // Useful for array-like or associative data structures, will stay null for other types.
		CopyConstructorPtr myCopyCtor = nullptr; // if null : this type is not copy-constructible

		// TODO: storing it here is kind of a hack to go quick.
		// I guess we should have a map of Type->ClassID to be able to easily find the class ID...
		// without duplicating this information in all the type info structures of every property of the same type.
		unsigned		myReflectableID = (unsigned)-1;

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