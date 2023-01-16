#include "DireReflectable.h"

#include "DireAssert.h"

namespace DIRE_NS
{
	const Reflectable2::GetPropertyResult Reflectable2::GetPropertyImpl(DIRE_STRING_VIEW pFullPath) const
	{
		const TypeInfo* thisTypeInfo = GetTypeInfo();

		auto propertyAddr = reinterpret_cast<const std::byte*>(this);

		// Test for either array, map or compound access and branch into the one seen first
		// compound property syntax uses the standard "." accessor
		// array and map property syntax uses the standard "[]" array subscript operator
		const size_t dotPos = pFullPath.find('.');
		const size_t leftBrackPos = pFullPath.find('[');
		if (dotPos != pFullPath.npos && dotPos < leftBrackPos) // it's a compound
		{
			DIRE_STRING_VIEW compoundPropName = DIRE_STRING_VIEW(pFullPath.data(), dotPos);
			return GetCompoundProperty(thisTypeInfo, compoundPropName, pFullPath, propertyAddr);
		}
		else if (leftBrackPos != pFullPath.npos && leftBrackPos < dotPos) // it's an array or map
		{
			const size_t rightBrackPos = pFullPath.find(']');
			// If there is a mismatched bracket, or the closing bracket is before the opening one,
			// or there is nothing between the two brackets, the expression has to be ill-formed!
			if (rightBrackPos == pFullPath.npos || rightBrackPos < leftBrackPos || rightBrackPos == leftBrackPos + 1)
			{
				return {};
			}

			DIRE_STRING_VIEW propName = DIRE_STRING_VIEW(pFullPath.data(), leftBrackPos);

			if (const PropertyTypeInfo* thisProp = thisTypeInfo->FindPropertyInHierarchy(propName))
			{
				if (thisProp->GetMetatype() == Type::Array)
				{
					const int propIndex = atoi(pFullPath.data() + leftBrackPos + 1); // TODO: use own atoi to avoid multi-k-LOC dependency!
					pFullPath.remove_prefix(rightBrackPos + 1);
					return GetArrayProperty(thisTypeInfo, propName, pFullPath, propIndex, propertyAddr);
				}
				else if (thisProp->GetMetatype() == Type::Map)
				{
					const auto keyStartPos = leftBrackPos + 1;
					DIRE_STRING_VIEW key(pFullPath.data() + keyStartPos, rightBrackPos - keyStartPos);
					pFullPath.remove_prefix(rightBrackPos + 1);
					return GetArrayMapProperty(thisTypeInfo, propName, pFullPath, key, propertyAddr);
				}
			}
			return {}; // Didn't find the property or it's of a type that we don't know how to handle in this context.
		}

		// Otherwise: search for a "normal" property
		for (PropertyTypeInfo const& prop : thisTypeInfo->GetPropertyList())
		{
			if (prop.GetName() == pFullPath)
			{
				return GetPropertyResult(propertyAddr + prop.GetOffset(), &prop);
			}
		}

		// Maybe in the parent classes?
		auto [parentClass, parentProperty] = thisTypeInfo->FindParentClassProperty(pFullPath);
		if (parentClass != nullptr && parentProperty != nullptr) // A parent property was found
		{
			return GetPropertyResult(propertyAddr + parentProperty->GetOffset(), parentProperty);
		}

		return {};
	}

	[[nodiscard]] bool Reflectable2::EraseProperty(DIRE_STRING_VIEW pName)
	{
		// if we're trying to erase from an array or a map, search for the array or map property first.
		if (pName.back() == ']')
		{
			const auto leftBrackPos = pName.rfind('[');
			if (leftBrackPos == DIRE_STRING_VIEW::npos || leftBrackPos == pName.length() - 2) // syntax error
				return false;

			const DIRE_STRING_VIEW propName = pName.substr(0, leftBrackPos);
			GetPropertyResult result = GetPropertyImpl(propName);

			if (result.Address != nullptr && result.TypeInfo != nullptr)
			{
				DIRE_STRING_VIEW key = pName.substr(leftBrackPos + 1, pName.length() - 1 - leftBrackPos);
				if (result.TypeInfo->GetArrayHandler() != nullptr)
				{
					void* array = const_cast<void*>(result.Address);
					size_t index = FromCharsConverter<size_t>::Convert(key);
					return result.TypeInfo->GetArrayHandler()->Erase(array, index);
				}
				if (result.TypeInfo->GetMapHandler() != nullptr)
				{
					void* map = const_cast<void*>(result.Address);
					return result.TypeInfo->GetMapHandler()->Erase(map, key);
				}
			}
		}
		else
		{
			GetPropertyResult result = GetPropertyImpl(pName);
			if (result.Address != nullptr && result.TypeInfo != nullptr)
			{
				if (result.TypeInfo->GetArrayHandler() != nullptr)
				{
					void* array = const_cast<void*>(result.Address);
					result.TypeInfo->GetArrayHandler()->Clear(array);
					return true;
				}
				if (result.TypeInfo->GetMapHandler() != nullptr)
				{
					void* map = const_cast<void*>(result.Address);
					result.TypeInfo->GetMapHandler()->Clear(map);
					return true;
				}
			}
		}

		return false;
	}

