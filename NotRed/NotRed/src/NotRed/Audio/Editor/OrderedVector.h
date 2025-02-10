#pragma once

#include <unordered_map>
#include <shared_mutex>

namespace NR
{
	struct LockBase
	{
	protected:
		virtual void Lock() = 0;
		virtual void Unlock() = 0;
		virtual void LockShared() = 0;
		virtual void UnlockShared() = 0;

	protected:
		bool mShared;

	public:
		explicit LockBase(bool shared = false) : mShared(shared) {}
	};

	struct EmptyLock : private LockBase
	{
		explicit EmptyLock(std::shared_mutex& mutex, bool shared = false)
			: LockBase(shared) {}

	private:
		void Lock() override {}
		void Unlock() override {}
		void LockShared() override {}
		void UnlockShared() override {}
	};

	struct SharedLock : private LockBase
	{
		explicit SharedLock(std::shared_mutex& mutex, bool shared = false)
			: LockBase(shared), mMutex(mutex)
		{
			if (mShared) LockShared();
			else Lock();
		}
		virtual ~SharedLock()
		{
			if (mShared) UnlockShared();
			else Unlock();
		}

	private:
		void Lock() override { mMutex.lock(); }
		void Unlock() override { mMutex.unlock(); }
		void LockShared() override { mMutex.lock_shared(); }
		void UnlockShared() override { mMutex.unlock_shared(); }

	private:
		std::shared_mutex& mMutex;
	};

	template<class Type, class LockType = EmptyLock, class Allocator = std::allocator<Type>>
	struct OrderedVector
	{
		OrderedVector() = default;
		OrderedVector(const OrderedVector& other)
			: order(other.order) {}

		template<class Value>
		void SetValues(const std::unordered_map<Type, Value>& map)
		{
			LockType lock{ mMutex };

			order.clear();
			order.reserve(map.size());
			for (auto& it : map) {
				order.push_back(it.first);
			}
			std::reverse(order.begin(), order.end());
		}

		void SetValues(const std::vector<Type>& vector)
		{
			LockType lock{ mMutex };

			order = vector;
		}

		bool SetValue(int index, const Type& value)
		{
			LockType lock{ mMutex };
			const bool isValid = index >= 0 && index < order.size();
			if (!isValid)
				return false;

			order[index] = value;
			return true;
		}

		bool SetNewPosition(const Type& value, int newIndex)
		{
			const int oldIndex = GetIndex(value);
			if (oldIndex < 0)
				return false;

			LockType lock{ mMutex };

			newIndex = std::clamp(newIndex, 0, (int)order.size() - 1);

			if (newIndex == oldIndex || oldIndex < 0)
				return false;

			if (newIndex < oldIndex)
			{
				// New position is before the old one
				order.erase(order.begin() + oldIndex);
				order.insert(order.begin() + newIndex, value);
			}
			else
			{
				// New position is after the old one
				order.insert(order.begin() + newIndex + 1, value);
				order.erase(order.begin() + oldIndex);
			}
			return true;
		}

		// @returns new size of the vector
		int Insert(const Type& value, int index)
		{
			LockType{ mMutex };
			order.insert(order.begin() + index, value);
			return order.size();
		}

		bool SetNewPosition(int oldIndex, int newIndex)
		{
			return SetNewPosition(order[oldIndex], newIndex);
		}

		bool IsIndexValid(int index) const { LockType lock{ mMutex, true }; return index >= 0 && index < order.size(); }
		size_t GetSize() const { LockType lock{ mMutex, true }; return order.size(); }
		bool IsEmpty() const { LockType lock{ mMutex, true }; return order.empty(); }
		void Clear() { LockType lock{ mMutex }; order.clear(); }

		const std::vector<Type>& GetVector() const { LockType lock{ mMutex, true }; return order; }
		std::vector<Type>& GetVector() { LockType lock{ mMutex }; return order; }

		Type GetLast() const { LockType lock{ mMutex, true };	return order.back(); }
		Type& GetLast() { LockType lock{ mMutex };			return order.back(); }
		Type GetFirst() const { LockType lock{ mMutex, true };	return order.front(); }
		Type& GetFirst() { LockType lock{ mMutex };			return order.front(); }
		Type GetValue(int index) const { LockType lock{ mMutex, true };	return order.at(index); }
		Type& GetValue(int index) { LockType lock{ mMutex };			return order.at(index); }

		Type& operator[](int index) { return order[index]; }

		// @returns index of the added item
		int PushBack(const Type& value) { LockType lock{ mMutex }; order.push_back(value); return (int)order.size() - 1; }

