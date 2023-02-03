#include "DireTypeInfo.h"
#include "dire/DireReflectable.h"
#include <cstddef> // byte
#include <algorithm> // find_if

bool dire::TypeInfo::IsParentOf(const dire::ReflectableID pChildClassID, bool pIncludingMyself) const
{
	// same logic as std::is_base_of: class is parent of itself (unless we strictly want only children)
	if (pChildClassID == GetID())
	{
		return pIncludingMyself;
	}

	auto predicate = [pChildClassID](const TypeInfo* pChildTypeInfo)
	{
		return pChildTypeInfo->myReflectableID == pChildClassID;
	};

	return std::find_if(myChildrenClasses.begin(), myChildrenClasses.end(), predicate) != myChildrenClasses.end();
}

dire::TypeInfo::ParentPropertyInfo dire::TypeInfo::FindParentClassProperty(const std::string_view& pName) const
{
	for (const TypeInfo* parentClass : myParentClasses)
	{
		const PropertyTypeInfo* foundProp = parentClass->FindProperty(pName);
		if (foundProp != nullptr)
		{
			return { parentClass, foundProp };
		}
	}

	return { nullptr, nullptr };
}

const dire::PropertyTypeInfo* dire::TypeInfo::FindProperty(const std::string_view& pName) const
{
	for (const auto& prop : myProperties)
	{
		if (prop.GetName() == pName)
		{
			return &prop;
		}
	}

	return nullptr;
}

const dire::PropertyTypeInfo* dire::TypeInfo::FindPropertyInHierarchy(const std::string_view& pName) const
{
	for (auto const& prop : myProperties)
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

dire::FunctionInfo const* dire::TypeInfo::FindFunction(const std::string_view& pFuncName) const
{
	for (FunctionInfo const& aFuncInfo : myMemberFunctions)
	{
		if (aFuncInfo.GetName() == pFuncName)
		{
			return &aFuncInfo;
		}
	}

	return nullptr;
}

dire::TypeInfo::ParentFunctionInfo dire::TypeInfo::FindParentFunction(const std::string_view& pFuncName) const
{
	for (const TypeInfo* parentClass : myParentClasses)
	{
		const FunctionInfo* foundFunc = parentClass->FindFunction(pFuncName);
		if (foundFunc != nullptr)
		{
			return { parentClass, foundFunc };
		}
	}

	return {};
}

void DIRE_NS::TypeInfo::CloneHierarchyPropertiesOf(Reflectable& pNewClone, const Reflectable& pCloned) const
{
	for (auto rit = myParentClasses.rbegin(); rit != myParentClasses.rend(); ++rit)
	{
		(*rit)->ClonePropertiesOf(pNewClone, pCloned);
	}

	ClonePropertiesOf(pNewClone, pCloned);
}

void DIRE_NS::TypeInfo::ClonePropertiesOf(Reflectable& pNewClone, const Reflectable& pCloned) const
{
	for (const PropertyTypeInfo& prop : myProperties)
	{
		Reflectable::PropertyAccessor<void> accessor = pCloned.GetProperty(prop.GetName());
		const void* propPtr = accessor.GetPointer();
		if (propPtr != nullptr)
		{
			auto actualOffset = reinterpret_cast<const std::byte *>(propPtr) - reinterpret_cast<const std::byte*>(&pCloned);
			if (prop.GetCopyConstructorFunction() != nullptr)
			{
				prop.GetCopyConstructorFunction()(&pNewClone, &pCloned, size_t(actualOffset));
			}
		}
	}
}
