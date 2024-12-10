#pragma once

#include "lockfree_memory_pool.h"
#include <windows.h>

template <typename T>
class lockfree_stack final
{
	static_assert(alignof(T) <= 16, "alignof(T) > 16");

public:
	using pointer_size = unsigned long long;

	static constexpr pointer_size _user_address_max		= 0x00007ffffffeffff;
	static constexpr pointer_size _user_address_mask	= 0x00007fffffffffff;

	static constexpr pointer_size _top_counter_mask		= 0xffff800000000000;
	static constexpr pointer_size _increment_counter	= 0x0000800000000000;

public:
	inline lockfree_stack(void) noexcept
		: _top(0)
#ifdef _DEBUG
		, _size(0)
#endif
	{
		SYSTEM_INFO si;
		GetSystemInfo(&si);
		if ((pointer_size)si.lpMaximumApplicationAddress ^ _user_address_max) __debugbreak();
	}

	inline ~lockfree_stack(void) noexcept
	{
		NODE* node = (NODE*)(_top & _user_address_mask);

		while (node != nullptr)
		{
			NODE* next = node->_next;
			_node_pool.ofree(node);
			node = next;
		}
	}

public:
	inline void push(T data) noexcept
	{
		NODE* node = _node_pool.oalloc();
		node->_data = data;

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

	inline bool pop(T* out) noexcept
	{
		for (;;)
		{
			pointer_size local_top = _top;
			NODE* node = (NODE*)(local_top & _user_address_mask);
			if (node == nullptr)
				return false;

			pointer_size next_top = (pointer_size)node->_next + (local_top & _top_counter_mask) + _increment_counter;
			pointer_size prev_top = InterlockedCompareExchange(&_top, next_top, local_top);
			if (prev_top == local_top)
			{
				*out = node->_data;
				_node_pool.ofree(node);
#ifdef _DEBUG
				InterlockedDecrement(&_size);
#endif
				return true;
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
	lockfree_memory_pool<NODE> _node_pool;

#ifdef _DEBUG
	unsigned long _size;
#endif

};
