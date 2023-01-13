#include "DireTypeInfo.h"
#include "DireReflectable.h"
#include <cstddef> // byte

void DIRE_NS::TypeInfo::CloneHierarchyPropertiesOf(Reflectable2& pNewClone, const Reflectable2& pCloned) const
{
	for (auto rit = ParentClasses.rbegin(); rit != ParentClasses.rend(); ++rit)
	{
		(*rit)->ClonePropertiesOf(pNewClone, pCloned);
	}

	ClonePropertiesOf(pNewClone, pCloned);
}

void DIRE_NS::TypeInfo::ClonePropertiesOf(Reflectable2& pNewClone, const Reflectable2& pCloned) const
{
	for (const PropertyTypeInfo& prop : Properties)
	{
		auto* propPtr = pCloned.GetProperty<const void *>(prop.GetName());
		if (propPtr != nullptr)
		{
			auto actualOffset = ((const ::std::byte *)propPtr - (const ::std::byte *)&pCloned); // accounting for vptr etc.
			if (prop.GetCopyConstructorFunction() != nullptr)
			{
				prop.GetCopyConstructorFunction()(&pNewClone, &pCloned, actualOffset);
			}
		}
	}
}
