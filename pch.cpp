// pch.cpp: 与预编译标头对应的源文件

#include "pch.h"
#include <string>
#include <vector>
// 当使用预编译的头时，需要使用此源文件，编译才能成功。



const std::string& ExecutableDirectoryPath()
{
	static std::string ss;
	if (!ss.empty())return ss;
	std::vector<char> full_path_exe(MAX_PATH);

	for (;;)
	{
		const DWORD result = GetModuleFileNameA(NULL,
			&full_path_exe[0],
			full_path_exe.size());

		if (result == 0)
		{
			// Report failure to caller. 
		}
		else if (full_path_exe.size() == result)
		{
			// Buffer too small: increase size. 
			full_path_exe.resize(full_path_exe.size() * 2);
		}
		else
		{
			// Success. 
			break;
		}
	}

	// Remove executable name. 
	std::string result(full_path_exe.begin(), full_path_exe.end());
	std::string::size_type i = result.find_last_of("\\/");
	if (std::string::npos != i) result.erase(i);

	ss = result;
	return ss;
}

DWORD QuickHashCStrUpper(const char* str);

const std::string& UniqueIDByPath()
{
	static std::string Result{};
	if (!Result.empty())return Result;
	auto id = QuickHashCStrUpper(ExecutableDirectoryPath().c_str());
	Result = std::to_string(id);
	return Result;
}
void RemoteMapper::Create(SharedMemHeader& rcd, int RemoteMapSuffix, const std::string& Prefix)
{
	if (!rcd.TotalSize)return;
	hMap = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, rcd.TotalSize, (Prefix + UniqueIDByPath() + std::to_string(RemoteMapSuffix)).c_str());
	MessageBoxA(NULL, (Prefix + UniqueIDByPath() + std::to_string(RemoteMapSuffix)).c_str(), "GameMD Side", MB_OK);
	if (!hMap)return;
	View = (LPBYTE)MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, rcd.TotalSize);
	if (!View)return;
	memcpy(View, &rcd, sizeof(SharedMemHeader));
	Size = rcd.TotalSize;
}
void RemoteMapper::Open(int RemoteMapSuffix, const std::string& Prefix)
{
	hMap = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, (Prefix + UniqueIDByPath() + std::to_string(RemoteMapSuffix)).c_str());
	if (!hMap)return;
	View = (LPBYTE)MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SharedMemHeader));
	if (!View)return;
	auto pHeader = Header();
	Size = pHeader->TotalSize;
	UnmapViewOfFile(View);
	View = (LPBYTE)MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, Size);
	if (!View)Size = 0;
}
bool RemoteMapper::Available()
{
	return View != nullptr;
}
RemoteMapper::RemoteMapper() :hMap(NULL), View(nullptr), Size(0) {}
RemoteMapper::~RemoteMapper()
{
	if (View)UnmapViewOfFile(View);
	if (hMap)CloseHandle(hMap);
}
