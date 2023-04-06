#pragma once
#include <Windows.h>
#pragma warning(disable:4996)


class CDLLInject
{
#define LDR_GET_DLL_HANDLE_EX_UNCHANGED_REFCOUNT 0x00000001

    typedef struct UNICODE_STRING
    {
        USHORT Length;
        USHORT MaximumLength;
        PWSTR  Buffer;
    }*PUNICODE_STRING;

    typedef struct ANSI_STRING
    {
        USHORT Length;
        USHORT MaximumLength;
        PSTR   Buffer;
    }*PANSI_STRING;

    typedef LONG(NTAPI* PFN_LdrUnloadDll)(HANDLE);
    typedef LONG(NTAPI* PFN_LdrGetDllHandleEx)(ULONG Flags, PWSTR DllPath, PULONG DllCharacteristics, PUNICODE_STRING DllName, PVOID* DllHandle);

    typedef LONG(NTAPI* PFN_RtlInitUnicodeString)(PUNICODE_STRING, PCWSTR);
    typedef LONG(NTAPI* PFN_RtlInitAnsiString)(PANSI_STRING, PCSTR);
    typedef LONG(NTAPI* PFN_LdrLoadDll)(PWCHAR, ULONG, PUNICODE_STRING, PHANDLE);
    typedef LONG(NTAPI* PFN_LdrGetProcedureAddress)(PVOID BaseAddress, PANSI_STRING Name, ULONG Ordinal, PVOID* ProcedureAddress);
    typedef int(WINAPI* PFN_MessageBox)(HWND, PCWSTR, PCWSTR, int);
    typedef LPVOID(WINAPI* PFN_CallFun)(PVOID);

    typedef struct _CALL_ARGUMENT_DATA
    {
        // 前面这些是函数地址
        PFN_RtlInitUnicodeString    fnRtlInitUnicodeString;     // 加载unicode字符串
        PFN_LdrLoadDll              fnLdrLoadDll;               // 加载dll的函数
        PFN_LdrGetDllHandleEx       fnLdrGetDllHandleEx;        // 获取模块句柄API
        PFN_LdrGetProcedureAddress  fnLdrGetProcedureAddress;   // 获取函数地址
        PFN_RtlInitAnsiString       fnRtlInitAnsiString;        // 加载ansi字符串


        // 下面是 LdrLoadDll 需要使用的数据, DllName通过RtlInitUnicodeString来提供被 LdrLoadDll 使用
        PWCHAR                      DllPath;                    // LdrLoadDll第一个参数, Dll路径, 可以为0
        ULONG                       Flags;                      // LdrLoadDll第二个参数, 标识
        UNICODE_STRING              UnicodeString;              // LdrLoadDll第三个参数, dll路径 UNICODE_STRING 结构
        HANDLE                      hModule;                    // LdrLoadDll第四个参数, 模块地址, 注入后模块地址保存到这里

        // 下面这些成员是获取函数地址使用的数据
        ANSI_STRING                 AnsiString;                 // 获取函数名的ansi字符串结构
        ULONG                       Ordinal;                    // LdrGetProcedureAddress 第三个参数
        PFN_CallFun                 fun;                        // LdrGetProcedureAddress第四个参数, 被调用的函数名 函数原型, GetProcAddress() 得到的函数

        // dll需要使用的数据
        ULONG                       initRVA;                    // 函数RVA, 有值的话, 加载dll后调用模块基址+RVA这个函数, 函数必须是__stdcall, 有一个参数和返回值, 有值则忽略 funName 成员
        ULONG                       isDebug;                    // 是否调试, 调试的话就弹出信息框
        ULONG                       funArg;                     // 传递到被调用的函数参数
        PVOID                       funRet;                     // 被调用函数的返回值

        // 缓冲区数据, 前面都4/8字节对齐
        WCHAR                       DllName[260];               // dll完整路径
        CHAR                        funName[260];               // 被调用的函数名
        WCHAR                       user32_dll[20];             // user32.dll 这几个文本
        CHAR                        szMessageBoxW[20];          // MessageBoxW 这几个文本
        CHAR                        argData[2000];              // 传递到函数里的数据, 参数数据最大支持2000个字节
    }CALL_ARGUMENT_DATA, * PCALL_ARGUMENT_DATA;


    typedef struct _FREEDLL_ARGUMENT_DATA
    {
        PFN_LdrUnloadDll            pfnLdrUnloadDll;            // 卸载dll的函数地址
        HANDLE                      hModule;                    // 需要卸载的模块地址
    }FREEDLL_ARGUMENT_DATA, *PFREEDLL_ARGUMENT_DATA;

    typedef struct _FREEDLL_ARGUMENT_FILE_DATA
    {
        PFN_LdrUnloadDll            pfnLdrUnloadDll;            // 卸载dll的函数地址
        HANDLE                      hModule;                    // 需要卸载的模块地址, 内部根据模块名获取模块地址存放到这里
        PFN_LdrGetDllHandleEx       pfnLdrGetDllHandleEx;       // 获取模块句柄的函数
        PFN_RtlInitUnicodeString    fnRtlInitUnicodeString;     // 初始化字符串的函数
        UNICODE_STRING              UnicodeString;              // 字符串结构
        WCHAR                       dllName[260];               // 要释放的模块名
    }FREEDLL_ARGUMENT_FILE_DATA, *PFREEDLL_ARGUMENT_FILE_DATA;

