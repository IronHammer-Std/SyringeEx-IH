#pragma once
#include <Syringe.h>
#include <vector>
#include <unordered_map>
#include <string>

#if SYR_VER == 2

#pragma pack(push, 16)
#pragma warning(push)
#pragma warning( disable : 4324)
__declspec(align(32)) struct hookaltdecl {
	unsigned int hookAddr;
	unsigned int hookSize;
	const char* hookName;
	int Priority;
	const char* SubPriorityPtr;
};

static_assert(sizeof(hookaltdecl) == 32);
#pragma warning(pop)
#pragma pack(pop)

#pragma section(".hphks00", read, write)

namespace SyringeData {
	namespace HookAlt {

	};
};

#define declhookex(hook, funcname, size, priority, sub_priority) \
namespace SyringeData { \
	namespace HookAlt { __declspec(allocate(".hphks00")) hookaltdecl _hk__ ## hook ## funcname = { ## hook, ## size, #funcname, ## priority, ## sub_priority }; }; \
};

#endif

#ifndef declhookex
#define declhookex(hook, funcname, size, priority, sub_priority)
#endif // declhookex

//#ifdef DEFINE_HOOK 

#define DEFINE_HOOKEX(hook, funcname, size, priority, sub_priority) \
declhookex(hook, funcname, size, priority, sub_priority) \
EXPORT_FUNC(funcname)

#define DEFINE_HOOKEX_AGAIN(hook, funcname, size, priority, sub_priority) \
declhookex(hook, funcname, size, priority, sub_priority)


//在DllMain当中Init::Initialize()后即可使用
namespace SyringeData
{
	struct ExeRemoteData
	{
		char SyringeVersionStr[256];
		BYTE VMajor;
		BYTE VMinor;
		BYTE VRelease;
		BYTE VBuild;

		char FileName[260];
		char AbsPath[260];
		DWORD BaseAddress;
		DWORD EntryPoint;
	};

	struct LibRemoteData
	{
		const char* LibName;
		const char* AbsPath;
		DWORD LibID;
	};

	struct AddrRemoteData
	{
		DWORD Addr;
		int HookCount;
		DWORD FirstHookIndex;//VLA Header

		inline DWORD HookIdx(int i)
		{
			return *((&FirstHookIndex) + i);
		}
	};
	struct HookRemoteData
	{
		const char* ProcName;
		DWORD HookID;
		DWORD LibID;
		DWORD HookAddress;
		size_t OverrideLength;
	};

	struct MemCopyData
	{
		char* Name;
		void* Addr;
	};

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

	extern std::unordered_map<std::string, LibRemoteData*> LibMap;
	extern std::unordered_map<DWORD, AddrRemoteData*> AddrMap;
	extern std::unordered_map<DWORD, HookRemoteData*> HookMap;
	extern std::unordered_map<std::string, MemCopyData*> MemMap;
	extern std::unordered_map<std::wstring, DWORD> LoadLib;

	DWORD SyringeHash(const char* str);
	DWORD SyringeHashUpper(const char* str);
	DWORD GetSyringeProcID();
	const std::string& ExecutableDirectoryPath();
	const std::string& UniqueIDByPath();//只考虑地址
	DWORD BaseAddress();
	void InitRemoteData();
	ExeRemoteData& GetExeData();
	DWORD GetDatabaseSize();
	LibRemoteData* GetLibData(const std::string& Name);
	LibRemoteData* GetLibData(const DWORD LibID);
	DWORD GetLibID(const std::string& Name);
	DWORD GetHookID(const std::string& Lib, const std::string& Proc);
	AddrRemoteData* GetAddrData(DWORD Addr);
	MemCopyData* GetCopyMemData(const std::string& Name);
	HookRemoteData* GetHookData(const std::string& Lib, const std::string& Proc);
	HookRemoteData* GetHookData(const DWORD HookID);
	DWORD GetLibBaseAddress(std::wstring Name);
}
