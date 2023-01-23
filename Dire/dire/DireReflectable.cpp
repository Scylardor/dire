#include "DireReflectable.h"

namespace DIRE_NS
{
	const Reflectable::GetPropertyResult Reflectable::GetPropertyImpl(DIRE_STRING_VIEW pFullPath) const
	{
		const TypeInfo* thisTypeInfo = GetReflectableTypeInfo();

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
				return {"Syntax error: Mismatched bracket or empty brackets."};
			}

			DIRE_STRING_VIEW propName = DIRE_STRING_VIEW(pFullPath.data(), leftBrackPos);

			if (const PropertyTypeInfo* thisProp = thisTypeInfo->FindPropertyInHierarchy(propName))
			{
				if (thisProp->GetMetatype() == Type::Array)
				{
					const ConvertResult<int> propIndex = FromCharsConverter<int>::Convert((pFullPath.data() + leftBrackPos + 1));
					if (propIndex.HasError())
						return { propIndex.GetError() };

					pFullPath.remove_prefix(rightBrackPos + 1);
					return GetArrayProperty(thisTypeInfo, propName, pFullPath, propIndex.GetValue(), propertyAddr);
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

	[[nodiscard]] bool Reflectable::EraseProperty(DIRE_STRING_VIEW pName)
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
				if (result.TypeInfo->GetMetatype() == Type::Array)
				{
					void* array = const_cast<void*>(result.Address);
					ConvertResult<size_t> index = FromCharsConverter<size_t>::Convert(key);
					if (index.HasError())
						return false;

					return result.TypeInfo->GetArrayHandler()->Erase(array, index.GetValue());
				}
				if (result.TypeInfo->GetMetatype() == Type::Map)
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
				if (result.TypeInfo->GetMetatype() == Type::Array)
				{
					void* array = const_cast<void*>(result.Address);
					result.TypeInfo->GetArrayHandler()->Clear(array);
					return true;
				}
				if (result.TypeInfo->GetMetatype() == Type::Map)
				{
					void* map = const_cast<void*>(result.Address);
					result.TypeInfo->GetMapHandler()->Clear(map);
					return true;
				}
			}
		}