    //TODO 需要获取ntdll原始的字节, 绕过APIhook
    // 加载dll并调用指定函数
    static LPVOID WINAPI test_load_call(PCALL_ARGUMENT_DATA data)
    {
        if (data->isDebug)
        {
            // 需要弹信息框, 那就获取信息框的地址, 然后调用
            // LdrGetDllHandleEx() 获取 user32.dll 的模块地址
            // LdrGetProcedureAddress() 获取MessageBoxW 函数地址
            // 然后调用, 只要有一步失败就不处理
            PFN_MessageBox pfn_MessageBox = 0;  // 信息框函数地址
            HANDLE hModule = 0;
            do
            {
                // 获取MessageBoxW函数地址
                data->fnRtlInitUnicodeString(&data->UnicodeString, data->user32_dll);
                if (data->fnLdrGetDllHandleEx(LDR_GET_DLL_HANDLE_EX_UNCHANGED_REFCOUNT, 0, 0, &data->UnicodeString, &hModule) != 0) break;
                if (!hModule) break;

                data->fnRtlInitAnsiString(&data->AnsiString, data->szMessageBoxW);
                data->fnLdrGetProcedureAddress(hModule, &data->AnsiString, 0, (LPVOID*)&pfn_MessageBox);
                if (pfn_MessageBox)
                    pfn_MessageBox(0, data->DllName, 0, 0);
            } while (0);

        }

        if (!data->hModule)
        {
            data->fnRtlInitUnicodeString(&data->UnicodeString, data->DllName);
            data->fnLdrLoadDll(data->DllPath, data->Flags, &data->UnicodeString, &data->hModule);
        }
        if (!data->hModule)
            return 0;
        // 调用函数, 基址 + RVA
        if (data->initRVA)
        {
            data->fun = (PFN_CallFun)(((PUCHAR)data->hModule) + data->initRVA);
        }
        else if (data->funName[0])
        {
            // 没有RVA的话, 那就根据函数名获取函数地址
            // 加载函数名 ANSI_STRING 结构
            // 然后调用 LdrGetProcedureAddress 获取函数名
            data->fnRtlInitAnsiString(&data->AnsiString, data->funName);
            if (data->fnLdrGetProcedureAddress(data->hModule, &data->AnsiString, data->Ordinal, (LPVOID*)&data->fun))
                return 0;
        }
        if (data->fun)
            data->funRet = data->fun(  ( (LPBYTE)data ) + ( data->funArg ) );
        return data->funRet;
    }