		/*	If removeOld is true - returned number of removed duplicate values found, new value added to the back.
			If removeOld is false - passed in value is not added, returned first index of the same value found.
			If removeOld is false and the same value was not found - the new value is added to the back, returned 0.
		*/
		int AddUnique(const Type& value, bool removeOld = false)
		{
			LockType lock{ mMutex };

			auto findIndex = [&] {
				auto it = std::find(order.begin(), order.end(), value);
				if (it != order.end()) return it - order.begin();
				else return 0;
				};

			if (removeOld)
			{
				auto it = std::remove(order.begin(), order.end(), value);
				const auto numRemoved = std::distance(order.end() - it);
				order.erase(it, order.end());
				order.push_back(value);
				return numRemoved;
			}
			else
			{
				const int index = findIndex();
				if (index != 0)
				{
					return index;
				}
				else
				{
					order.push_back(value);
					return 0;
				}
			}
		}

		bool Erase(const Type& value)
		{
			LockType lock{ mMutex };

			auto contains = [&](const Type& value) {
				return std::find(order.begin(), order.end(), value) != order.end();
				};

			if (!contains(value))
				return false;

			order.erase(order.begin() + GetIndex(value));
			return true;
		}
		bool ErasePosition(int index)
		{
			LockType lock{ mMutex };

			auto isValid = [&] { return index >= 0 && index < order.size(); };
			if (!isValid())
				return false;

			order.erase(order.begin() + index);
			return true;
		}

		bool MoveToFront(const Type& value) { return SetNewPosition(value, 0); };
		bool MoveToFront(int oldPosition) { return SetNewPosition(oldPosition, 0); };
		bool MoveToBack(const Type& value) { return SetNewPosition(value, (int)GetSize() - 1); };
		bool MoveToBack(int oldPosition) { return SetNewPosition(oldPosition, GetSize() - 1); };
		bool Swap(const Type& value1, const Type& value2)
		{
			LockType lock{ mMutex };

			auto it1 = std::find(order.begin(), order.end(), value1);
			auto it2 = std::find(order.begin(), order.end(), value2);
			if (it1 == order.end() || it2 == order.end())
				return false;

			std::swap(*it1, *it2);
			return true;
		};
		bool Swap(int position1, int position2)
		{
			LockType lock{ mMutex };

			auto isValid = [&](int index) { return index >= 0 && index < order.size(); };
			if (!isValid(position1) || !isValid(position2))
				return false;

			std::swap(order[position1], order[position2]);
			return true;
		}

		bool Contains(const Type& value) const
		{
			LockType lock{ mMutex, true };
			return std::find(order.begin(), order.end(), value) != order.end();
		}

		// @returns index of the first found value
		int GetIndex(const Type& value) const
		{
			LockType lock{ mMutex, true };

			const auto it = std::find(order.begin(), order.end(), value);
			if (it != order.end())
				return (int)(it - order.begin());
			else
				return -1;
		}

		/*	If removeOld is true - returned number of removed duplicate values found, new value added to the back.
			If removeOld is false - passed in value is not added, returned first index of the same value found.
			If removeOld is false and the same value was not found - the new value is added to the back, returned 0.
		*/
		template<class T>
		static int AddUnique(std::vector<T>& vector, const T& value, bool removeOld = false)
		{
			auto findIndex = [&] {
				auto it = std::find(vector.begin(), vector.end(), value);
				if (it != vector.end()) return it - vector.begin();
				else return 0;
				};

			if (removeOld)
			{
				auto it = std::remove(vector.begin(), vector.end(), value);
				const auto numRemoved = std::distance(vector.end() - it);
				vector.erase(it, vector.end());
				vector.push_back(value);
				return numRemoved;
			}
			else
			{
				const int index = findIndex();
				if (index != 0)
				{
					return index;
				}
				else
				{
					vector.push_back(value);
					return 0;
				}
			}
		}

		template<class T>
		static bool Contains(const std::vector<T>& vector, const T& value) { return std::find(vector.begin(), vector.end(), value) != vector.end(); }

		template<class T>
		static int GetIndex(const std::vector<T>& vector, const T& value)
		{
			const auto it = std::find(vector.begin(), vector.end(), value);
			if (it != vector.end())
				return it - vector.begin();
			else
				return -1;
		}

		template<class T>
		static bool SetNewPosition(std::vector<T>& vector, const T& value, int newIndex)
		{
			newIndex = std::clamp(newIndex, 0, (int)vector.size() - 1);

			const int oldIndex = GetIndex(vector, value);

			if (newIndex == oldIndex || oldIndex < 0)
				return false;

			if (newIndex < oldIndex)
			{
				// New position is before the old one
				vector.erase(vector.begin() + oldIndex);
				vector.insert(vector.begin() + newIndex, value);
			}
			else
			{
				// New position is after the old one
				vector.insert(vector.begin() + newIndex + 1, value);
				vector.erase(vector.begin() + oldIndex);
			}
			return true;
		}

		template<class T>
		static bool SetNewPosition(std::vector<T>& vector, int oldIndex, int newIndex)
		{
			return SetNewPosition(vector, vector[oldIndex], newIndex);
		}

	private:
		std::vector<Type, Allocator> order;
		mutable std::shared_mutex mMutex;
	};

} // namespace NotRed