#pragma once

namespace DIRE_NS
{
	template <typename T>
	typename IntrusiveLinkedList<T>::iterator& IntrusiveLinkedList<T>::iterator::operator++()
	{
		myNode = myNode->Next;
		return *this;
	}

	template <typename T>
	typename IntrusiveLinkedList<T>::iterator IntrusiveLinkedList<T>::iterator::operator++(int)
	{
		iterator it(myNode);
		this->operator++();
		return it;
	}

	template <typename T>
	IntrusiveLinkedList<T>::iterator::operator bool() const
	{
		return (myNode != nullptr);
	}

	template <typename T>
	bool IntrusiveLinkedList<T>::iterator::operator!=(iterator& other) const
	{
		return myNode != other.myNode;
	}

	template <typename T>
	T& IntrusiveLinkedList<T>::iterator::operator*() const
	{
		return *((T*)myNode);
	}

	template <typename T>
	typename IntrusiveLinkedList<T>::const_iterator& IntrusiveLinkedList<T>::const_iterator::operator++()
	{
		myNode = myNode->Next;
		return *this;
	}

	template <typename T>
	typename IntrusiveLinkedList<T>::const_iterator IntrusiveLinkedList<T>::const_iterator::operator++(int)
	{
		const_iterator it(myNode);
		this->operator++();
		return it;
	}

	template <typename T>
	IntrusiveLinkedList<T>::const_iterator::operator bool() const
	{
		return (myNode != nullptr);
	}

	template <typename T>
	bool IntrusiveLinkedList<T>::const_iterator::operator!=(const_iterator& other) const
	{
		return myNode != other.myNode;
	}

	template <typename T>
	T const& IntrusiveLinkedList<T>::const_iterator::operator*() const
	{
		return *((T const*)myNode);
	}

	template <typename T>
	typename IntrusiveLinkedList<T>::iterator IntrusiveLinkedList<T>::begin()
	{
		return iterator(Head);
	}

	template <typename T>
	typename IntrusiveLinkedList<T>::iterator IntrusiveLinkedList<T>::end()
	{
		return nullptr;
	}

	template <typename T>
	typename IntrusiveLinkedList<T>::const_iterator IntrusiveLinkedList<T>::begin() const
	{
		return const_iterator(Head);
	}

	template <typename T>
	typename IntrusiveLinkedList<T>::const_iterator IntrusiveLinkedList<T>::end() const
	{
		return nullptr;
	}

	template <typename T>
	void IntrusiveLinkedList<T>::PushBackNewNode(IntrusiveListNode<T>& pNewNode)
	{
		if (Head == nullptr)
			Head = &pNewNode;

		if (Tail != nullptr)
			Tail->Next = &pNewNode;

		Tail = &pNewNode;
	}
}