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
        // ǰ����Щ�Ǻ�����ַ
        PFN_RtlInitUnicodeString    fnRtlInitUnicodeString;     // ����unicode�ַ���
        PFN_LdrLoadDll              fnLdrLoadDll;               // ����dll�ĺ���
        PFN_LdrGetDllHandleEx       fnLdrGetDllHandleEx;        // ��ȡģ����API
        PFN_LdrGetProcedureAddress  fnLdrGetProcedureAddress;   // ��ȡ������ַ
        PFN_RtlInitAnsiString       fnRtlInitAnsiString;        // ����ansi�ַ���


        // ������ LdrLoadDll ��Ҫʹ�õ�����, DllNameͨ��RtlInitUnicodeString���ṩ�� LdrLoadDll ʹ��
        PWCHAR                      DllPath;                    // LdrLoadDll��һ������, Dll·��, ����Ϊ0
        ULONG                       Flags;                      // LdrLoadDll�ڶ�������, ��ʶ
        UNICODE_STRING              UnicodeString;              // LdrLoadDll����������, dll·�� UNICODE_STRING �ṹ
        HANDLE                      hModule;                    // LdrLoadDll���ĸ�����, ģ���ַ, ע���ģ���ַ���浽����

        // ������Щ��Ա�ǻ�ȡ������ַʹ�õ�����
        ANSI_STRING                 AnsiString;                 // ��ȡ��������ansi�ַ����ṹ
        ULONG                       Ordinal;                    // LdrGetProcedureAddress ����������
        PFN_CallFun                 fun;                        // LdrGetProcedureAddress���ĸ�����, �����õĺ����� ����ԭ��, GetProcAddress() �õ��ĺ���

        // dll��Ҫʹ�õ�����
        ULONG                       initRVA;                    // ����RVA, ��ֵ�Ļ�, ����dll�����ģ���ַ+RVA�������, ����������__stdcall, ��һ�������ͷ���ֵ, ��ֵ����� funName ��Ա
        ULONG                       isDebug;                    // �Ƿ����, ���ԵĻ��͵�����Ϣ��
        ULONG                       funArg;                     // ���ݵ������õĺ�������
        PVOID                       funRet;                     // �����ú����ķ���ֵ

        // ����������, ǰ�涼4/8�ֽڶ���
        WCHAR                       DllName[260];               // dll����·��
        CHAR                        funName[260];               // �����õĺ�����
        WCHAR                       user32_dll[20];             // user32.dll �⼸���ı�
        CHAR                        szMessageBoxW[20];          // MessageBoxW �⼸���ı�
        CHAR                        argData[2000];              // ���ݵ������������, �����������֧��2000���ֽ�
    }CALL_ARGUMENT_DATA, * PCALL_ARGUMENT_DATA;


    typedef struct _FREEDLL_ARGUMENT_DATA
    {
        PFN_LdrUnloadDll            pfnLdrUnloadDll;            // ж��dll�ĺ�����ַ
        HANDLE                      hModule;                    // ��Ҫж�ص�ģ���ַ
    }FREEDLL_ARGUMENT_DATA, *PFREEDLL_ARGUMENT_DATA;

    typedef struct _FREEDLL_ARGUMENT_FILE_DATA
    {
        PFN_LdrUnloadDll            pfnLdrUnloadDll;            // ж��dll�ĺ�����ַ
        HANDLE                      hModule;                    // ��Ҫж�ص�ģ���ַ, �ڲ�����ģ������ȡģ���ַ��ŵ�����
        PFN_LdrGetDllHandleEx       pfnLdrGetDllHandleEx;       // ��ȡģ�����ĺ���
        PFN_RtlInitUnicodeString    fnRtlInitUnicodeString;     // ��ʼ���ַ����ĺ���
        UNICODE_STRING              UnicodeString;              // �ַ����ṹ
        WCHAR                       dllName[260];               // Ҫ�ͷŵ�ģ����
    }FREEDLL_ARGUMENT_FILE_DATA, *PFREEDLL_ARGUMENT_FILE_DATA;

    //TODO ��Ҫ��ȡntdllԭʼ���ֽ�, �ƹ�APIhook
    // ����dll������ָ������
    static LPVOID WINAPI test_load_call(PCALL_ARGUMENT_DATA data)
    {
        if (data->isDebug)
        {
            // ��Ҫ����Ϣ��, �Ǿͻ�ȡ��Ϣ��ĵ�ַ, Ȼ�����
            // LdrGetDllHandleEx() ��ȡ user32.dll ��ģ���ַ
            // LdrGetProcedureAddress() ��ȡMessageBoxW ������ַ
            // Ȼ�����, ֻҪ��һ��ʧ�ܾͲ�����
            PFN_MessageBox pfn_MessageBox = 0;  // ��Ϣ������ַ
            HANDLE hModule = 0;
            do
            {
                // ��ȡMessageBoxW������ַ
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
        // ���ú���, ��ַ + RVA
        if (data->initRVA)
        {
            data->fun = (PFN_CallFun)(((PUCHAR)data->hModule) + data->initRVA);
        }
        else if (data->funName[0])
        {
            // û��RVA�Ļ�, �Ǿ͸��ݺ�������ȡ������ַ
            // ���غ����� ANSI_STRING �ṹ
            // Ȼ����� LdrGetProcedureAddress ��ȡ������
            data->fnRtlInitAnsiString(&data->AnsiString, data->funName);
            if (data->fnLdrGetProcedureAddress(data->hModule, &data->AnsiString, data->Ordinal, (LPVOID*)&data->fun))
                return 0;
        }
        if (data->fun)
            data->funRet = data->fun(  ( (LPBYTE)data ) + ( data->funArg ) );
        return data->funRet;
    }

    // �ͷ�ָ�����
    static BOOL WINAPI test_free_dll(PFREEDLL_ARGUMENT_DATA data)
    {
        return data->pfnLdrUnloadDll(data->hModule);
    }
    // �ͷ�ָ��ģ����
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
    // ��dllע�뵽ָ��������, ����ģ����, ʧ�ܷ���0
    // pid = ����ID
    // dllFileName = Ҫע���dll����·��, x86����ֻ��ע��x86��dll, x64�Ľ���ֻ��ע��x64��dll
    // lpszFunName = ����dll����Ҫ���õĺ�����, Ϊ0�򲻵���
    // pArgBuffer = ���ú������ݽ�ȥ�Ĳ�������, ���֧��2000���ֽ�
    // nArgBufSize = ���ݲ����ĳߴ�, ��λΪ�ֽ�
    // pCallFuncRet = ���յ��ú����ķ���ֵ, lpszFunName Ϊ���򲻻���ú���, ����ֵҲΪ0
    static inline HMODULE InjectDll(DWORD pid, LPCWSTR lpszDllFileName, LPCSTR lpszFunName = 0, PVOID pArgBuffer = 0, int nArgBufSize = 0, PVOID* pCallFuncRet = 0)
    {
        HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, TRUE, pid);
        if (!hProcess)
            return 0;
        HMODULE hModule = InjectDll(hProcess, lpszDllFileName, lpszFunName, pArgBuffer, nArgBufSize, pCallFuncRet);
        CloseHandle(hProcess);
        return hModule;
    }

    // ��dllע�뵽ָ��������
    // hProcess = ���̾��, ������Ҫ��д��Ȩ��
    // dllFileName = Ҫע���dll����·��, x86����ֻ��ע��x86��dll, x64�Ľ���ֻ��ע��x64��dll
    // lpszFunName = ����dll����Ҫ���õĺ�����, Ϊ0�򲻵���
    // pArgBuffer = ���ú������ݽ�ȥ�Ĳ�������, ���֧��2000���ֽ�
    // nArgBufSize = ���ݲ����ĳߴ�, ��λΪ�ֽ�
    // pDllModule = ���ռ��سɹ���ģ���ַ
    static inline HMODULE InjectDll(HANDLE hProcess, LPCWSTR lpszDllFileName, LPCSTR lpszFunName = 0, PVOID pArgBuffer = 0, int nArgBufSize = 0, PVOID* pCallFuncRet = 0)
    {
        if (pCallFuncRet)*pCallFuncRet = 0;
        if (!hProcess || !lpszDllFileName || !lpszDllFileName[0]) return 0;

        // opCode, �� test_load_call() ������� release�����Ĵ���
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
        BOOL bRet = FALSE;      // �Ƿ�ɹ�
        HANDLE hThread = NULL;  // Զ���߳̾��
        LPVOID pAddress = NULL; // ��Ҫִ�е��ֽ���ʹ��ݽ�ȥ�Ĳ�������
        HMODULE hModule = 0;
        __try
        {
            // ����һ���ڴ�, �ж�д��ִ�е�Ȩ��, ��Ҫ���ֽ����Ž�ȥ, ���Ѳ�������Ҳ��Ž�ȥ
            pAddress = VirtualAllocEx(hProcess, NULL, 0x1000, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
            if (pAddress == NULL)
                __leave;

            LPVOID pCode = (LPVOID)(((LPBYTE)pAddress) + 0x0000);
            LPVOID pArg  = (LPVOID)(((LPBYTE)pAddress) + opCodeSize);   // ������ַ���ֽ���ĺ���




            // �Ѻ���������������, ��ʱ�򴴽�Զ���߳̾�ִ��pCode������ݵĴ���
            BOOL bWriteOK = WriteProcessMemory(hProcess, pCode, opCode, opCodeSize, NULL);
            if (!bWriteOK) __leave;

            // ����Զ���߳�ʱ���ݽ�ȥ�Ĳ���
            bWriteOK = WriteProcessMemory(hProcess, pArg, &data, sizeof(data), NULL);
            if (!bWriteOK) __leave;
            
            //wchar_t dbg[260];
            //swprintf(dbg, 260, L"ע��dll, ������ʼ��ַ = 0x%p\n������ʼ��ַ = 0x%p", pCode, pArg);
            //MessageBoxW(0, dbg, 0, 0);

            //����Զ���̣߳���pCode��Ϊ�߳���ʼ������pArg��Ϊ����;
            hThread = NtCreateThreadEx(hProcess, (LPTHREAD_START_ROUTINE)pCode, pArg);
            if (hThread == NULL) __leave;

            // �ȴ����
            WaitForSingleObject(hThread, INFINITE);
            
            // ִ����� data.ModuleHandle ��¼���سɹ���ģ����, ��Ҫ������
            LPVOID pModuleHandle = (LPVOID)((LPBYTE)pArg + offsetof(CALL_ARGUMENT_DATA, hModule));
            SIZE_T NumberOfBytesRead = 0;
            ReadProcessMemory(hProcess, pModuleHandle, &data.hModule, sizeof(data.hModule), &NumberOfBytesRead);
            hModule = (HMODULE)data.hModule;

            if (pCallFuncRet)
            {
                // ��ȡ���ú�����ķ���ֵ
                LPVOID pCallRet = (LPVOID)((LPBYTE)pArg + offsetof(CALL_ARGUMENT_DATA, funRet));
                ReadProcessMemory(hProcess, pCallRet, pCallFuncRet, sizeof(PVOID), &NumberOfBytesRead);
            }
            bRet = TRUE;
        }
        __finally
        {
            // ִ�����, �ͷ�����
            if (pAddress != NULL)
                VirtualFreeEx(hProcess, pAddress, 0, MEM_RELEASE);
            if (hThread != NULL)
                CloseHandle(hThread);
        }

        return hModule;

    }

    // ��ָ���������ͷ�ģ��
    // pid = ����ID
    // dllName = ģ����, ����·�������ļ���
    static inline BOOL InjectFreeLibrary(DWORD pid, LPCWSTR dllName)
    {
        HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, TRUE, pid);
        if (!hProcess)
            return false;
        BOOL ret = InjectFreeLibrary(hProcess, dllName);
        CloseHandle(hProcess);
        return ret;
    }

    // ��ָ���������ͷ�ģ��
    // hProcess = ���̾��, ������Ҫ��д��Ȩ��
    // dllName = ģ����, ����·�������ļ���
    static inline BOOL InjectFreeLibrary(HANDLE hProcess, LPCWSTR dllName)
    {
        const BYTE opCode[] =
        {
            // return data->pfnFreeLibrary(data->pfnGetModuleHandleW(data->dllName));
            // ������Щ�ֽ������������д������Ľ��
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

    // ��ָ���������ͷ�ģ��
    // pid = ����ID
    // hModule = InjectDll ���ص�ֵ
    static inline BOOL InjectFreeLibrary(DWORD pid, HANDLE hModule)
    {
        HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, TRUE, pid);
        if (!hProcess)
            return 0;
        BOOL ret = InjectFreeLibrary(hProcess, hModule);
        CloseHandle(hProcess);
        return ret;
    }

    // ��ָ���������ͷ�ģ��
    // hProcess = ���̾��, ������Ҫ��д��Ȩ��
    // hModule = InjectDll ���ص�ֵ
    static inline BOOL InjectFreeLibrary(HANDLE hProcess, HANDLE hModule)
    {
        const BYTE opCode[] =
        {
            // return data->pfnFreeLibrary(data->hModule);
            // ������Щ�ֽ������������д������Ľ��
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
    // ��ָ������ע��һ������������, Զ�̴����߳�, ִ�� opCode
    // pid = ����ID
    // opCode = �������ֽ���, �뱣֤��δ���ִ�в������, ����Ŀ����̿��ܻ����
    // opCodeSize = �ֽ��볤��
    // pArg = ���õĲ���, ��ע��, ���ǰ������ַ���ݹ�ȥ, ������Ŀ����������ڴ汣���������
    // nArgSize = ����ռ���ֽ���
    // pThreadRet = �̵߳ķ���ֵ
    static inline BOOL InjectFunction(DWORD pid, LPCVOID opCode, int opCodeSize, LPCVOID pFunArg, int nFunArgSize, DWORD* pThreadRet = 0)
    {
        HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, TRUE, pid);
        if (!hProcess)
            return false;
        BOOL ret = InjectFunction(hProcess, opCode, opCodeSize, pFunArg, nFunArgSize);
        CloseHandle(hProcess);
        return ret;
    }

    // ��ָ������ע��һ������������, Զ�̴����߳�, ִ�� opCode
    // hProcess = ���̾��, ������Ҫ��дִ�е�Ȩ��
    // opCode = �������ֽ���, �뱣֤��δ���ִ�в������, ����Ŀ����̿��ܻ����
    // opCodeSize = �ֽ��볤��
    // pArg = ���õĲ���, ��ע��, ���ǰ������ַ���ݹ�ȥ, ������Ŀ����������ڴ汣���������
    // nArgSize = ����ռ���ֽ���
    // pThreadRet = �̵߳ķ���ֵ
    static inline BOOL InjectFunction(HANDLE hProcess, LPCVOID opCode, int opCodeSize, LPCVOID pFunArg, int nFunArgSize, DWORD* pThreadRet = 0)
    {
        if (!hProcess || !opCode || opCodeSize <= 0)
            return 0;

        int size = opCodeSize + nFunArgSize;
        {   // ����һ��������ڴ�ߴ�, 4k����
            double n = ((double)size) / 0x1000;
            int i = ((int)(n)) / 0x1000;
            if (n > 0) i++;
            size = i * 0x1000;
            if (size < 0x1000)
                size = 0x1000;
        }
        BOOL bRet = FALSE;      // �Ƿ�ɹ�
        HANDLE hThread = NULL;  // Զ���߳̾��
        LPVOID pAddress = NULL; // ��Ҫִ�е��ֽ���ʹ��ݽ�ȥ�Ĳ�������
        __try
        {
            // ����һ���ڴ�, �ж�д��ִ�е�Ȩ��, ��Ҫ���ֽ����Ž�ȥ, ���Ѳ�������Ҳ��Ž�ȥ
            pAddress = VirtualAllocEx(hProcess, NULL, size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
            if (pAddress == NULL)
                __leave;

            LPVOID pCode = (LPVOID)(((LPBYTE)pAddress) + 0x0000);
            LPVOID pArg  = (LPVOID)(((LPBYTE)pAddress) + opCodeSize);   // ������ַ���ֽ���ĺ���

            // �Ѻ���������������, ��ʱ�򴴽�Զ���߳̾�ִ��pCode������ݵĴ���
            BOOL bWriteOK = WriteProcessMemory(hProcess, pCode, opCode, opCodeSize, NULL);
            if (!bWriteOK) __leave;

            // ����Զ���߳�ʱ���ݽ�ȥ�Ĳ���
            if (pFunArg)
            {
                bWriteOK = WriteProcessMemory(hProcess, pArg, pFunArg, nFunArgSize, NULL);
                if (!bWriteOK) __leave;
            }

            //����Զ���̣߳���pCode��Ϊ�߳���ʼ������pArg��Ϊ����;
            hThread = NtCreateThreadEx(hProcess, (LPTHREAD_START_ROUTINE)pCode, pArg);
            if (hThread == NULL) __leave;

            // �ȴ����
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

    // ��ȡntdll.dll��ģ����
    inline static HMODULE GetNtdllHandle()
    {
        static HMODULE hNtdll = 0;
        if (!hNtdll)
        {
            hNtdll = GetModuleHandleW(L"ntdll.dll");
            // �����Ȩ��, ��߳ɹ���
            EnableDebugPrivilege();
        }
        return hNtdll;
    }

private:

    // �����������Ȩ��, �����Ƿ�ɹ�
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
    //����ϵͳ�汾�ж�
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
    // ����Զ���߳�, �����߳̾��
    // hProcess = ���̾��, ��Ҫ����������ϴ���һ���߳�
    // lpStartAddress = �߳�ִ�еĺ���
    // lpParameter = �̲߳���
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


// ��dllע�뵽ָ��������
// hProcess = ���̾��, ������Ҫ��д��Ȩ��
// dllFileName = Ҫע���dll����·��, x86����ֻ��ע��x86��dll, x64�Ľ���ֻ��ע��x64��dll
// lpszFunName = ����dll����Ҫ���õĺ�����, Ϊ0�򲻵���
// pArgBuffer = ���ú������ݽ�ȥ�Ĳ�������, ���֧��2000���ֽ�
// nArgBufSize = ���ݲ����ĳߴ�, ��λΪ�ֽ�
// pDllModule = ���ռ��سɹ���ģ���ַ
static inline HMODULE InjectDll(DWORD pid, LPCWSTR lpszDllFileName, LPCSTR lpszFunName = 0, PVOID pArgBuffer = 0, int nArgBufSize = 0, PVOID* pCallFuncRet = 0)
{
    CDLLInject x64;
    return x64.InjectDll(pid, lpszDllFileName, lpszFunName, pArgBuffer, nArgBufSize, pCallFuncRet);
}

// ��ָ������ע��һ������������, Զ�̴����߳�, ִ�� opCode
// hProcess = ���̾��, ������Ҫ��дִ�е�Ȩ��
// opCode = �������ֽ���, �뱣֤��δ���ִ�в������, ����Ŀ����̿��ܻ����
// opCodeSize = �ֽ��볤��
// pArg = ���õĲ���, ��ע��, ���ǰ������ַ���ݹ�ȥ, ������Ŀ����������ڴ汣���������
// nArgSize = ����ռ���ֽ���
inline BOOL InjectFunction(HANDLE hProcess, LPCVOID opCode, int opCodeSize, LPCVOID pFunArg, int nFunArgSize)
{
    CDLLInject x64;
    return x64.InjectFunction(hProcess, opCode, opCodeSize, pFunArg, nFunArgSize);
}