	Reflectable2::GetPropertyResult Reflectable2::RecurseFindArrayProperty(ArrayDataStructureHandler const* pArrayHandler,
		DIRE_STRING_VIEW pName, DIRE_STRING_VIEW pRemainingPath,
		int pArrayIdx, std::byte const* pPropPtr) const
	{
		if (pArrayHandler == nullptr)
		{
			return {}; // not an array or index is out-of-bounds
		}

		pPropPtr = (std::byte const*)pArrayHandler->Read(pPropPtr, pArrayIdx);
		if (pRemainingPath.empty()) // this was what we were looking for : return
		{
			return GetPropertyResult(pPropPtr, nullptr);
		}

		// There is more to eat: find out if we're looking for an array in an array or a compound
		size_t dotPos = pRemainingPath.find('.');
		size_t const leftBrackPos = pRemainingPath.find('[');
		if (dotPos == pRemainingPath.npos && leftBrackPos == pRemainingPath.npos)
		{
			return {}; // it's not an array neither a compound? I don't know how to read it!
		}
		if (dotPos != pRemainingPath.npos && dotPos < leftBrackPos) // it's a compound in an array
		{
			TypeInfo const* arrayElemTypeInfo = Reflector3::GetSingleton().GetTypeInfo(pArrayHandler->ElementReflectableID());
			if (arrayElemTypeInfo == nullptr) // compound type that is not Reflectable...
			{
				return {};
			}
			pRemainingPath.remove_prefix(1); // strip the leading dot
			// Lookup the next dot (if any) to know if we should search for a compound or an array.
			dotPos = pRemainingPath.find('.');
			// Use leftBrackPos-1 because it was computed before stripping the leading dot, so it is off by 1
			auto suffixPos = (dotPos != pRemainingPath.npos || leftBrackPos != pRemainingPath.npos ? std::min(dotPos, leftBrackPos - 1) : 0);
			pName = DIRE_STRING_VIEW(pRemainingPath.data() + dotPos + 1, pRemainingPath.length() - (dotPos + 1));
			dotPos = pName.find('.');
			if (dotPos < leftBrackPos) // next entry is a compound
				return GetCompoundProperty(arrayElemTypeInfo, pName, pRemainingPath, pPropPtr);
			else // next entry is an array
			{
				size_t const rightBrackPos = pRemainingPath.find(']');
				if (rightBrackPos == pRemainingPath.npos || rightBrackPos < leftBrackPos || rightBrackPos == leftBrackPos + 2) //+2 because leftBrackPos is off by 1
				{
					return {};
				}
				int propIndex = atoi(pRemainingPath.data() + leftBrackPos); // TODO: use own atoi to avoid multi-k-LOC dependency!
				pName = DIRE_STRING_VIEW(pRemainingPath.data(), pRemainingPath.length() - suffixPos + 1);
				pRemainingPath.remove_prefix(rightBrackPos + 1);
				return GetArrayProperty(arrayElemTypeInfo, pName, pRemainingPath, propIndex, pPropPtr);
			}
		}
		else if (leftBrackPos != pRemainingPath.npos && leftBrackPos < dotPos) // it's an array in an array
		{
			size_t const rightBrackPos = pRemainingPath.find(']');
			// If there is a mismatched bracket, or the closing bracket is before the opening one,
			// or there is no number between the two brackets, the expression has to be ill-formed!
			if (rightBrackPos == pRemainingPath.npos || rightBrackPos < leftBrackPos || rightBrackPos == leftBrackPos + 1)
			{
				return {};
			}

			ArrayDataStructureHandler const* elementHandler = pArrayHandler->ElementHandler().GetArrayHandler();

			if (elementHandler == nullptr)
			{
				return {}; // Tried to access a property type that is not an array or not a Reflectable
			}

			int propIndex = atoi(pRemainingPath.data() + leftBrackPos + 1); // TODO: use own atoi to avoid multi-k-LOC dependency!
			pRemainingPath.remove_prefix(rightBrackPos + 1);
			return RecurseFindArrayProperty(elementHandler, pName, pRemainingPath, propIndex, pPropPtr);
		}

		DIRE_ASSERT(false);
		return {}; // Never supposed to arrive here!
	}

