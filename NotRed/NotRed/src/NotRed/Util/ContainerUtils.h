#pragma once

namespace NR::Utils 
{
	template<typename T, typename ConditionFunction>
	bool RemoveIf(std::vector<T>& vector, ConditionFunction condition)
	{
		for (std::vector<T>::iterator it = vector.begin(); it != vector.end(); ++it)
		{
			if (condition(*it))
			{
				vector.erase(it);
				return true;
			}
		}

		return false;
	}
}