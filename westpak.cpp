#include "pch.h"
//int CALLBACK EnumPakProcW(const wchar_t* PakName, const ENUMPAKFILEW* e, LPVOID param);



LPVOID GetDllFun(LPCSTR funName)
{

    wstr file(260);
    file.assign(_str::A2W(g_argData->pszR2Path)).append(L"westpak_dll.dll");
    const LPCWSTR dllFile = file.c_str();
    static HMODULE hModule;
    if ( !hModule )
        hModule = LoadLibraryW(dllFile);
    
    if ( !hModule )
        return 0;

    return GetProcAddress(hModule, funName);
}

// ��ѹLZSSѹ�����ִ�, ����д����������������ݳ���
// InBuf  = ���뻺����ָ��,�������ʼ��
// InLen  = ���뻺��������,��ָ��
// OutBuf = ���������ָ��,���ݲ����ʼ��
// OutLen = �������������,��ָ��,Ϊ0��ȫ����ѹ,���OutLenָ��Ϊ0,�����������������,�����������������
int lzssdecompress(LPVOID InBuf, int InLen, LPVOID OutBuf, int OutLen)
{
    typedef int ( CALLBACK* pfn_def )( LPVOID InBuf, int InLen, LPVOID OutBuf, int OutLen );
    pfn_def pFun = (pfn_def)GetDllFun("lzssdecompress");
    if ( !pFun )
        return 0;
    return pFun(InBuf, InLen, OutBuf, OutLen);
}

// ��ѹLZSSѹ�����ִ�, ����д����������������ݳ���
// InBuf  = ���뻺����ָ��,�������ʼ��
// InLen  = ���뻺��������,��ָ��
// OutBuf = ���������ָ��,���ݲ����ʼ��
// OutLen = �������������,��ָ��,Ϊ0��ȫ����ѹ,���OutLenָ��Ϊ0,�����������������,�����������������
int lzssdecompress_xor(LPVOID InBuf, int InLen, LPVOID OutBuf, int OutLen)
{
    typedef int ( CALLBACK* pfn_def )( LPVOID InBuf, int InLen, LPVOID OutBuf, int OutLen );
    pfn_def pFun = (pfn_def)GetDllFun("lzssdecompress_xor");
    if ( !pFun )
        return 0;
    return pFun(InBuf, InLen, OutBuf, OutLen);
}


// ��ѹLZSSѹ�����ִ�, ����д����������������ݳ���
// InBuf  = ���뻺����ָ��,�������ʼ��
// InLen  = ���뻺��������,��ָ��
// OutBuf = ���������ָ��,���ݲ����ʼ��
// OutLen = Ϊָ��������������ȵ�ָ��,���ݲ����ʼ��,����ѹ�������ݵĴ�С
// CompLevel = ѹ������,0Ϊ�ٷ�ԭ��,1Ϊ��ѹ��
int lzsscompress(LPVOID InBuf, int InLen, LPVOID OutBuf, int OutLen, int CompLevel)
{
    typedef int ( CALLBACK* pfn_def )( LPVOID InBuf, int InLen, LPVOID OutBuf, int OutLen, int CompLevel );
    pfn_def pFun = (pfn_def)GetDllFun("lzsscompress");
    if ( !pFun )
        return 0;
    return pFun(InBuf, InLen, OutBuf, OutLen, CompLevel);
}


// ��ѹһ��PAK�ļ�����ǰĿ¼, 0=�ɹ�, 1=���ļ�����, 2=����һ��PAK�ļ�
// filename
int decodepakfileW(LPCWSTR filename)
{
    typedef int ( CALLBACK* pfn_def )( LPCWSTR filename );
    pfn_def pFun = (pfn_def)GetDllFun("decodepakfileW");
    if ( !pFun )
        return 0;
    return pFun(filename);
}

// ѹ��һ��Ŀ¼����ǰĿ¼,������Ϊ<Ŀ¼��>.pak, ����ֵ��Զ��0
int encodepakfileW(LPCWSTR filename)
{
    typedef int ( CALLBACK* pfn_def )( LPCWSTR filename );
    pfn_def pFun = (pfn_def)GetDllFun("encodepakfileW");
    if ( !pFun )
        return 0;
    return pFun(filename);
}