	Reflectable2::GetPropertyResult Reflectable2::RecurseArrayMapProperty(DataStructureHandler const& pDataStructureHandler, std::byte const* pPropPtr,
		DIRE_STRING_VIEW pRemainingPath, DIRE_STRING_VIEW pKey) const
	{
		// Find out the type of handler we are interested in
		void const* value = nullptr;
		if (pDataStructureHandler.GetArrayHandler())
		{
			size_t index = FromCharsConverter<size_t>::Convert(pKey); // TODO: check for errors
			value = pDataStructureHandler.GetArrayHandler()->Read(pPropPtr, index);
		}
		else if (pDataStructureHandler.GetMapHandler())
		{
			value = pDataStructureHandler.GetMapHandler()->Read(pPropPtr, pKey);
		}

		if (value == nullptr)
		{
			return {}; // there was no explorable data structure found: probably a syntax error
		}

		if (pRemainingPath.empty()) // this was what we were looking for : return
		{
			return GetPropertyResult(value, nullptr);
		}

		// there is more: we have to find if it's a compound, an array or a map...
		auto* valueAsBytes = static_cast<std::byte const*>(value);

		if (pRemainingPath[0] == '.') // it's a nested structure
		{
			ReflectableID valueID = pDataStructureHandler.GetArrayHandler() ? pDataStructureHandler.GetArrayHandler()->ElementReflectableID() : pDataStructureHandler.GetMapHandler()->ValueReflectableID();
			TypeInfo const* valueTypeInfo = Reflector3::GetSingleton().GetTypeInfo(valueID);
			auto nextDelimiterPos = pRemainingPath.find_first_of(".[", 1);
			DIRE_STRING_VIEW propName = pRemainingPath.substr(1, nextDelimiterPos == pRemainingPath.npos ? nextDelimiterPos : nextDelimiterPos - 1);
			if (nextDelimiterPos == pRemainingPath.npos)
			{
				pRemainingPath.remove_prefix(1);
				Reflectable2 const* reflectable = reinterpret_cast<Reflectable2 const*>(valueAsBytes);
				return reflectable->GetPropertyImpl(pRemainingPath);
			}
			if (pRemainingPath[nextDelimiterPos] == '.')
			{
				pRemainingPath.remove_prefix(1);
				return GetCompoundProperty(valueTypeInfo, propName, pRemainingPath, valueAsBytes);
			}
			else
			{
				size_t rightBracketPos = pRemainingPath.find(']', 1);
				if (rightBracketPos == pRemainingPath.npos || rightBracketPos == 1)
				{
					return {}; // syntax error: no right bracket or nothing between the brackets
				}
				pKey = pRemainingPath.substr(nextDelimiterPos + 1, rightBracketPos - nextDelimiterPos - 1);
				pRemainingPath.remove_prefix(rightBracketPos + 1);

				return GetArrayMapProperty(valueTypeInfo, propName, pRemainingPath, pKey, valueAsBytes);
			}
		}

		if (pRemainingPath[0] == '[')
		{
			size_t rightBracketPos = pRemainingPath.find(']', 1);
			if (rightBracketPos == pRemainingPath.npos || rightBracketPos == 1)
			{
				return {}; // syntax error: no right bracket or nothing between the brackets
			}
			pKey = pRemainingPath.substr(1, rightBracketPos - 1);
			pRemainingPath.remove_prefix(rightBracketPos + 1);

			if (pDataStructureHandler.GetArrayHandler())
			{
				return RecurseArrayMapProperty(pDataStructureHandler.GetArrayHandler()->ElementHandler(), valueAsBytes, pRemainingPath, pKey);
			}
			else if (pDataStructureHandler.GetMapHandler())
			{
				return RecurseArrayMapProperty(pDataStructureHandler.GetMapHandler()->ValueHandler(), valueAsBytes, pRemainingPath, pKey);
			}
		}

		return {};
	}

