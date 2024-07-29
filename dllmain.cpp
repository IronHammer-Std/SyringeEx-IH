// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "pch.h"
#include <windows.h>
#include <winternl.h>
#include <cstdio>
#include <string>
#include <vector>
#include <unordered_map>
#include "LogLite.h"

struct _USTRING
{
	unsigned short Len;
	unsigned short MaxLen;
	wchar_t* Buf;
};

struct SharedMemRecord
{
	wchar_t Name[256];
	DWORD BaseAddr;
	DWORD TargetHash;
	int Reserved[14];
};

DWORD* _PEB;
DWORD ModuleListHeader()
{
	__asm
	{
		push eax
		mov eax, fs: [0x30]
		mov _PEB, eax
		pop eax
	}
	return *(_PEB + 0x03) + 0x0C;
}

DWORD QuickHashCStrUpper(const char* str)
{
	DWORD Result = 0;
	DWORD Mod = 19260817;
	for (const char* ps = str; *ps; ++ps)
	{
		Result *= Mod;
		Result += (DWORD)toupper(*ps);
	}
	return Result + strlen(str);
}

std::string UnicodetoANSI(const std::wstring& Unicode)
{
	int ANSIlen = WideCharToMultiByte(CP_ACP, 0, Unicode.c_str(), -1, 0, 0, 0, 0);// 获取UTF-8编码长度
	char* ANSI = new CHAR[ANSIlen + 4]{};
	WideCharToMultiByte(CP_ACP, 0, Unicode.c_str(), -1, ANSI, ANSIlen, 0, 0); //转换成UTF-8编码
	std::string ret = ANSI;
	delete[] ANSI;
	return ret;
}

DWORD QuickHashWStrUpper(const std::wstring& Unicode)
{
	return QuickHashCStrUpper(UnicodetoANSI(Unicode).c_str());
}

DWORD QuickHashCWStrUpper(const wchar_t* Unicode)
{
	return QuickHashCStrUpper(UnicodetoANSI(Unicode).c_str());
}

std::unordered_map <DWORD, int>Targets;
//Base Addr: 0x888808 RulesClass::Instance.align_1628[0]
DWORD ppHeaderAddr = 0x888808;
DWORD* ppHeader = (DWORD*)ppHeaderAddr;
std::vector<SharedMemRecord>LibBase;
RemoteMapper MapperAlt;

void PrintModuleList(DWORD Header)
{
	Open();
	WriteLine("开始加载模块信息。Syringe.exe进入等待。");

	//static wchar_t ws[1000];
	_LIST_ENTRY* p, * Head;
	p = Head = ((_LIST_ENTRY*)Header)->Flink;
	RemoteMapper Mapper;
	int Suffix = (int)*ppHeader;

	WriteLine("配置所需信息……");

	Mapper.Open(Suffix, "SYRINGE");
	if (!Mapper.Available())return;

	WriteLine("已打开共享内存。");

	auto pHeader = Mapper.OffsetPtr<SharedMemHeader>(0);
	auto pArr = Mapper.OffsetPtr<SharedMemRecord>(sizeof(SharedMemHeader));
	for (int i = 0; i < pHeader->RecordCount; i++)
	{
		Targets[pArr[i].TargetHash] = i;
	}

	WriteLine("已读入共享内存。");

	int Loaded = 0;
	do
	{
		_USTRING* Name = (_USTRING*)(((int)p) + 0x2C);
		int Base = *((int*)(((int)p) + 0x18));
		//Name->Len : 占字节数，不算尾部0 MaxLen算尾部0
		//if (Name->Buf)swprintf(ws, 1000, L"%s %d %d : 0x%08X Hash %d", Name->Buf, Name->Len, Name->MaxLen, Base, QuickHashCWStrUpper(Name->Buf));
		//else swprintf(ws, 1000, L"NULL : 0x%08X", Base);
		//MessageBoxW(NULL, ws, L"remote", MB_OK);
		if (!Name->Buf)break;

		WriteLine("已读入DLL信息： %ls ：0x%08X。", Name->Buf, Base);

		LibBase.emplace_back();
		auto& b = LibBase.back();
		b.TargetHash = QuickHashCWStrUpper(Name->Buf);
		b.BaseAddr = (DWORD)Base;
		lstrcpynW(b.Name, Name->Buf, 255);

		auto it = Targets.find(QuickHashCWStrUpper(Name->Buf));
		if (it != Targets.end())
		{
			pArr[it->second].BaseAddr = (DWORD)Base;
			lstrcpynW(pArr[it->second].Name, Name->Buf, 255);
			++Loaded;
			//if (Loaded >= pHeader->RecordCount)break;
		}
		p = p->Flink;
	} while (p != Head);

	WriteLine("已读入全部DLL信息。");

	Mapper.Header()->DllRecordCount = (int)LibBase.size();
	Mapper.Header()->DllRecordAddr = (DWORD)LibBase.data();

	WriteLine("向共享内存提交信息。");

	pHeader->WritingComplete = 1;

	WriteLine("成功加载模块信息。Syringe.exe结束等待。");
	Close();
}

DWORD Initialize(LPVOID R)
{
	return 0;
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
	switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
		PrintModuleList(ModuleListHeader());
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