		return false;
	}

	Reflectable::GetPropertyResult Reflectable::RecurseFindArrayProperty(IArrayDataStructureHandler const* pArrayHandler,
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
			const TypeInfo * arrayElemTypeInfo = TypeInfoDatabase::GetSingleton().GetTypeInfo(pArrayHandler->ElementReflectableID());
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
				const size_t rightBrackPos = pRemainingPath.find(']');
				if (rightBrackPos == pRemainingPath.npos || rightBrackPos < leftBrackPos || rightBrackPos == leftBrackPos + 2) //+2 because leftBrackPos is off by 1
				{
					return {};
				}
				ConvertResult<int> propIndex = FromCharsConverter<int>::Convert(pRemainingPath.data() + leftBrackPos);
				if (propIndex.HasError())
					return { propIndex.GetError() };

				pName = DIRE_STRING_VIEW(pRemainingPath.data(), pRemainingPath.length() - suffixPos + 1);
				pRemainingPath.remove_prefix(rightBrackPos + 1);
				return GetArrayProperty(arrayElemTypeInfo, pName, pRemainingPath, propIndex.GetValue(), pPropPtr);
			}
		}
		else if (leftBrackPos != pRemainingPath.npos && leftBrackPos < dotPos) // it's an array in an array
		{
			const size_t rightBrackPos = pRemainingPath.find(']');
			// If there is a mismatched bracket, or the closing bracket is before the opening one,
			// or there is no number between the two brackets, the expression has to be ill-formed!
			if (rightBrackPos == pRemainingPath.npos || rightBrackPos < leftBrackPos || rightBrackPos == leftBrackPos + 1)
			{
				return {};
			}

			IArrayDataStructureHandler const* elementHandler = pArrayHandler->ElementHandler().GetArrayHandler();

			if (elementHandler == nullptr)
			{
				return {}; // Tried to access a property type that is not an array or not a Reflectable
			}

			ConvertResult<int> propIndex = FromCharsConverter<int>::Convert(pRemainingPath.data() + leftBrackPos + 1);
			if (propIndex.HasError())
				return { propIndex.GetError() };

			pRemainingPath.remove_prefix(rightBrackPos + 1);
			return RecurseFindArrayProperty(elementHandler, pName, pRemainingPath, propIndex.GetValue(), pPropPtr);
		}

		DIRE_ASSERT(false);
		return {}; // Never supposed to arrive here!
	}

	Reflectable::GetPropertyResult Reflectable::RecurseArrayMapProperty(const PropertyTypeInfo * pProperty, const std::byte * pPropPtr,
		DIRE_STRING_VIEW pRemainingPath, DIRE_STRING_VIEW pKey) const
	{
		// Find out the type of handler we are interested in
		const void * value = nullptr;
		const bool propIsAnArray = pProperty->GetMetatype() == Type::Array;
		DIRE_ASSERT(propIsAnArray || pProperty->GetMetatype() == Type::Map);

		if (propIsAnArray)
		{
			ConvertResult<size_t> index = FromCharsConverter<size_t>::Convert(pKey);
			if (index.HasError())
				return { index.GetError() };

			value = pProperty->GetArrayHandler()->Read(pPropPtr, index.GetValue());
		}
		else
		{
			value = pProperty->GetMapHandler()->Read(pPropPtr, pKey);
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
		auto* valueAsBytes = static_cast<const std::byte *>(value);

		if (pRemainingPath[0] == '.') // it's a nested structure
		{
			ReflectableID valueID = propIsAnArray ? pProperty->GetArrayHandler()->ElementReflectableID() : pProperty->GetMapHandler()->ValueReflectableID();
			const TypeInfo * valueTypeInfo = TypeInfoDatabase::GetSingleton().GetTypeInfo(valueID);
			auto nextDelimiterPos = pRemainingPath.find_first_of(".[", 1);
			DIRE_STRING_VIEW propName = pRemainingPath.substr(1, nextDelimiterPos == pRemainingPath.npos ? nextDelimiterPos : nextDelimiterPos - 1);
			if (nextDelimiterPos == pRemainingPath.npos)
			{
				pRemainingPath.remove_prefix(1);
				Reflectable const* reflectable = reinterpret_cast<Reflectable const*>(valueAsBytes);
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

			if (propIsAnArray)
			{
				return RecurseArrayProperty(pProperty->GetArrayHandler()->ElementHandler().GetArrayHandler(), valueAsBytes, pRemainingPath, pKey);
			}
			else
			{
				return RecurseMapProperty(pProperty->GetMapHandler()->ValueDataHandler().GetMapHandler(), valueAsBytes, pRemainingPath, pKey);
			}
		}

		return {};
	}

	Reflectable::GetPropertyResult Reflectable::RecurseArrayProperty(const IArrayDataStructureHandler* pArrayHandler, const std::byte* pPropPtr,
		DIRE_STRING_VIEW pRemainingPath, DIRE_STRING_VIEW pKey) const
	{
		// Find out the type of handler we are interested in
		const void* value = nullptr;

		ConvertResult<size_t> index = FromCharsConverter<size_t>::Convert(pKey);
		if (index.HasError())
			return { index.GetError() };

		value = pArrayHandler->Read(pPropPtr, index.GetValue());

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
			ReflectableID valueID = pArrayHandler->ElementReflectableID();
			const TypeInfo * valueTypeInfo = TypeInfoDatabase::GetSingleton().GetTypeInfo(valueID);
			auto nextDelimiterPos = pRemainingPath.find_first_of(".[", 1);
			DIRE_STRING_VIEW propName = pRemainingPath.substr(1, nextDelimiterPos == pRemainingPath.npos ? nextDelimiterPos : nextDelimiterPos - 1);
			if (nextDelimiterPos == pRemainingPath.npos)
			{
				pRemainingPath.remove_prefix(1);
				Reflectable const* reflectable = reinterpret_cast<Reflectable const*>(valueAsBytes);
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

			DataStructureHandler elementHandler = pArrayHandler->ElementHandler();
			if (pArrayHandler->ElementType() == Type::Array)
			{
				return RecurseArrayProperty(elementHandler.GetArrayHandler(), valueAsBytes, pRemainingPath, pKey);
			}

			return RecurseMapProperty(elementHandler.GetMapHandler(), valueAsBytes, pRemainingPath, pKey);
		}

		return {};
	}

	Reflectable::GetPropertyResult Reflectable::RecurseMapProperty(const IMapDataStructureHandler* pMapHandler, const std::byte* pPropPtr,
		DIRE_STRING_VIEW pRemainingPath, DIRE_STRING_VIEW pKey) const
	{
		// Find out the type of handler we are interested in
		const void* value = pMapHandler->Read(pPropPtr, pKey);

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
			ReflectableID valueID = pMapHandler->ValueReflectableID();
			const TypeInfo * valueTypeInfo = TypeInfoDatabase::GetSingleton().GetTypeInfo(valueID);
			auto nextDelimiterPos = pRemainingPath.find_first_of(".[", 1);
			DIRE_STRING_VIEW propName = pRemainingPath.substr(1, nextDelimiterPos == pRemainingPath.npos ? nextDelimiterPos : nextDelimiterPos - 1);
			if (nextDelimiterPos == pRemainingPath.npos)
			{
				pRemainingPath.remove_prefix(1);
				Reflectable const* reflectable = reinterpret_cast<Reflectable const*>(valueAsBytes);
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
			
			DataStructureHandler elementHandler = pMapHandler->ValueDataHandler();
			if (pMapHandler->ValueMetaType() == Type::Array)
			{
				return RecurseArrayProperty(elementHandler.GetArrayHandler(), valueAsBytes, pRemainingPath, pKey);
			}

			return RecurseMapProperty(elementHandler.GetMapHandler(), valueAsBytes, pRemainingPath, pKey);
		}

		return {};
	}

	[[nodiscard]] Reflectable::GetPropertyResult Reflectable::GetCompoundProperty(const TypeInfo * pTypeInfoOwner, DIRE_STRING_VIEW pName, DIRE_STRING_VIEW pFullPath, std::byte const* propertyAddr) const
	{
		// First, find our compound property
		const PropertyTypeInfo * thisProp = pTypeInfoOwner->FindPropertyInHierarchy(pName);
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
		const TypeInfo * nestedTypeInfo = TypeInfoDatabase::GetSingleton().GetTypeInfo(thisProp->GetReflectableID());
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
			const size_t rightBrackPos = pFullPath.find(']');
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

	Reflectable::GetPropertyResult Reflectable::GetArrayMapProperty(const TypeInfo * pTypeInfoOwner, DIRE_STRING_VIEW pName, DIRE_STRING_VIEW pRemainingPath, DIRE_STRING_VIEW pKey, std::byte const* pPropPtr) const
	{
		// First, find our array property
		const PropertyTypeInfo * thisProp = pTypeInfoOwner->FindPropertyInHierarchy(pName);
		if (thisProp == nullptr)
		{
			return {}; // There was no property with the given name.
		}

		pPropPtr += thisProp->GetOffset();
		// Find out the type of handler we are interested in
		return RecurseArrayMapProperty(thisProp, pPropPtr, pRemainingPath, pKey);
	}

	[[nodiscard]] Reflectable::GetPropertyResult Reflectable::GetArrayProperty(const TypeInfo * pTypeInfoOwner, DIRE_STRING_VIEW pName, DIRE_STRING_VIEW pRemainingPath, int pArrayIdx, std::byte const* pPropPtr) const
	{
		// First, find our array property
		const PropertyTypeInfo * thisProp = pTypeInfoOwner->FindPropertyInHierarchy(pName);
		if (thisProp == nullptr)
		{
			return {}; // There was no property with the given name.
		}

		pPropPtr += thisProp->GetOffset();
		IArrayDataStructureHandler const* arrayHandler = thisProp->GetArrayHandler();

		return RecurseFindArrayProperty(arrayHandler, pName, pRemainingPath, pArrayIdx, pPropPtr);
	}

	FunctionInfo const* Reflectable::GetFunction(DIRE_STRING_VIEW pMemberFuncName) const
	{
		const TypeInfo * thisTypeInfo = GetReflectableTypeInfo();
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
