#pragma once
#include "DireDefines.h"

namespace DIRE_NS
{
	template <typename T>
	class IntrusiveListNode
	{
	public:
		IntrusiveListNode() = default;

		IntrusiveListNode(IntrusiveListNode& pNext);

		IntrusiveListNode* Next = nullptr;
	};

	template <typename T>
	IntrusiveListNode<T>::IntrusiveListNode(IntrusiveListNode& pNext)
	{
		Next = pNext;
	}

	template <typename T>
	class IntrusiveLinkedList
	{
	public:

		class iterator
		{
		public:
			iterator(IntrusiveListNode<T>* pItNode = nullptr) :
				myNode(pItNode)
			{}

			iterator& operator ++();

			iterator operator ++(int);

			operator bool() const;

			bool operator!=(iterator& other) const;

			T& operator*() const;

		private:

			IntrusiveListNode<T>* myNode;
		};

		class const_iterator
		{
		public:
			const_iterator(IntrusiveListNode<T> const* pItNode = nullptr) :
				myNode(pItNode)
			{}

			const_iterator& operator ++();

			const_iterator operator ++(int);

			operator bool() const;

			bool operator!=(const_iterator& other) const;

			T const& operator*() const;

		private:

			IntrusiveListNode<T> const* myNode;
		};

		iterator begin();

		iterator end();

		const_iterator begin() const;

		const_iterator end() const;

		void	PushBackNewNode(IntrusiveListNode<T>& pNewNode);

	private:

		IntrusiveListNode<T>* Head = nullptr;
		IntrusiveListNode<T>* Tail = nullptr;
	};
}

#include "DireIntrusiveList.inl"