	[[nodiscard]] Reflectable2::GetPropertyResult Reflectable2::GetCompoundProperty(TypeInfo const* pTypeInfoOwner, DIRE_STRING_VIEW pName, DIRE_STRING_VIEW pFullPath, std::byte const* propertyAddr) const
	{
		// First, find our compound property
		PropertyTypeInfo const* thisProp = pTypeInfoOwner->FindPropertyInHierarchy(pName);
		if (thisProp == nullptr)
		{
			return {}; // There was no property with the given name.
		}

		// We found our compound property: consume its name from the "full path"
		// +1 to also remove the '.', except if we are at the last nested property.
		pFullPath.remove_prefix(std::min(pName.size() + 1, pFullPath.size()));

		// Now, add the offset of the compound to the total offset
		propertyAddr += (int)thisProp->GetOffset();

		// Do we need to go deeper?
		if (pFullPath.empty())
		{
			// No: just return the variable at the current offset
			return GetPropertyResult(propertyAddr, thisProp);
		}

		// Yes: recurse one more time, this time using this property's type info (needs to be Reflectable)
		TypeInfo const* nestedTypeInfo = Reflector3::GetSingleton().GetTypeInfo(thisProp->GetReflectableID());
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
				return {};
			}
			pName = DIRE_STRING_VIEW(pFullPath.data(), nextDelimiterPos);
			DIRE_STRING_VIEW key = pFullPath.substr(nextDelimiterPos + 1, rightBrackPos - nextDelimiterPos - 1);
			pFullPath.remove_prefix(rightBrackPos + 1);
			return GetArrayMapProperty(pTypeInfoOwner, pName, pFullPath, key, propertyAddr);
		}

		return GetCompoundProperty(pTypeInfoOwner, pName, pFullPath, propertyAddr);
	}

	Reflectable2::GetPropertyResult Reflectable2::GetArrayMapProperty(TypeInfo const* pTypeInfoOwner, DIRE_STRING_VIEW pName, DIRE_STRING_VIEW pRemainingPath, DIRE_STRING_VIEW pKey, std::byte const* pPropPtr) const
	{
		// First, find our array property
		PropertyTypeInfo const* thisProp = pTypeInfoOwner->FindPropertyInHierarchy(pName);
		if (thisProp == nullptr)
		{
			return {}; // There was no property with the given name.
		}

		pPropPtr += thisProp->GetOffset();
		// Find out the type of handler we are interested in
		DataStructureHandler const& dataStructureHandler = thisProp->GetDataStructureHandler();
		return RecurseArrayMapProperty(dataStructureHandler, pPropPtr, pRemainingPath, pKey);
	}

	[[nodiscard]] Reflectable2::GetPropertyResult Reflectable2::GetArrayProperty(TypeInfo const* pTypeInfoOwner, DIRE_STRING_VIEW pName, DIRE_STRING_VIEW pRemainingPath, int pArrayIdx, std::byte const* pPropPtr) const
	{
		// First, find our array property
		PropertyTypeInfo const* thisProp = pTypeInfoOwner->FindPropertyInHierarchy(pName);
		if (thisProp == nullptr)
		{
			return {}; // There was no property with the given name.
		}

		pPropPtr += thisProp->GetOffset();
		ArrayDataStructureHandler const* arrayHandler = thisProp->GetArrayHandler();

		return RecurseFindArrayProperty(arrayHandler, pName, pRemainingPath, pArrayIdx, pPropPtr);
	}

	FunctionInfo const* Reflectable2::GetFunction(DIRE_STRING_VIEW pMemberFuncName) const
	{
		TypeInfo const* thisTypeInfo = GetTypeInfo();
		for (FunctionInfo const& aFuncInfo : thisTypeInfo->GetFunctionList())
		{
			if (aFuncInfo.GetName() == pMemberFuncName)
			{
				return &aFuncInfo;
			}
		}

		// Maybe in parent classes?
		auto [_, functionInfo] = thisTypeInfo->FindParentFunction(pMemberFuncName);
		if (functionInfo != nullptr)
		{
			return functionInfo;
		}

		return nullptr;
	}
}
