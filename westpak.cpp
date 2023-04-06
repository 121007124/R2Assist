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

// 解压LZSS压缩的字串, 返回写入输出缓冲区的数据长度
// InBuf  = 输入缓冲区指针,内容需初始化
// InLen  = 输入缓冲区长度,需指定
// OutBuf = 输出缓冲区指针,内容不需初始化
// OutLen = 输出缓冲区长度,需指定,为0则全部解压,如果OutLen指定为0,而输出缓冲区不够大,则可能引起程序崩溃。
int lzssdecompress(LPVOID InBuf, int InLen, LPVOID OutBuf, int OutLen)
{
    typedef int ( CALLBACK* pfn_def )( LPVOID InBuf, int InLen, LPVOID OutBuf, int OutLen );
    pfn_def pFun = (pfn_def)GetDllFun("lzssdecompress");
    if ( !pFun )
        return 0;
    return pFun(InBuf, InLen, OutBuf, OutLen);
}

// 解压LZSS压缩的字串, 返回写入输出缓冲区的数据长度
// InBuf  = 输入缓冲区指针,内容需初始化
// InLen  = 输入缓冲区长度,需指定
// OutBuf = 输出缓冲区指针,内容不需初始化
// OutLen = 输出缓冲区长度,需指定,为0则全部解压,如果OutLen指定为0,而输出缓冲区不够大,则可能引起程序崩溃。
int lzssdecompress_xor(LPVOID InBuf, int InLen, LPVOID OutBuf, int OutLen)
{
    typedef int ( CALLBACK* pfn_def )( LPVOID InBuf, int InLen, LPVOID OutBuf, int OutLen );
    pfn_def pFun = (pfn_def)GetDllFun("lzssdecompress_xor");
    if ( !pFun )
        return 0;
    return pFun(InBuf, InLen, OutBuf, OutLen);
}


// 解压LZSS压缩的字串, 返回写入输出缓冲区的数据长度
// InBuf  = 输入缓冲区指针,内容需初始化
// InLen  = 输入缓冲区长度,需指定
// OutBuf = 输出缓冲区指针,内容不需初始化
// OutLen = 为指向输出缓冲区长度的指针,内容不需初始化,返回压缩后内容的大小
// CompLevel = 压缩级别,0为官方原版,1为高压版
int lzsscompress(LPVOID InBuf, int InLen, LPVOID OutBuf, int OutLen, int CompLevel)
{
    typedef int ( CALLBACK* pfn_def )( LPVOID InBuf, int InLen, LPVOID OutBuf, int OutLen, int CompLevel );
    pfn_def pFun = (pfn_def)GetDllFun("lzsscompress");
    if ( !pFun )
        return 0;
    return pFun(InBuf, InLen, OutBuf, OutLen, CompLevel);
}


// 解压一个PAK文件到当前目录, 0=成功, 1=打开文件错误, 2=不是一个PAK文件
// filename
int decodepakfileW(LPCWSTR filename)
{
    typedef int ( CALLBACK* pfn_def )( LPCWSTR filename );
    pfn_def pFun = (pfn_def)GetDllFun("decodepakfileW");
    if ( !pFun )
        return 0;
    return pFun(filename);
}

// 压缩一个目录到当前目录,并保存为<目录名>.pak, 返回值永远是0
int encodepakfileW(LPCWSTR filename)
{
    typedef int ( CALLBACK* pfn_def )( LPCWSTR filename );
    pfn_def pFun = (pfn_def)GetDllFun("encodepakfileW");
    if ( !pFun )
        return 0;
    return pFun(filename);
}

// 压缩一个目录到当前目录,并保存为<目录名>.pak, 返回值永远是0
// CompLevel = 压缩级别,0为官方原版,1为高压版
int encodepakfile2W(LPCWSTR filename, int CompLevel)
{
    typedef int ( CALLBACK* pfn_def )( LPCWSTR filename, int CompLevel );
    pfn_def pFun = (pfn_def)GetDllFun("encodepakfile2W");
    if ( !pFun )
        return 0;
    return pFun(filename, CompLevel);
}

// 从一个PAK文件中读取一个文件, 返回值: 若buflen为0,则返回文件长度;否则把文件内容存放在buf中,返回读取到的文件长度;若buflen小于文件大小,则不读取文件直接返回0
// buf = 输出缓冲区指针,内容不需初始化
// buflen = 输出缓冲区长度,需指定
// fn = PAK文件名,需指定
// fnwant = 需要读取的文件名,需指定
int GetFileFromPakW(LPVOID buf, int buflen, LPCWSTR fn, LPCWSTR fnwant)
{
    typedef int ( CALLBACK* pfn_def )( LPVOID buf, int buflen, LPCWSTR fn, LPCWSTR fnwant );
    pfn_def pFun = (pfn_def)GetDllFun("GetFileFromPakW");
    if ( !pFun )
        return 0;
    return pFun(buf, buflen, fn, fnwant);
}

// 从一个PAK文件中的某个位置直接读取解压后的数据, 返回值: 把解压后的数据存放在buf中,返回成功解压的字节数;若buf不足以完全存放解压后的数据,则截断后续数据
// buf = 输出缓冲区指针,内容不需初始化
// buflen = 输出缓冲区长度,需指定
// fn = PAK文件名,需指定
// offset = PAK文件内开始解压的位置,需指定
// compsize = 从offset开始解压多少个字节,需指定
// algo = 压缩算法,0为不压缩,1为LZSS压缩
int GetFileFromPakOffset2W(LPVOID buf, int bufLen, LPCWSTR fn, int offset, int compsize, int algo)
{
    typedef int ( CALLBACK* pfn_def )( LPVOID buf, int bufLen, LPCWSTR fn, int offset, int compsize, int algo );
    pfn_def pFun = (pfn_def)GetDllFun("GetFileFromPakOffset2W");
    if ( !pFun )
        return 0;
    return pFun(buf, bufLen, fn, offset, compsize, algo);
}




// 返回值: 返回最后一次枚举过程的返回值（通常是1或-1）。如果PAK文件错误则返回0，内空返回1。
// PakName = PAK文件路径,需指定
// lpEnumFunc = 枚举过程的函数指针  int CALLBACK EnumPakProcA(const char *PakName, const ENUMPAKFILEA *e, void *param);
// param = 传递给枚举过程的参数,可以是任意值
int EnumFileFromPakW(LPCWSTR PakName, PFN_EnumPakProcW lpEnumFunc, LPVOID param)
{
    typedef int ( CALLBACK* pfn_def )( LPCWSTR PakName, PFN_EnumPakProcW lpEnumFunc, LPVOID param );
    pfn_def pFun = (pfn_def)GetDllFun("EnumFileFromPakW");
    if ( !pFun )
        return 0;
    return pFun(PakName, lpEnumFunc, param);
}


// 返回值: 1或-1(表示True)表示继续枚举,0(表示False)表示中止枚举,其它值会引起未定义的行为
int CALLBACK EnumPakProcW(const ENUMPAKFILEW* e, LPVOID param)
{
    if ( !e ) return 1;

    return 1;
}


