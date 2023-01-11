#pragma once

namespace DIRE_NS
{
	template <typename T, typename IDType = int>
	class AutomaticTypeCounter
	{
	public:
		AutomaticTypeCounter() = default;

		static IDType	TakeNextID()
		{
			return theCounter++;
		}

		static IDType	PeekNextID()
		{
			return theCounter;
		}

	private:
		inline static IDType	theCounter = 0;
	};
}