#pragma once

#define WIN32_LEAN_AND_MEAN             // 从 Windows 头文件中排除极少使用的内容
// Windows 头文件
#include <windows.h>
#include <string>

struct SharedMemHeader
{
	int TotalSize;
	int WritingComplete;
	int RecordCount;
	int RecordSize;

	DWORD DatabaseAddr;
	DWORD DllRecordAddr;
	int DllRecordCount;
	int Reserved[9];
};

class RemoteMapper
{
private:
	HANDLE hMap;
	LPBYTE View;
	size_t Size;
public:
	RemoteMapper();
	void Create(SharedMemHeader&, int RemoteMapSuffix,const std::string& Prefix);
	void Open(int RemoteMapSuffix, const std::string& Prefix);
	bool Available();
	inline LPBYTE GetView()
	{
		return View;
	}
	template<typename T>
	T* OffsetPtr(size_t Ofs)
	{
		if (GetView())return((T*)(GetView() + Ofs));
		else return nullptr;
	}
	inline SharedMemHeader* Header()
	{
		return OffsetPtr<SharedMemHeader>(0);
	}
	~RemoteMapper();
};