    // 释放指定句柄
    static BOOL WINAPI test_free_dll(PFREEDLL_ARGUMENT_DATA data)
    {
        return data->pfnLdrUnloadDll(data->hModule);
    }
    // 释放指定模块名
    static BOOL WINAPI test_free_dll_file(PFREEDLL_ARGUMENT_FILE_DATA data)
    {

        if (data->fnRtlInitUnicodeString(&data->UnicodeString, data->dllName) != 0)
            return false;
        if (data->pfnLdrGetDllHandleEx(LDR_GET_DLL_HANDLE_EX_UNCHANGED_REFCOUNT, 0, 0, &data->UnicodeString, &data->hModule))
            return false;
        return data->pfnLdrUnloadDll(data->hModule);
    }
    static BOOLEAN _make_fun(PCALL_ARGUMENT_DATA data, LPCWSTR dllFileName, LPCSTR funName, LPCVOID pArgData, int nArgSize)
    {
        HMODULE hNtdll = CDLLInject::GetNtdllHandle();
        data->fnRtlInitUnicodeString        = (PFN_RtlInitUnicodeString)    GetProcAddress(hNtdll, "RtlInitUnicodeString");
        data->fnRtlInitAnsiString           = (PFN_RtlInitAnsiString)       GetProcAddress(hNtdll, "RtlInitAnsiString");
        data->fnLdrLoadDll                  = (PFN_LdrLoadDll)              GetProcAddress(hNtdll, "LdrLoadDll");
        data->fnLdrGetProcedureAddress      = (PFN_LdrGetProcedureAddress)  GetProcAddress(hNtdll, "LdrGetProcedureAddress");
        data->fnLdrGetDllHandleEx           = (PFN_LdrGetDllHandleEx)       GetProcAddress(hNtdll, "LdrGetDllHandleEx");

        wcscpy_s(data->DllName, 260, dllFileName);
        if (funName)
            strcpy_s(data->funName, 260, funName);

        wcscpy_s(data->user32_dll, 20, L"user32.dll");
        strcpy_s(data->szMessageBoxW, 20, "MessageBoxW");

        data->DllPath   = NULL;
        data->Flags     = 0;
        data->hModule   = 0;
        data->funArg    = offsetof(CALL_ARGUMENT_DATA, argData);

        if (nArgSize > 2000)
        {
            return FALSE;
        }
        if (pArgData != NULL && nArgSize > 0)
            memcpy(data->argData, pArgData, nArgSize);
        //sizeof(CALL_ARGUMENT_DATA);
        return TRUE;
    }

public:
    // 把dll注入到指定进程上, 返回模块句柄, 失败返回0
    // pid = 进程ID
    // dllFileName = 要注入的dll完整路径, x86进程只能注入x86的dll, x64的进程只能注入x64的dll
    // lpszFunName = 加载dll后需要调用的函数名, 为0则不调用
    // pArgBuffer = 调用函数传递进去的参数数据, 最大支持2000个字节
    // nArgBufSize = 传递参数的尺寸, 单位为字节
    // pCallFuncRet = 接收调用函数的返回值, lpszFunName 为空则不会调用函数, 返回值也为0
    static inline HMODULE InjectDll(DWORD pid, LPCWSTR lpszDllFileName, LPCSTR lpszFunName = 0, PVOID pArgBuffer = 0, int nArgBufSize = 0, PVOID* pCallFuncRet = 0)
    {
        HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, TRUE, pid);
        if (!hProcess)
            return 0;
        HMODULE hModule = InjectDll(hProcess, lpszDllFileName, lpszFunName, pArgBuffer, nArgBufSize, pCallFuncRet);
        CloseHandle(hProcess);
        return hModule;
    }

    // 把dll注入到指定进程上
    // hProcess = 进程句柄, 至少需要读写的权限
    // dllFileName = 要注入的dll完整路径, x86进程只能注入x86的dll, x64的进程只能注入x64的dll
    // lpszFunName = 加载dll后需要调用的函数名, 为0则不调用
    // pArgBuffer = 调用函数传递进去的参数数据, 最大支持2000个字节
    // nArgBufSize = 传递参数的尺寸, 单位为字节
    // pDllModule = 接收加载成功的模块地址
    static inline HMODULE InjectDll(HANDLE hProcess, LPCWSTR lpszDllFileName, LPCSTR lpszFunName = 0, PVOID pArgBuffer = 0, int nArgBufSize = 0, PVOID* pCallFuncRet = 0)
    {
        if (pCallFuncRet)*pCallFuncRet = 0;
        if (!hProcess || !lpszDllFileName || !lpszDllFileName[0]) return 0;

        // opCode, 是 test_load_call() 这个函数 release编译后的代码
        const BYTE opCode[] =
        {
        #ifdef _WIN64
            0x48, 0x89, 0x4c, 0x24, 0x08, 0x48, 0x83, 0xec, 0x48, 0x48, 0x8b, 0x44, 0x24, 0x50, 0x83, 0x78, 0x74, 0x00, 0x0f, 0x84,
            0xd4, 0x00, 0x00, 0x00, 0x48, 0xc7, 0x44, 0x24, 0x38, 0x00, 0x00, 0x00, 0x00, 0x48, 0xc7, 0x44, 0x24, 0x30, 0x00, 0x00,
            0x00, 0x00, 0x48, 0x8b, 0x44, 0x24, 0x50, 0x48, 0x05, 0x94, 0x03, 0x00, 0x00, 0x48, 0x8b, 0x4c, 0x24, 0x50, 0x48, 0x83,
            0xc1, 0x38, 0x48, 0x8b, 0xd0, 0x48, 0x8b, 0x44, 0x24, 0x50, 0xff, 0x10, 0x48, 0x8b, 0x44, 0x24, 0x50, 0x48, 0x83, 0xc0,
            0x38, 0x48, 0x8d, 0x4c, 0x24, 0x30, 0x48, 0x89, 0x4c, 0x24, 0x20, 0x4c, 0x8b, 0xc8, 0x45, 0x33, 0xc0, 0x33, 0xd2, 0xb9,
            0x01, 0x00, 0x00, 0x00, 0x48, 0x8b, 0x44, 0x24, 0x50, 0xff, 0x50, 0x10, 0x85, 0xc0, 0x74, 0x02, 0xeb, 0x76, 0x48, 0x83,
            0x7c, 0x24, 0x30, 0x00, 0x75, 0x02, 0xeb, 0x6c, 0x48, 0x8b, 0x44, 0x24, 0x50, 0x48, 0x05, 0xbc, 0x03, 0x00, 0x00, 0x48,
            0x8b, 0x4c, 0x24, 0x50, 0x48, 0x83, 0xc1, 0x50, 0x48, 0x8b, 0xd0, 0x48, 0x8b, 0x44, 0x24, 0x50, 0xff, 0x50, 0x20, 0x48,
            0x8b, 0x44, 0x24, 0x50, 0x48, 0x83, 0xc0, 0x50, 0x4c, 0x8d, 0x4c, 0x24, 0x38, 0x45, 0x33, 0xc0, 0x48, 0x8b, 0xd0, 0x48,
            0x8b, 0x4c, 0x24, 0x30, 0x48, 0x8b, 0x44, 0x24, 0x50, 0xff, 0x50, 0x18, 0x48, 0x83, 0x7c, 0x24, 0x38, 0x00, 0x74, 0x1a,
            0x48, 0x8b, 0x44, 0x24, 0x50, 0x48, 0x05, 0x88, 0x00, 0x00, 0x00, 0x45, 0x33, 0xc9, 0x45, 0x33, 0xc0, 0x48, 0x8b, 0xd0,
            0x33, 0xc9, 0xff, 0x54, 0x24, 0x38, 0x33, 0xc0, 0x85, 0xc0, 0x0f, 0x85, 0x3e, 0xff, 0xff, 0xff, 0x48, 0x8b, 0x44, 0x24,
            0x50, 0x48, 0x83, 0x78, 0x48, 0x00, 0x75, 0x4f, 0x48, 0x8b, 0x44, 0x24, 0x50, 0x48, 0x05, 0x88, 0x00, 0x00, 0x00, 0x48,
            0x8b, 0x4c, 0x24, 0x50, 0x48, 0x83, 0xc1, 0x38, 0x48, 0x8b, 0xd0, 0x48, 0x8b, 0x44, 0x24, 0x50, 0xff, 0x10, 0x48, 0x8b,
            0x44, 0x24, 0x50, 0x48, 0x83, 0xc0, 0x48, 0x48, 0x8b, 0x4c, 0x24, 0x50, 0x48, 0x83, 0xc1, 0x38, 0x4c, 0x8b, 0xc8, 0x4c,
            0x8b, 0xc1, 0x48, 0x8b, 0x44, 0x24, 0x50, 0x8b, 0x50, 0x30, 0x48, 0x8b, 0x44, 0x24, 0x50, 0x48, 0x8b, 0x48, 0x28, 0x48,
            0x8b, 0x44, 0x24, 0x50, 0xff, 0x50, 0x08, 0x48, 0x8b, 0x44, 0x24, 0x50, 0x48, 0x83, 0x78, 0x48, 0x00, 0x75, 0x07, 0x33,
            0xc0, 0xe9, 0xdc, 0x00, 0x00, 0x00, 0x48, 0x8b, 0x44, 0x24, 0x50, 0x83, 0x78, 0x70, 0x00, 0x74, 0x1c, 0x48, 0x8b, 0x44,
            0x24, 0x50, 0x8b, 0x40, 0x70, 0x48, 0x8b, 0x4c, 0x24, 0x50, 0x48, 0x03, 0x41, 0x48, 0x48, 0x8b, 0x4c, 0x24, 0x50, 0x48,
            0x89, 0x41, 0x68, 0xeb, 0x73, 0xb8, 0x01, 0x00, 0x00, 0x00, 0x48, 0x6b, 0xc0, 0x00, 0x48, 0x8b, 0x4c, 0x24, 0x50, 0x0f,
            0xbe, 0x84, 0x01, 0x90, 0x02, 0x00, 0x00, 0x85, 0xc0, 0x74, 0x59, 0x48, 0x8b, 0x44, 0x24, 0x50, 0x48, 0x05, 0x90, 0x02,
            0x00, 0x00, 0x48, 0x8b, 0x4c, 0x24, 0x50, 0x48, 0x83, 0xc1, 0x50, 0x48, 0x8b, 0xd0, 0x48, 0x8b, 0x44, 0x24, 0x50, 0xff,
            0x50, 0x20, 0x48, 0x8b, 0x44, 0x24, 0x50, 0x48, 0x83, 0xc0, 0x68, 0x48, 0x8b, 0x4c, 0x24, 0x50, 0x48, 0x83, 0xc1, 0x50,
            0x4c, 0x8b, 0xc8, 0x48, 0x8b, 0x44, 0x24, 0x50, 0x44, 0x8b, 0x40, 0x60, 0x48, 0x8b, 0xd1, 0x48, 0x8b, 0x44, 0x24, 0x50,
            0x48, 0x8b, 0x48, 0x48, 0x48, 0x8b, 0x44, 0x24, 0x50, 0xff, 0x50, 0x18, 0x85, 0xc0, 0x74, 0x04, 0x33, 0xc0, 0xeb, 0x42,
            0x48, 0x8b, 0x44, 0x24, 0x50, 0x48, 0x83, 0x78, 0x68, 0x00, 0x74, 0x2a, 0x48, 0x8b, 0x44, 0x24, 0x50, 0x8b, 0x40, 0x78,
            0x48, 0x8b, 0x4c, 0x24, 0x50, 0x48, 0x03, 0xc8, 0x48, 0x8b, 0xc1, 0x48, 0x8b, 0xc8, 0x48, 0x8b, 0x44, 0x24, 0x50, 0xff,
            0x50, 0x68, 0x48, 0x8b, 0x4c, 0x24, 0x50, 0x48, 0x89, 0x81, 0x80, 0x00, 0x00, 0x00, 0x48, 0x8b, 0x44, 0x24, 0x50, 0x48,
            0x8b, 0x80, 0x80, 0x00, 0x00, 0x00, 0x48, 0x83, 0xc4, 0x48, 0xc3

        #else
            0x55, 0x8b, 0xec, 0x83, 0xec, 0x08, 0x8b, 0x45, 0x08, 0x83, 0x78, 0x3c, 0x00, 0x0f, 0x84, 0x9c, 0x00, 0x00, 0x00, 0xc7,
            0x45, 0xf8, 0x00, 0x00, 0x00, 0x00, 0xc7, 0x45, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x8b, 0x4d, 0x08, 0x81, 0xc1, 0x54, 0x03,
            0x00, 0x00, 0x51, 0x8b, 0x55, 0x08, 0x83, 0xc2, 0x1c, 0x52, 0x8b, 0x45, 0x08, 0x8b, 0x08, 0xff, 0xd1, 0x8d, 0x55, 0xfc,
            0x52, 0x8b, 0x45, 0x08, 0x83, 0xc0, 0x1c, 0x50, 0x6a, 0x00, 0x6a, 0x00, 0x6a, 0x01, 0x8b, 0x4d, 0x08, 0x8b, 0x51, 0x08,
            0xff, 0xd2, 0x85, 0xc0, 0x74, 0x02, 0xeb, 0x57, 0x83, 0x7d, 0xfc, 0x00, 0x75, 0x02, 0xeb, 0x4f, 0x8b, 0x45, 0x08, 0x05,
            0x7c, 0x03, 0x00, 0x00, 0x50, 0x8b, 0x4d, 0x08, 0x83, 0xc1, 0x28, 0x51, 0x8b, 0x55, 0x08, 0x8b, 0x42, 0x10, 0xff, 0xd0,
            0x8d, 0x4d, 0xf8, 0x51, 0x6a, 0x00, 0x8b, 0x55, 0x08, 0x83, 0xc2, 0x28, 0x52, 0x8b, 0x45, 0xfc, 0x50, 0x8b, 0x4d, 0x08,
            0x8b, 0x51, 0x0c, 0xff, 0xd2, 0x83, 0x7d, 0xf8, 0x00, 0x74, 0x10, 0x6a, 0x00, 0x6a, 0x00, 0x8b, 0x45, 0x08, 0x83, 0xc0,
            0x48, 0x50, 0x6a, 0x00, 0xff, 0x55, 0xf8, 0x33, 0xc9, 0x0f, 0x85, 0x72, 0xff, 0xff, 0xff, 0x8b, 0x55, 0x08, 0x83, 0x7a,
            0x24, 0x00, 0x75, 0x39, 0x8b, 0x45, 0x08, 0x83, 0xc0, 0x48, 0x50, 0x8b, 0x4d, 0x08, 0x83, 0xc1, 0x1c, 0x51, 0x8b, 0x55,
            0x08, 0x8b, 0x02, 0xff, 0xd0, 0x8b, 0x4d, 0x08, 0x83, 0xc1, 0x24, 0x51, 0x8b, 0x55, 0x08, 0x83, 0xc2, 0x1c, 0x52, 0x8b,
            0x45, 0x08, 0x8b, 0x48, 0x18, 0x51, 0x8b, 0x55, 0x08, 0x8b, 0x42, 0x14, 0x50, 0x8b, 0x4d, 0x08, 0x8b, 0x51, 0x04, 0xff,
            0xd2, 0x8b, 0x45, 0x08, 0x83, 0x78, 0x24, 0x00, 0x75, 0x07, 0x33, 0xc0, 0xe9, 0xa0, 0x00, 0x00, 0x00, 0x8b, 0x4d, 0x08,
            0x83, 0x79, 0x38, 0x00, 0x74, 0x14, 0x8b, 0x55, 0x08, 0x8b, 0x42, 0x24, 0x8b, 0x4d, 0x08, 0x03, 0x41, 0x38, 0x8b, 0x55,
            0x08, 0x89, 0x42, 0x34, 0xeb, 0x5c, 0xb8, 0x01, 0x00, 0x00, 0x00, 0x6b, 0xc8, 0x00, 0x8b, 0x55, 0x08, 0x0f, 0xbe, 0x84,
            0x0a, 0x50, 0x02, 0x00, 0x00, 0x85, 0xc0, 0x74, 0x45, 0x8b, 0x4d, 0x08, 0x81, 0xc1, 0x50, 0x02, 0x00, 0x00, 0x51, 0x8b,
            0x55, 0x08, 0x83, 0xc2, 0x28, 0x52, 0x8b, 0x45, 0x08, 0x8b, 0x48, 0x10, 0xff, 0xd1, 0x8b, 0x55, 0x08, 0x83, 0xc2, 0x34,
            0x52, 0x8b, 0x45, 0x08, 0x8b, 0x48, 0x30, 0x51, 0x8b, 0x55, 0x08, 0x83, 0xc2, 0x28, 0x52, 0x8b, 0x45, 0x08, 0x8b, 0x48,
            0x24, 0x51, 0x8b, 0x55, 0x08, 0x8b, 0x42, 0x0c, 0xff, 0xd0, 0x85, 0xc0, 0x74, 0x04, 0x33, 0xc0, 0xeb, 0x27, 0x8b, 0x4d,
            0x08, 0x83, 0x79, 0x34, 0x00, 0x74, 0x18, 0x8b, 0x55, 0x08, 0x8b, 0x45, 0x08, 0x03, 0x42, 0x40, 0x50, 0x8b, 0x4d, 0x08,
            0x8b, 0x51, 0x34, 0xff, 0xd2, 0x8b, 0x4d, 0x08, 0x89, 0x41, 0x44, 0x8b, 0x55, 0x08, 0x8b, 0x42, 0x44, 0x8b, 0xe5, 0x5d,
            0xc2, 0x04, 0x00

        #endif
        };
        const int opCodeSize = sizeof(opCode) / sizeof(opCode[0]);

        CALL_ARGUMENT_DATA data = { 0 };
        //sizeof(data);
        if (!_make_fun(&data, lpszDllFileName, lpszFunName, pArgBuffer, nArgBufSize))
            return 0;
#ifdef DEBUG_SHELLCODE
        __debugbreak();
        test_load_call(&data);
#endif
        BOOL bRet = FALSE;      // 是否成功
        HANDLE hThread = NULL;  // 远程线程句柄
        LPVOID pAddress = NULL; // 需要执行的字节码和传递进去的参数数据
        HMODULE hModule = 0;
        __try
        {
            // 申请一块内存, 有读写可执行的权限, 需要把字节码存放进去, 并把参数数据也存放进去
            pAddress = VirtualAllocEx(hProcess, NULL, 0x1000, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
            if (pAddress == NULL)
                __leave;

            LPVOID pCode = (LPVOID)(((LPBYTE)pAddress) + 0x0000);
            LPVOID pArg  = (LPVOID)(((LPBYTE)pAddress) + opCodeSize);   // 参数地址是字节码的后面




            // 把函数拷贝到进程上, 到时候创建远程线程就执行pCode这个数据的代码
            BOOL bWriteOK = WriteProcessMemory(hProcess, pCode, opCode, opCodeSize, NULL);
            if (!bWriteOK) __leave;

            // 创建远程线程时传递进去的参数
            bWriteOK = WriteProcessMemory(hProcess, pArg, &data, sizeof(data), NULL);
            if (!bWriteOK) __leave;
            
            //wchar_t dbg[260];
            //swprintf(dbg, 260, L"注入dll, 代码起始地址 = 0x%p\n参数起始地址 = 0x%p", pCode, pArg);
            //MessageBoxW(0, dbg, 0, 0);

            //创建远程线程，把pCode作为线程起始函数，pArg作为参数;
            hThread = NtCreateThreadEx(hProcess, (LPTHREAD_START_ROUTINE)pCode, pArg);
            if (hThread == NULL) __leave;

            // 等待完成
            WaitForSingleObject(hThread, INFINITE);
            
            // 执行完后 data.ModuleHandle 记录加载成功的模块句柄, 需要读出来
            LPVOID pModuleHandle = (LPVOID)((LPBYTE)pArg + offsetof(CALL_ARGUMENT_DATA, hModule));
            SIZE_T NumberOfBytesRead = 0;
            ReadProcessMemory(hProcess, pModuleHandle, &data.hModule, sizeof(data.hModule), &NumberOfBytesRead);
            hModule = (HMODULE)data.hModule;

            if (pCallFuncRet)
            {
                // 获取调用函数后的返回值
                LPVOID pCallRet = (LPVOID)((LPBYTE)pArg + offsetof(CALL_ARGUMENT_DATA, funRet));
                ReadProcessMemory(hProcess, pCallRet, pCallFuncRet, sizeof(PVOID), &NumberOfBytesRead);
            }
            bRet = TRUE;
        }
        __finally
        {
            // 执行完毕, 释放数据
            if (pAddress != NULL)
                VirtualFreeEx(hProcess, pAddress, 0, MEM_RELEASE);
            if (hThread != NULL)
                CloseHandle(hThread);
        }

        return hModule;

    }

    // 在指定进程上释放模块
    // pid = 进程ID
    // dllName = 模块名, 完整路径或者文件名
    static inline BOOL InjectFreeLibrary(DWORD pid, LPCWSTR dllName)
    {
        HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, TRUE, pid);
        if (!hProcess)
            return false;
        BOOL ret = InjectFreeLibrary(hProcess, dllName);
        CloseHandle(hProcess);
        return ret;
    }

    // 在指定进程上释放模块
    // hProcess = 进程句柄, 至少需要读写的权限
    // dllName = 模块名, 完整路径或者文件名
    static inline BOOL InjectFreeLibrary(HANDLE hProcess, LPCWSTR dllName)
    {
        const BYTE opCode[] =
        {
            // return data->pfnFreeLibrary(data->pfnGetModuleHandleW(data->dllName));
            // 下面这些字节码是上面这行代码编译的结果
#ifdef _WIN64
            0x48, 0x89, 0x4c, 0x24, 0x08, 0x48, 0x83, 0xec, 0x38, 0x48, 0x8b, 0x44, 0x24, 0x40, 0x48, 0x83, 0xc0, 0x30, 0x48, 0x8b,
            0x4c, 0x24, 0x40, 0x48, 0x83, 0xc1, 0x20, 0x48, 0x8b, 0xd0, 0x48, 0x8b, 0x44, 0x24, 0x40, 0xff, 0x50, 0x18, 0x85, 0xc0,
            0x74, 0x04, 0x33, 0xc0, 0xeb, 0x44, 0x48, 0x8b, 0x44, 0x24, 0x40, 0x48, 0x83, 0xc0, 0x08, 0x48, 0x8b, 0x4c, 0x24, 0x40,
            0x48, 0x83, 0xc1, 0x20, 0x48, 0x89, 0x44, 0x24, 0x20, 0x4c, 0x8b, 0xc9, 0x45, 0x33, 0xc0, 0x33, 0xd2, 0xb9, 0x01, 0x00,
            0x00, 0x00, 0x48, 0x8b, 0x44, 0x24, 0x40, 0xff, 0x50, 0x10, 0x85, 0xc0, 0x74, 0x04, 0x33, 0xc0, 0xeb, 0x10, 0x48, 0x8b,
            0x44, 0x24, 0x40, 0x48, 0x8b, 0x48, 0x08, 0x48, 0x8b, 0x44, 0x24, 0x40, 0xff, 0x10, 0x48, 0x83, 0xc4, 0x38, 0xc3
#else
            0x55, 0x8b, 0xec, 0x8b, 0x45, 0x08, 0x83, 0xc0, 0x18, 0x50,
            0x8b, 0x4d, 0x08, 0x83, 0xc1, 0x10, 0x51, 0x8b, 0x55, 0x08,
            0x8b, 0x42, 0x0c, 0xff, 0xd0, 0x85, 0xc0, 0x74, 0x04, 0x33,
            0xc0, 0xeb, 0x32, 0x8b, 0x4d, 0x08, 0x83, 0xc1, 0x04, 0x51,
            0x8b, 0x55, 0x08, 0x83, 0xc2, 0x10, 0x52, 0x6a, 0x00, 0x6a,
            0x00, 0x6a, 0x01, 0x8b, 0x45, 0x08, 0x8b, 0x48, 0x08, 0xff,
            0xd1, 0x85, 0xc0, 0x74, 0x04, 0x33, 0xc0, 0xeb, 0x0e, 0x8b,
            0x55, 0x08, 0x8b, 0x42, 0x04, 0x50, 0x8b, 0x4d, 0x08, 0x8b,
            0x11, 0xff, 0xd2, 0x5d, 0xc2, 0x04, 0x00
#endif
        };
        const int opCodeSize = sizeof(opCode) / sizeof(opCode[0]);
        FREEDLL_ARGUMENT_FILE_DATA data;
        data.pfnLdrUnloadDll        = (PFN_LdrUnloadDll)        GetProcAddress(GetNtdllHandle(), "LdrUnloadDll");
        data.pfnLdrGetDllHandleEx   = (PFN_LdrGetDllHandleEx)   GetProcAddress(GetNtdllHandle(), "LdrGetDllHandleEx");
        data.fnRtlInitUnicodeString = (PFN_RtlInitUnicodeString)GetProcAddress(GetNtdllHandle(), "RtlInitUnicodeString");
        wcscpy_s(data.dllName, dllName);
        return InjectFunction(hProcess, opCode, opCodeSize, &data, sizeof(data));
    }

    // 在指定进程上释放模块
    // pid = 进程ID
    // hModule = InjectDll 返回的值
    static inline BOOL InjectFreeLibrary(DWORD pid, HANDLE hModule)
    {
        HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, TRUE, pid);
        if (!hProcess)
            return 0;
        BOOL ret = InjectFreeLibrary(hProcess, hModule);
        CloseHandle(hProcess);
        return ret;
    }

    // 在指定进程上释放模块
    // hProcess = 进程句柄, 至少需要读写的权限
    // hModule = InjectDll 返回的值
    static inline BOOL InjectFreeLibrary(HANDLE hProcess, HANDLE hModule)
    {
        const BYTE opCode[] =
        {
            // return data->pfnFreeLibrary(data->hModule);
            // 下面这些字节码是上面这行代码编译的结果
#ifdef _WIN64
            0x48, 0x89, 0x4c, 0x24, 0x08, 0x48, 0x83, 0xec, 0x28, 0x48,
            0x8b, 0x44, 0x24, 0x30, 0x48, 0x8b, 0x48, 0x08, 0x48, 0x8b,
            0x44, 0x24, 0x30, 0xff, 0x10, 0x48, 0x83, 0xc4, 0x28, 0xc3
#else
            0x55, 0x8b, 0xec, 0x8b, 0x45, 0x08, 0x8b, 0x48, 0x04, 0x51,
            0x8b, 0x55, 0x08, 0x8b, 0x02, 0xff, 0xd0, 0x5d, 0xc2, 0x04, 0x00
#endif
        };
        const int opCodeSize = sizeof(opCode) / sizeof(opCode[0]);
        FREEDLL_ARGUMENT_DATA data;
        data.hModule = hModule;
        data.pfnLdrUnloadDll = (PFN_LdrUnloadDll)GetProcAddress(GetNtdllHandle(), "LdrUnloadDll");
        DWORD ret = 0;
        BOOL isOk = InjectFunction(hProcess, opCode, opCodeSize, &data, sizeof(data), &ret);
        if (isOk && ret == 0)
            return true;
        return false;
    }
    // 向指定进程注入一个函数并调用, 远程创建线程, 执行 opCode
    // pid = 进程ID
    // opCode = 函数的字节码, 请保证这段代码执行不会出错, 否则目标进程可能会崩溃
    // opCodeSize = 字节码长度
    // pArg = 调用的参数, 请注意, 不是把这个地址传递过去, 而是在目标进程申请内存保存这块数据
    // nArgSize = 参数占用字节数
    // pThreadRet = 线程的返回值
    static inline BOOL InjectFunction(DWORD pid, LPCVOID opCode, int opCodeSize, LPCVOID pFunArg, int nFunArgSize, DWORD* pThreadRet = 0)
    {
        HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, TRUE, pid);
        if (!hProcess)
            return false;
        BOOL ret = InjectFunction(hProcess, opCode, opCodeSize, pFunArg, nFunArgSize);
        CloseHandle(hProcess);
        return ret;
    }

    // 向指定进程注入一个函数并调用, 远程创建线程, 执行 opCode
    // hProcess = 进程句柄, 至少需要读写执行的权限
    // opCode = 函数的字节码, 请保证这段代码执行不会出错, 否则目标进程可能会崩溃
    // opCodeSize = 字节码长度
    // pArg = 调用的参数, 请注意, 不是把这个地址传递过去, 而是在目标进程申请内存保存这块数据
    // nArgSize = 参数占用字节数
    // pThreadRet = 线程的返回值
    static inline BOOL InjectFunction(HANDLE hProcess, LPCVOID opCode, int opCodeSize, LPCVOID pFunArg, int nFunArgSize, DWORD* pThreadRet = 0)
    {
        if (!hProcess || !opCode || opCodeSize <= 0)
            return 0;

        int size = opCodeSize + nFunArgSize;
        {   // 处理一下申请的内存尺寸, 4k对齐
            double n = ((double)size) / 0x1000;
            int i = ((int)(n)) / 0x1000;
            if (n > 0) i++;
            size = i * 0x1000;
            if (size < 0x1000)
                size = 0x1000;
        }
        BOOL bRet = FALSE;      // 是否成功
        HANDLE hThread = NULL;  // 远程线程句柄
        LPVOID pAddress = NULL; // 需要执行的字节码和传递进去的参数数据
        __try
        {
            // 申请一块内存, 有读写可执行的权限, 需要把字节码存放进去, 并把参数数据也存放进去
            pAddress = VirtualAllocEx(hProcess, NULL, size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
            if (pAddress == NULL)
                __leave;

            LPVOID pCode = (LPVOID)(((LPBYTE)pAddress) + 0x0000);
            LPVOID pArg  = (LPVOID)(((LPBYTE)pAddress) + opCodeSize);   // 参数地址是字节码的后面

            // 把函数拷贝到进程上, 到时候创建远程线程就执行pCode这个数据的代码
            BOOL bWriteOK = WriteProcessMemory(hProcess, pCode, opCode, opCodeSize, NULL);
            if (!bWriteOK) __leave;

            // 创建远程线程时传递进去的参数
            if (pFunArg)
            {
                bWriteOK = WriteProcessMemory(hProcess, pArg, pFunArg, nFunArgSize, NULL);
                if (!bWriteOK) __leave;
            }

            //创建远程线程，把pCode作为线程起始函数，pArg作为参数;
            hThread = NtCreateThreadEx(hProcess, (LPTHREAD_START_ROUTINE)pCode, pArg);
            if (hThread == NULL) __leave;

            // 等待完成
            WaitForSingleObject(hThread, INFINITE);
            bRet = TRUE;
            GetExitCodeThread(hThread, pThreadRet);
        }
        __finally
        {
            if (pAddress != NULL)
                VirtualFreeEx(hProcess, pAddress, 0, MEM_RELEASE);
            if (hThread != NULL)
                CloseHandle(hThread);
        }

        return bRet;
    }

    // 获取ntdll.dll的模块句柄
    inline static HMODULE GetNtdllHandle()
    {
        static HMODULE hNtdll = 0;
        if (!hNtdll)
        {
            hNtdll = GetModuleHandleW(L"ntdll.dll");
            // 先提个权限, 提高成功率
            EnableDebugPrivilege();
        }
        return hNtdll;
    }

private:

    // 提升自身进程权限, 返回是否成功
    inline static BOOL EnableDebugPrivilege()
    {
        HANDLE hToken;
        TOKEN_PRIVILEGES tkp = { 0 };
        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
            return(FALSE);
        LookupPrivilegeValueW(NULL, SE_DEBUG_NAME, &tkp.Privileges[0].Luid);
        tkp.PrivilegeCount = 1;
        tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
        AdjustTokenPrivileges(hToken, FALSE, &tkp, sizeof(TOKEN_PRIVILEGES), (PTOKEN_PRIVILEGES)NULL, 0);
        CloseHandle(hToken);
        if (GetLastError() != ERROR_SUCCESS)
            return FALSE;

        return TRUE;
    }
    //操作系统版本判断
    inline static BOOL IsVistaOrLater()
    {
        OSVERSIONINFO osvi;
        memset(&osvi, 0, sizeof(OSVERSIONINFO));
        osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
        GetVersionExW(&osvi);
        if (osvi.dwMajorVersion >= 6)
            return TRUE;
        return FALSE;
    }

public:
    // 创建远程线程, 返回线程句柄
    // hProcess = 进程句柄, 需要在这个进程上创建一条线程
    // lpStartAddress = 线程执行的函数
    // lpParameter = 线程参数
    inline static HANDLE NtCreateThreadEx(HANDLE hProcess, LPTHREAD_START_ROUTINE lpStartAddress, LPVOID lpParameter, int* err = 0)
    {
        if (err)*err = 0;
        typedef struct _OBJECT_ATTRIBUTES {
            ULONG Length;
            HANDLE RootDirectory;
            PUNICODE_STRING ObjectName;
            ULONG Attributes;
            PVOID SecurityDescriptor;
            PVOID SecurityQualityOfService;
        } OBJECT_ATTRIBUTES;
        typedef OBJECT_ATTRIBUTES* POBJECT_ATTRIBUTES;

        typedef DWORD64(WINAPI* PFN_NtCreateThreadEx)(
            __out PHANDLE ThreadHandle,
            __in ACCESS_MASK DesiredAccess,
            __in_opt POBJECT_ATTRIBUTES ObjectAttributes,
            __in HANDLE ProcessHandle,
            __in LPTHREAD_START_ROUTINE lpStartAddress,
            __in_opt PVOID lpParameter,
            __in ULONG dwCreationFlags,
            __in_opt ULONG_PTR StackZeroBits,
            __in_opt SIZE_T StackCommit,
            __in_opt SIZE_T StackReserve,
            __in_opt PVOID  AttributeList
            );

        HANDLE hThread = NULL;

        int ret = 0;
        if (IsVistaOrLater())// Vista, 7, Server2008
        {
            static PFN_NtCreateThreadEx pFunc = (PFN_NtCreateThreadEx)GetProcAddress(GetNtdllHandle(), "NtCreateThreadEx");
            if (pFunc)
                ret = (int)pFunc(&hThread, 0x1FFFFF, NULL, hProcess, lpStartAddress, lpParameter, FALSE, NULL, NULL, NULL, NULL);
            if (err)*err = ret;
        }
        else
        {
            // 2000, XP, Server2003
            hThread = CreateRemoteThread(hProcess, NULL, 0, lpStartAddress, lpParameter, 0, NULL);
            if (!hThread)
            {
                if (err)*err = GetLastError();
            }
        }

        return hThread;
    }

};


