
以下内容讲解了有关如何使用SyringeEx库，以提供完整的运行前信息及扩展钩子的支持。


使用SyringeEx：
为了使用运行前信息，请先配置：
DllMain当中（必须是DllMain！）应该调用SyringeData::InitRemoteData()，例如：

BOOL APIENTRY DllMain( HMODULE hModule,
					   DWORD  ul_reason_for_call,
					   LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		SyringeData::InitRemoteData();
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

在SyringeData::InitRemoteData()调用之后，全部的运行前信息就可用了

SyringeEx.h中的内容：
以下内容只需 #include "SyringeEx.h" 即可使用

宏：

宏的使用不需要InitRemoteData调用过。
DEFINE_HOOKEX：
	参数：hook, funcname, size, priority, sub_priority
	前三个参数同DEFINE_HOOK
	priority：优先级，int类型，优先级越大执行越靠前，对一般的HOOK默认值为100000
	sub_priority：次优先级，const char*类型（长度不大于255，建议全ASCII），在优先级相等时生效，对一般的HOOK默认值为""
	优先级判断规则：
	1、比较priority，值大的先执行
	2、若priority相等，比较sub_priority，
		（1）sub_priority非空的比sub_priority为空串的先执行
		（2）两个都非空时，比较字典序，字典序大的先执行（通过strcmp确定字典序）

DEFINE_HOOKEX_AGAIN：
	参数：hook, funcname, size, priority, sub_priority
	参数含义同DEFINE_HOOKEX
	使用方法同DEFINE_HOOK_AGAIN



函数：

以下绝大部分函数的使用需要调用过InitRemoteData后才是可用的。
以下函数包含在SyringeData命名空间当中。

void InitRemoteData();
	初始化函数！一定要在DllMain当中调用后才能使用其余函数！

DWORD SyringeHash(const char* str);
	Syringe内部的哈希函数，在实现上与Syringe保持一致。
	区分大小写。

DWORD SyringeHashUpper(const char* str);
	Syringe内部的哈希函数，在实现上与Syringe保持一致。
	不区分大小写。（由于文件名不区分大小写，大部分地方用的都是这个）

DWORD GetSyringeProcID()
	返回启动这个程序的Syringe.exe的进程ID。
	
const std::string& ExecutableDirectoryPath();
	获取程序的路径。返回值尾部没有斜杠。（其实和Syringe没啥关系，只是搬了Syringe的实现）

const std::string& UniqueIDByPath();
	按照路径生成一个ID。只要程序的路径一定，这个ID就一定。

DWORD BaseAddress();
	返回运行前数据存储区域的基地址。运行前数据在内存上连续占据了一段空间。

ExeRemoteData& GetExeData();
	返回有关Syringe的基础信息。
DWORD GetDatabaseSize();
	返回运行前数据占据的空间。从BaseAddress()返回的基地址起，连续的一段空间都装有运行前数据。

LibRemoteData* GetLibData(const std::string& Name);
	按照名字获取Syringe载入的插件DLL的基础信息。找不到返回nullptr。

LibRemoteData* GetLibData(const DWORD LibID);
	按照ID获取Syringe载入的插件DLL的基础信息。找不到返回nullptr。

DWORD GetLibID(const std::string& Name)
	按照名字获取DLL的ID。可以作为索引。

DWORD GetHookID(const std::string& Lib, const std::string& Proc);
	按照DLL和函数的名字获取钩子的ID。可以作为索引。

AddrRemoteData* GetAddrData(DWORD Addr);
	获取地址处的钩子信息。如果那个位置无钩子，返回nullptr。

MemCopyData* GetCopyMemData(const std::string& Name);
	获取复制的内存块的信息。按照名字索引。找不到返回nullptr。

HookRemoteData* GetHookData(const std::string& Lib, const std::string& Proc);
	获取钩子的信息。按照DLL和函数的名字索引。如果钩了多次，固定返回第一个。找不到返回nullptr。

HookRemoteData* GetHookData(const DWORD HookID);
	获取钩子的信息。按照钩子ID索引。找不到返回nullptr。

DWORD GetLibBaseAddress(std::wstring Name);
	按照名字获取模块的基地址。参数可以为任何载入的DLL，乃至gamemd.exe。不区分大小写。找不到返回0。