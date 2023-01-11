#pragma once

namespace DIRE_NS
{
	using ReflectableID = unsigned;

	static const ReflectableID INVALID_REFLECTABLE_ID = (ReflectableID)-1;

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
			return (parentTypeInfo.GetID() == myReflectableClassID || parentTypeInfo.IsParentOf(myReflectableClassID));
		}

		template <typename T = Reflectable2>
		T* Clone()
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

			thisTypeInfo->CloneHierarchyPropertiesOf(*clone, *this);

			return (T*)clone;
		}

		void	CloneProperties(Reflectable2 const* pCloned, ReflectableTypeInfo const* pClonedTypeInfo, Reflectable2* pClone)
		{
			pClonedTypeInfo->CloneHierarchyPropertiesOf(*pClone, *pCloned);
		}


		[[nodiscard]] IntrusiveLinkedList<TypeInfo2> const& GetProperties() const
		{
			return Reflector3::GetSingleton().GetTypeInfo(myReflectableClassID)->GetPropertyList();
		}

		[[nodiscard]] IntrusiveLinkedList<FunctionInfo> const& GetMemberFunctions() const
		{
			return Reflector3::GetSingleton().GetTypeInfo(myReflectableClassID)->GetFunctionList();
		}

		template <typename T>
		T const* GetMemberAtOffset(ptrdiff_t pOffset) const
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
			std::byte const* propertyAddr = reinterpret_cast<std::byte const*>(this) - thisTypeInfo->GetVptrOffset();

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
			for (TypeInfo2 const& prop : thisTypeInfo->GetPropertyList())
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
			std::byte* propertyAddr = reinterpret_cast<std::byte*>(this) - pTypeInfo->GetVptrOffset();

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
			for (ReflectableTypeInfo const* parentClass : pTypeInfo->GetParentClasses())
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
		[[nodiscard]] TProp const* GetCompoundProperty(ReflectableTypeInfo const* pTypeInfoOwner, std::string_view pName, std::string_view pFullPath, std::byte const* propertyAddr) const
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
			for (FunctionInfo const& aFuncInfo : thisTypeInfo->GetFunctionList())
			{
				if (aFuncInfo.name == pMemberFuncName)
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

}