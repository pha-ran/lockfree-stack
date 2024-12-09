#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif

#include "lockfree_stack.h"
#include <windows.h>
#include <process.h>
#include <stdio.h>
#include <conio.h>

#pragma comment(lib, "winmm.lib")

#define THREAD_COUNT	4
#define FOR_COUNT		10

bool start = 0;
lockfree_stack<int> ls;

unsigned __stdcall worker_func(void*)
{
	int worker_id = GetCurrentThreadId();

	printf("[WORKER] %d\n", worker_id);

	while (!start);

	for (;;)
	{
		for (int i = 0; i < FOR_COUNT; ++i)
			ls.push(worker_id);

		for (int i = 0; i < FOR_COUNT; ++i)
			ls.pop();

		if (!start) break;
	}

	Sleep(3000);

	return 0;
}

int wmain(void) noexcept
{
#ifdef _DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	HANDLE handles[THREAD_COUNT];

	timeBeginPeriod(1);
	for (int i = 0; i < THREAD_COUNT; ++i)
		handles[i] = (HANDLE)_beginthreadex(nullptr, 0, worker_func, nullptr, 0, nullptr);
	system("pause");
	start = true;

	for (;;)
	{
		char c = _getch();

		if (c == 'q')
		{
			start = 0;
			break;
		}
	}

	DWORD wait = WaitForMultipleObjects(THREAD_COUNT, handles, TRUE, INFINITE);
	if (wait != WAIT_OBJECT_0) __debugbreak();
	for (int i = 0; i < THREAD_COUNT; ++i)
#pragma warning(suppress:6001)
#pragma warning(suppress:28183)
		CloseHandle(handles[i]);
	timeEndPeriod(1);

	return 0;
}
