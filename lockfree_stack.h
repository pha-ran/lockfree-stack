#pragma once

#include <windows.h>

template <typename T>
class lockfree_stack final
{
	using pointer_size = unsigned long long;

public:
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
		// todo
	}

public:
	inline void push(T data) noexcept
	{
		// todo
	}

	inline T pop(void) noexcept
	{
		// todo
	}

private:
	struct NODE
	{
		//~NODE(void) = delete;

		NODE* _next;
		T _data;
	};

private:
	pointer_size _top;

#ifdef _DEBUG
	unsigned long _size;
#endif

};
