#include "DireTypeInfo.h"
#include "dire/DireReflectable.h"
#include <cstddef> // byte

void DIRE_NS::TypeInfo::CloneHierarchyPropertiesOf(Reflectable& pNewClone, const Reflectable& pCloned) const
{
	for (auto rit = ParentClasses.rbegin(); rit != ParentClasses.rend(); ++rit)
	{
		(*rit)->ClonePropertiesOf(pNewClone, pCloned);
	}

	ClonePropertiesOf(pNewClone, pCloned);
}

void DIRE_NS::TypeInfo::ClonePropertiesOf(Reflectable& pNewClone, const Reflectable& pCloned) const
{
	for (const PropertyTypeInfo& prop : Properties)
	{
		auto* propPtr = pCloned.GetProperty<const void *>(prop.GetName());
		if (propPtr != nullptr)
		{
			auto actualOffset = ((const ::std::byte *)propPtr - (const ::std::byte *)&pCloned);
			if (prop.GetCopyConstructorFunction() != nullptr)
			{
				prop.GetCopyConstructorFunction()(&pNewClone, &pCloned, actualOffset);
			}
		}
	}
}
