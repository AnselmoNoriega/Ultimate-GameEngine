#pragma once

#include <stdint.h>
#include <atomic>

namespace NR
{
	class RefCounted
	{
	public:
		void IncRefCount() const
		{
			++mRefCount;
		}
		void DecRefCount() const
		{
			--mRefCount;
		}

		uint32_t GetRefCount() const { return mRefCount.load(); }

	private:
		mutable std::atomic<uint32_t> mRefCount = 0;
	};

	namespace RefUtils 
	{
		void AddToLiveReferences(void* instance);
		void RemoveFromLiveReferences(void* instance);
		bool IsLive(void* instance);
	}

	template<typename T>
	class Ref
	{
	public:
		Ref()
			: mInstance(nullptr)
		{
		}

		Ref(std::nullptr_t n)
			: mInstance(nullptr)
		{
		}

		Ref(T* instance)
			: mInstance(instance)
		{
			static_assert(std::is_base_of<RefCounted, T>::value, "Class is not RefCounted!");

			IncRef();
		}

		template<typename T2>
		Ref(const Ref<T2>& other)
		{
			mInstance = (T*)other.mInstance;
			IncRef();
		}

		template<typename T2>
		Ref(Ref<T2>&& other)
		{
			mInstance = (T*)other.mInstance;
			other.mInstance = nullptr;
		}

		static Ref<T> CopyWithoutIncrement(const Ref<T>& other)
		{
			Ref<T> result = nullptr;
			result->mInstance = other.mInstance;
			return result;
		}

		~Ref()
		{
			DecRef();
		}

		Ref(const Ref<T>& other)
			: mInstance(other.mInstance)
		{
			IncRef();
		}

		Ref& operator=(std::nullptr_t)
		{
			DecRef();
			mInstance = nullptr;
			return *this;
		}

		Ref& operator=(const Ref<T>& other)
		{
			other.IncRef();
			DecRef();

			mInstance = other.mInstance;
			return *this;
		}

		template<typename T2>
		Ref& operator=(const Ref<T2>& other)
		{
			other.IncRef();
			DecRef();

			mInstance = other.mInstance;
			return *this;
		}

		template<typename T2>
		Ref& operator=(Ref<T2>&& other)
		{
			DecRef();

			mInstance = other.mInstance;
			other.mInstance = nullptr;
			return *this;
		}

		operator bool() { return mInstance != nullptr; }
		operator bool() const { return mInstance != nullptr; }

		T* operator->() { return mInstance; }
		const T* operator->() const { return mInstance; }

		T& operator*() { return *mInstance; }
		const T& operator*() const { return *mInstance; }

		T* Raw() { return  mInstance; }
		const T* Raw() const { return  mInstance; }

		void Reset(T* instance = nullptr)
		{
			DecRef();
			mInstance = instance;
		}

		template<typename T2>
		Ref<T2> As() const
		{
			return Ref<T2>(*this);
		}

		template<typename... Args>
		static Ref<T> Create(Args&&... args)
		{
			return Ref<T>(new T(std::forward<Args>(args)...));
		}

		bool operator==(const Ref<T>& other) const
		{
			return mInstance == other.mInstance;
		}

		bool operator!=(const Ref<T>& other) const
		{
			return !(*this == other);
		}

		bool EqualsObject(const Ref<T>& other)
		{
			if (!mInstance || !other.mInstance)
			{
				return false;
			}

			return *mInstance == *other.mInstance;
		}

	private:
		void IncRef() const
		{
			if (mInstance)
			{
				mInstance->IncRefCount();
				RefUtils::AddToLiveReferences((void*)mInstance);
			}
		}

		void DecRef() const
		{
			if (mInstance)
			{
				mInstance->DecRefCount();
				if (mInstance->GetRefCount() == 0)
				{
					delete mInstance;
					RefUtils::RemoveFromLiveReferences((void*)mInstance);
					mInstance = nullptr;
				}
			}
		}

		template<class T2>
		friend class Ref;
		mutable T* mInstance;
	};

	template<typename T>
	class WeakRef
	{
	public:
		WeakRef() = default;

		WeakRef(Ref<T> ref)
		{
			mInstance = ref.Raw();
		}

		WeakRef(T* instance)
		{
			mInstance = instance;
		}
		bool IsValid() const { return mInstance ? RefUtils::IsLive(mInstance) : false; }
		operator bool() const { return IsValid(); }

	private:
		T* mInstance = nullptr;
	};
}