#pragma once
#include "DireDefines.h"

namespace DIRE_NS
{
	template <class T>
	class Singleton
	{
	public:

		Dire_EXPORT static T& EditSingleton()
		{
			static T theSingleton; // Thread-safe since C++11!
			return theSingleton;
		}

		Dire_EXPORT static const T & GetSingleton()
		{
			return EditSingleton();
		}

		template <typename... Args>
		T& ReinitializeSingleton(Args&&... pCtorParams)
		{
			EditSingleton() = T(std::forward<Args>(pCtorParams)...);
			return EditSingleton();
		}

	protected:
		Singleton() = default;
		~Singleton() = default;

	private:

		Singleton(Singleton const&) = delete;   // Copy construct
		Singleton(Singleton&&) = delete;   // Move construct
		Singleton& operator=(Singleton const&) = delete;   // Copy assign
		Singleton& operator=(Singleton&&) = delete;   // Move assign
	};
}