// ѹ��һ��Ŀ¼����ǰĿ¼,������Ϊ<Ŀ¼��>.pak, ����ֵ��Զ��0
// CompLevel = ѹ������,0Ϊ�ٷ�ԭ��,1Ϊ��ѹ��
int encodepakfile2W(LPCWSTR filename, int CompLevel)
{
    typedef int ( CALLBACK* pfn_def )( LPCWSTR filename, int CompLevel );
    pfn_def pFun = (pfn_def)GetDllFun("encodepakfile2W");
    if ( !pFun )
        return 0;
    return pFun(filename, CompLevel);
}

// ��һ��PAK�ļ��ж�ȡһ���ļ�, ����ֵ: ��buflenΪ0,�򷵻��ļ�����;������ļ����ݴ����buf��,���ض�ȡ�����ļ�����;��buflenС���ļ���С,�򲻶�ȡ�ļ�ֱ�ӷ���0
// buf = ���������ָ��,���ݲ����ʼ��
// buflen = �������������,��ָ��
// fn = PAK�ļ���,��ָ��
// fnwant = ��Ҫ��ȡ���ļ���,��ָ��
int GetFileFromPakW(LPVOID buf, int buflen, LPCWSTR fn, LPCWSTR fnwant)
{
    typedef int ( CALLBACK* pfn_def )( LPVOID buf, int buflen, LPCWSTR fn, LPCWSTR fnwant );
    pfn_def pFun = (pfn_def)GetDllFun("GetFileFromPakW");
    if ( !pFun )
        return 0;
    return pFun(buf, buflen, fn, fnwant);
}

// ��һ��PAK�ļ��е�ĳ��λ��ֱ�Ӷ�ȡ��ѹ�������, ����ֵ: �ѽ�ѹ������ݴ����buf��,���سɹ���ѹ���ֽ���;��buf��������ȫ��Ž�ѹ�������,��ضϺ�������
// buf = ���������ָ��,���ݲ����ʼ��
// buflen = �������������,��ָ��
// fn = PAK�ļ���,��ָ��
// offset = PAK�ļ��ڿ�ʼ��ѹ��λ��,��ָ��
// compsize = ��offset��ʼ��ѹ���ٸ��ֽ�,��ָ��
// algo = ѹ���㷨,0Ϊ��ѹ��,1ΪLZSSѹ��
int GetFileFromPakOffset2W(LPVOID buf, int bufLen, LPCWSTR fn, int offset, int compsize, int algo)
{
    typedef int ( CALLBACK* pfn_def )( LPVOID buf, int bufLen, LPCWSTR fn, int offset, int compsize, int algo );
    pfn_def pFun = (pfn_def)GetDllFun("GetFileFromPakOffset2W");
    if ( !pFun )
        return 0;
    return pFun(buf, bufLen, fn, offset, compsize, algo);
}




// ����ֵ: �������һ��ö�ٹ��̵ķ���ֵ��ͨ����1��-1�������PAK�ļ������򷵻�0���ڿշ���1��
// PakName = PAK�ļ�·��,��ָ��
// lpEnumFunc = ö�ٹ��̵ĺ���ָ��  int CALLBACK EnumPakProcA(const char *PakName, const ENUMPAKFILEA *e, void *param);
// param = ���ݸ�ö�ٹ��̵Ĳ���,����������ֵ
int EnumFileFromPakW(LPCWSTR PakName, PFN_EnumPakProcW lpEnumFunc, LPVOID param)
{
    typedef int ( CALLBACK* pfn_def )( LPCWSTR PakName, PFN_EnumPakProcW lpEnumFunc, LPVOID param );
    pfn_def pFun = (pfn_def)GetDllFun("EnumFileFromPakW");
    if ( !pFun )
        return 0;
    return pFun(PakName, lpEnumFunc, param);
}


// ����ֵ: 1��-1(��ʾTrue)��ʾ����ö��,0(��ʾFalse)��ʾ��ֹö��,����ֵ������δ�������Ϊ
int CALLBACK EnumPakProcW(const ENUMPAKFILEW* e, LPVOID param)
{
    if ( !e ) return 1;

    return 1;
}