// 把dll注入到指定进程上
// hProcess = 进程句柄, 至少需要读写的权限
// dllFileName = 要注入的dll完整路径, x86进程只能注入x86的dll, x64的进程只能注入x64的dll
// lpszFunName = 加载dll后需要调用的函数名, 为0则不调用
// pArgBuffer = 调用函数传递进去的参数数据, 最大支持2000个字节
// nArgBufSize = 传递参数的尺寸, 单位为字节
// pDllModule = 接收加载成功的模块地址
static inline HMODULE InjectDll(DWORD pid, LPCWSTR lpszDllFileName, LPCSTR lpszFunName = 0, PVOID pArgBuffer = 0, int nArgBufSize = 0, PVOID* pCallFuncRet = 0)
{
    CDLLInject x64;
    return x64.InjectDll(pid, lpszDllFileName, lpszFunName, pArgBuffer, nArgBufSize, pCallFuncRet);
}

// 向指定进程注入一个函数并调用, 远程创建线程, 执行 opCode
// hProcess = 进程句柄, 至少需要读写执行的权限
// opCode = 函数的字节码, 请保证这段代码执行不会出错, 否则目标进程可能会崩溃
// opCodeSize = 字节码长度
// pArg = 调用的参数, 请注意, 不是把这个地址传递过去, 而是在目标进程申请内存保存这块数据
// nArgSize = 参数占用字节数
inline BOOL InjectFunction(HANDLE hProcess, LPCVOID opCode, int opCodeSize, LPCVOID pFunArg, int nFunArgSize)
{
    CDLLInject x64;
    return x64.InjectFunction(hProcess, opCode, opCodeSize, pFunArg, nFunArgSize);
}

