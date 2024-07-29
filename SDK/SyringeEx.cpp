#include "SyringeEx.h"
#include <YRPP.h>


template<typename T>
struct SyrPArray
{
	size_t N;
	const T* Data;

	SyrPArray() :N(0), Data(nullptr) {}
	SyrPArray(const std::vector<T>& p) :N(p.size()), Data(p.data()) {}
};

struct _USTRING
{
	unsigned short Len;
	unsigned short MaxLen;
	wchar_t* Buf;
};

namespace SyringeData
{
	struct RemoteDataHeader
	{
		int Size;
		int NLib;
		int NAddr;
		int NHook;
		int NMem;

		int ExeDataOffset;
		int LibDataListOffset;
		int AddrDataListOffset;
		int HookDataListOffset;
		int CopyMemListOffset;
		int dwReserved[22];
	};



	RemoteDataHeader* pHeader;
	ExeRemoteData* pExe;

	SyrPArray<LibRemoteData*> pLibArray;
	std::unordered_map<std::string, LibRemoteData*> LibMap;
	std::unordered_map<DWORD, LibRemoteData*> LibMap_ID;
	SyrPArray<AddrRemoteData*> pAddrArray;
	std::unordered_map<DWORD, AddrRemoteData*> AddrMap;
	SyrPArray<HookRemoteData*> pHookArray;
	std::unordered_map<DWORD, HookRemoteData*> HookMap;
	SyrPArray<MemCopyData*> pMemArray;
	std::unordered_map<std::string, MemCopyData*> MemMap;
	std::string SharedMemoryName;
	DWORD SyringeProcID;
	std::unordered_map<std::wstring, DWORD> LoadLib;

	reference<DWORD, 0x888808> pBuffer;
	DWORD Buffer_;
	template<typename T>
	T* BufferOffset(DWORD sz)
	{
		return reinterpret_cast<T*>(Buffer_ + sz);
	}

	DWORD GetSyringeProcID()
	{
		return SyringeProcID;
	}

	DWORD BaseAddress()
	{
		return Buffer_;
	}

	DWORD GetLibBaseAddress(std::wstring Name)
	{
		for (auto& wc : Name)wc = towupper(wc);
		auto it = LoadLib.find(Name);
		if (it != LoadLib.end())return it->second;
		else return 0;
	}

	DWORD GetDatabaseSize()
	{
		if (!pHeader)return 0;
		return (DWORD)pHeader->Size;
	}

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
	DWORD SyringeHash(const char* str)
	{
		DWORD Result = 0;
		DWORD Mod = 19260817;
		for (const char* ps = str; *ps; ++ps)
		{
			Result *= Mod;
			Result += (DWORD)(*ps);
		}
		return Result + strlen(str);
	}
	DWORD SyringeHashUpper(const char* str)
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
	const std::string& UniqueIDByPath()
	{
		static std::string Result{};
		if (!Result.empty())return Result;
		auto id = SyringeHashUpper(ExecutableDirectoryPath().c_str());
		Result = std::to_string(id);
		return Result;
	}

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
	void GetModuleList(DWORD Header)
	{
		_LIST_ENTRY* p, * Head;
		p = Head = ((_LIST_ENTRY*)Header)->Flink;
		int Loaded = 0;
		do
		{
			_USTRING* Name = (_USTRING*)(((int)p) + 0x2C);
			int Base = *((int*)(((int)p) + 0x18));
			if (!Name->Buf)break;
			std::wstring ws = Name->Buf;
			for (auto& wc : ws)wc = towupper(wc);
			LoadLib[ws] = (DWORD)Base;
			p = p->Flink;
		} while (p != Head);
	}


	bool HasInit{ false };
	void InitRemoteData()
	{
		if (HasInit)return;
		SyringeProcID = pBuffer();
		SharedMemoryName = ("SYRINGE" + UniqueIDByPath() + std::to_string((int)pBuffer()));
		auto hMap = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, SharedMemoryName.c_str());
		//::MessageBoxA(NULL, ("SYRINGE" + UniqueIDByPath() + std::to_string((int)pBuffer())).c_str(), "InitRemoteData_02", MB_OK);
		if (!hMap)return;
		auto View = (SharedMemHeader*)MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SharedMemHeader));
		if (!View) {
			CloseHandle(hMap); return;
		}

		Buffer_ = View->DatabaseAddr;
		HasInit = true;
		UnmapViewOfFile(View);
		CloseHandle(hMap);

		pHeader = BufferOffset<RemoteDataHeader>(0);
		GetModuleList(ModuleListHeader());

		pExe = BufferOffset<ExeRemoteData>(pHeader->ExeDataOffset);
		pLibArray.N = pHeader->NLib;
		pLibArray.Data = BufferOffset<LibRemoteData*>(pHeader->LibDataListOffset);
		for (size_t i = 0; i < pLibArray.N; i++)
		{
			LibMap[pLibArray.Data[i]->LibName] = pLibArray.Data[i];
			LibMap_ID[pLibArray.Data[i]->LibID] = pLibArray.Data[i];
		}

		pAddrArray.N = pHeader->NAddr;
		pAddrArray.Data = BufferOffset<AddrRemoteData*>(pHeader->AddrDataListOffset);
		for (size_t i = 0; i < pAddrArray.N; i++)
		{
			AddrMap[pAddrArray.Data[i]->Addr] = pAddrArray.Data[i];
		}
		pHookArray.N = pHeader->NHook;
		pHookArray.Data = BufferOffset<HookRemoteData*>(pHeader->HookDataListOffset);
		for (size_t i = 0; i < pHookArray.N; i++)
		{
			HookMap[pHookArray.Data[i]->HookID] = pHookArray.Data[i];
		}
		pMemArray.N = pHeader->NMem;
		pMemArray.Data = BufferOffset<MemCopyData*>(pHeader->CopyMemListOffset);
		for (size_t i = 0; i < pMemArray.N; i++)
		{
			MemMap[pMemArray.Data[i]->Name] = pMemArray.Data[i];
		}
	}

	ExeRemoteData& GetExeData()
	{
		return *pExe;
	}
	LibRemoteData* GetLibData(const std::string& Name)
	{
		auto it = LibMap.find(Name);
		if (it != LibMap.end())return it->second;
		else return nullptr;
	}
	LibRemoteData* GetLibData(const DWORD LibID)
	{
		auto it = LibMap_ID.find(LibID);
		if (it != LibMap_ID.end())return it->second;
		else return nullptr;
	}

	DWORD GetLibID(const std::string& Name)
	{
		return SyringeHashUpper(Name.c_str());
	}
	DWORD GetHookID(const std::string& Lib, const std::string& Proc)
	{
		static const std::string AnalyzerDelim = "\\*^*\\";
		return SyringeHashUpper((Lib + AnalyzerDelim + Proc).c_str());
	}
	AddrRemoteData* GetAddrData(DWORD Addr)
	{
		auto it = AddrMap.find(Addr);
		if (it != AddrMap.end())return it->second;
		else return nullptr;
	}
	MemCopyData* GetCopyMemData(const std::string& Name)
	{
		auto it = MemMap.find(Name);
		if (it != MemMap.end())return it->second;
		else return nullptr;
	}
	HookRemoteData* GetHookData(const std::string& Lib, const std::string& Proc)
	{
		auto it = HookMap.find(GetHookID(Lib, Proc));
		if (it != HookMap.end())return it->second;
		else return nullptr;
	}
	HookRemoteData* GetHookData(const DWORD HookID)
	{
		auto it = HookMap.find(HookID);
		if (it != HookMap.end())return it->second;
		else return nullptr;
	}

	

}