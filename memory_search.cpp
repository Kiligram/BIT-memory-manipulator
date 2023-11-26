// the following code is modified code taken from https://github.com/meemknight/memGrab/tree/master
// and http://kylehalladay.com/blog/2020/05/20/Rendering-With-Notepad.html
#pragma once
#include <iostream>
#include <Windows.h>
#include <TlHelp32.h>
#include <vector>
#include <algorithm>
#include <sstream>
#include <winnt.h>
#include <string>

#undef min
#undef max
//using PID = DWORD;
//using PROCESS = HANDLE;

enum
{
	memQueryFlags_ = 0,
	memQueryFlags_None = 0,
	memQueryFlags_Read = 0b0001,
	memQueryFlags_Write = 0b0010,
	memQueryFlags_Execute = 0b0100,

};

inline bool readMemory(HANDLE process, void* start, size_t size, void* buff)
{
	SIZE_T readSize = 0;
	return ReadProcessMemory(process, start, buff, size, &readSize);
}

struct OppenedQuery
{
	HANDLE queriedProcess = 0;
	char* baseQueriedPtr = (char*)0x0;
	bool oppened()
	{
		return queriedProcess != 0;
	}
};

inline OppenedQuery initVirtualQuery(HANDLE process)
{
	OppenedQuery q = {};

	q.queriedProcess = process;
	q.baseQueriedPtr = 0;
	return q;
}

inline bool getNextQuery(OppenedQuery& query, void*& low, void*& hi, int& flags)
{

	if (query.queriedProcess == 0) { return false; }

	flags = memQueryFlags_None;
	low = nullptr;
	hi = nullptr;

	MEMORY_BASIC_INFORMATION memInfo;

	bool rez = 0;
	while (true)
	{
		rez = VirtualQueryEx(query.queriedProcess, (void*)query.baseQueriedPtr, &memInfo, sizeof(MEMORY_BASIC_INFORMATION));

		if (!rez)
		{
			query = {};
			return false;
		}

		query.baseQueriedPtr = (char*)memInfo.BaseAddress + memInfo.RegionSize;

		if (memInfo.State == MEM_COMMIT)
		{
			if (memInfo.Protect & PAGE_READONLY)
			{
				flags |= memQueryFlags_Read;
			}

			if (memInfo.Protect & PAGE_READWRITE)
			{
				flags |= (memQueryFlags_Read | memQueryFlags_Write);
			}

			if (memInfo.Protect & PAGE_EXECUTE)
			{
				flags |= memQueryFlags_Execute;
			}

			if (memInfo.Protect & PAGE_EXECUTE_READ)
			{
				flags |= (memQueryFlags_Execute | memQueryFlags_Read);
			}

			if (memInfo.Protect & PAGE_EXECUTE_READWRITE)
			{
				flags |= (memQueryFlags_Execute | memQueryFlags_Read | memQueryFlags_Write);
			}

			if (memInfo.Protect & PAGE_EXECUTE_WRITECOPY)
			{
				flags |= (memQueryFlags_Execute | memQueryFlags_Read);
			}

			if (memInfo.Protect & PAGE_WRITECOPY)
			{
				flags |= memQueryFlags_Read;
			}

			low = memInfo.BaseAddress;
			hi = (char*)memInfo.BaseAddress + memInfo.RegionSize;
			return true;
		}

	}
}

// inspired by http://kylehalladay.com/blog/2020/05/20/Rendering-With-Notepad.html
inline std::vector<void*> findBytePatternInProcessMemory(HANDLE process, void* pattern, size_t patternLen)
{
	if (patternLen == 0) { return {}; }

	std::vector<void*> returnVec;
	returnVec.reserve(1000);

	auto query = initVirtualQuery(process);

	if (!query.oppened())
		return {};

	void* low = nullptr;
	void* hi = nullptr;
	int flags = memQueryFlags_None;

	while (getNextQuery(query, low, hi, flags))
	{
		if ((flags | memQueryFlags_Read) && (flags | memQueryFlags_Write))
		{
			//search for our byte patern
			size_t size = (char*)hi - (char*)low;
			char* localCopyContents = new char[size];
			if (
				readMemory(process, low, size, localCopyContents)
				)
			{
				char* cur = localCopyContents;
				size_t curPos = 0;
				while (curPos < size - patternLen + 1)
				{
					if (memcmp(cur, pattern, patternLen) == 0)
					{
						returnVec.push_back((char*)low + curPos);
					}
					curPos++;
					cur++;
				}
			}
			delete[] localCopyContents;
		}
	}

	return returnVec;
}

inline void refindBytePatternInProcessMemory(HANDLE process, void* pattern, size_t patternLen, std::vector<void*>& found)
{
	if (patternLen == 0) { return; }

	auto newFound = findBytePatternInProcessMemory(process, pattern, patternLen);

	std::vector<void*> intersect;
	intersect.resize(std::min(found.size(), newFound.size()));

	std::set_intersection(found.begin(), found.end(),
		newFound.begin(), newFound.end(),
		intersect.begin());

	intersect.erase(std::remove(intersect.begin(), intersect.end(), nullptr), intersect.end());

	found = std::move(intersect);
}
