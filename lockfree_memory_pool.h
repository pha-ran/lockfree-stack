#pragma once

#include <windows.h>
#include <new>

template <typename T, bool reuse = true>
class lockfree_memory_pool final {};

template <typename T>
class lockfree_memory_pool<T, true> final
{
	static_assert(alignof(T) <= 16, "alignof(T) > 16");

public:
	using pointer_size = unsigned long long;

	static constexpr pointer_size _user_address_max = 0x00007ffffffeffff;
	static constexpr pointer_size _user_address_mask = 0x00007fffffffffff;

	static constexpr pointer_size _top_counter_mask = 0xffff800000000000;
	static constexpr pointer_size _increment_counter = 0x0000800000000000;

public:
	inline lockfree_memory_pool(void) noexcept
		: _top(0)
#ifdef _DEBUG
		, _size(0)
#endif
	{
		SYSTEM_INFO si;
		GetSystemInfo(&si);
		if ((pointer_size)si.lpMaximumApplicationAddress ^ _user_address_max) __debugbreak();
	}

	inline lockfree_memory_pool(unsigned int count) noexcept
		: _top(0)
#ifdef _DEBUG
		, _size(0)
#endif
	{
		SYSTEM_INFO si;
		GetSystemInfo(&si);
		if ((pointer_size)si.lpMaximumApplicationAddress ^ _user_address_max) __debugbreak();

		while (count > 0)
		{
			NODE* temp = new(std::nothrow) NODE;
#pragma warning(suppress:28182)
			temp->_next = (NODE*)_top;
			_top = (pointer_size)temp;
			--count;
#ifdef _DEBUG
			++_size;
#endif
		}
	}

	inline ~lockfree_memory_pool(void) noexcept
	{
		NODE* node = (NODE*)(_top & _user_address_mask);

		while (node != nullptr)
		{
			NODE* next = node->_next;
			delete node;
			node = next;
		}
	}

	lockfree_memory_pool(lockfree_memory_pool&) = delete;
	lockfree_memory_pool(lockfree_memory_pool&&) = delete;
	lockfree_memory_pool& operator=(lockfree_memory_pool&) = delete;
	lockfree_memory_pool& operator=(lockfree_memory_pool&&) = delete;

public:
	inline T* oalloc(void) noexcept
	{
		for (;;)
		{
			pointer_size local_top = _top;
			NODE* node = (NODE*)(local_top & _user_address_mask);
			if (node == nullptr)
				return (T*)((char*)(::new(std::nothrow) NODE) + offsetof(NODE, _data));

			pointer_size next_top = (pointer_size)node->_next + (local_top & _top_counter_mask) + _increment_counter;
			pointer_size prev_top = InterlockedCompareExchange(&_top, next_top, local_top);
			if (prev_top == local_top)
			{
#ifdef _DEBUG
				InterlockedDecrement(&_size);
#endif
				return (T*)((char*)node + offsetof(NODE, _data));
			}
		}
	}

	inline void ofree(T* ptr) noexcept
	{
		NODE* node = (NODE*)((char*)ptr - offsetof(NODE, _data));

		for (;;)
		{
			pointer_size local_top = _top;
			node->_next = (NODE*)(local_top & _user_address_mask);
			pointer_size next_top = (pointer_size)node + (local_top & _top_counter_mask) + _increment_counter;
			pointer_size prev_top = InterlockedCompareExchange(&_top, next_top, local_top);
			if (prev_top == local_top)
			{
#ifdef _DEBUG
				InterlockedIncrement(&_size);
#endif
				return;
			}
		}
	}

private:
	struct NODE
	{
		NODE* _next;
		T _data;
	};

private:
	pointer_size _top;

#ifdef _DEBUG
	unsigned long _size;
#endif

};

template <typename T>
class lockfree_memory_pool<T, false> final
{
	static_assert(alignof(T) <= 16, "alignof(T) > 16");

public:
	using pointer_size = unsigned long long;

	static constexpr pointer_size _user_address_max = 0x00007ffffffeffff;
	static constexpr pointer_size _user_address_mask = 0x00007fffffffffff;

	static constexpr pointer_size _top_counter_mask = 0xffff800000000000;
	static constexpr pointer_size _increment_counter = 0x0000800000000000;

public:
	inline lockfree_memory_pool(void) noexcept
		: _top(0)
#ifdef _DEBUG
		, _size(0)
#endif
	{
		SYSTEM_INFO si;
		GetSystemInfo(&si);
		if ((pointer_size)si.lpMaximumApplicationAddress ^ _user_address_max) __debugbreak();
	}

	inline lockfree_memory_pool(unsigned int count) noexcept
		: _top(0)
#ifdef _DEBUG
		, _size(0)
#endif
	{
		SYSTEM_INFO si;
		GetSystemInfo(&si);
		if ((pointer_size)si.lpMaximumApplicationAddress ^ _user_address_max) __debugbreak();

		while (count > 0)
		{
			NODE* temp = (NODE*)malloc(sizeof(NODE));
			temp->_next = (NODE*)_top;
			_top = (pointer_size)temp;
			--count;
#ifdef _DEBUG
			++_size;
#endif
		}
	}

	inline ~lockfree_memory_pool(void) noexcept
	{
		NODE* node = (NODE*)(_top & _user_address_mask);

		while (node != nullptr)
		{
			NODE* next = node->_next;
			free(node);
			node = next;
		}
	}

	lockfree_memory_pool(lockfree_memory_pool&) = delete;
	lockfree_memory_pool(lockfree_memory_pool&&) = delete;
	lockfree_memory_pool& operator=(lockfree_memory_pool&) = delete;
	lockfree_memory_pool& operator=(lockfree_memory_pool&&) = delete;

public:
	inline T* oalloc(void) noexcept
	{
		NODE* node;

		for (;;)
		{
			pointer_size local_top = _top;
			node = (NODE*)(local_top & _user_address_mask);
			if (node == nullptr)
			{
				node = (NODE*)malloc(sizeof(NODE));
				break;
			}

			pointer_size next_top = (pointer_size)node->_next + (local_top & _top_counter_mask) + _increment_counter;
			pointer_size prev_top = InterlockedCompareExchange(&_top, next_top, local_top);
			if (prev_top == local_top)
			{
#ifdef _DEBUG
				InterlockedDecrement(&_size);
#endif
				break;
			}
		}

		return new(node) T;
	}

	inline void ofree(T* ptr) noexcept
	{
		ptr->~T();
		NODE* node = (NODE*)ptr;

		for (;;)
		{
			pointer_size local_top = _top;
			node->_next = (NODE*)(local_top & _user_address_mask);
			pointer_size next_top = (pointer_size)node + (local_top & _top_counter_mask) + _increment_counter;
			pointer_size prev_top = InterlockedCompareExchange(&_top, next_top, local_top);
			if (prev_top == local_top)
			{
#ifdef _DEBUG
				InterlockedIncrement(&_size);
#endif
				return;
			}
		}
	}

private:
	union NODE
	{
		~NODE(void) = delete;

		NODE* _next;
		T _data;
	};

private:
	pointer_size _top;

#ifdef _DEBUG
	unsigned long _size;
#endif

};
