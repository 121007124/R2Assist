// pch.cpp: 与预编译标头对应的源文件

#include "pch.h"
#include "bass.h"
#include "include/zlib/Czlib.h"

// 当使用预编译的头时，需要使用此源文件，编译才能成功。

LPVOID GetBassFun(LPCSTR funName)
{

    wstr file(260);
    file.assign(_str::A2W(g_argData->pszR2Path)).append(L"bass.dll");
    const LPCWSTR dllFile = file.c_str();
    static HMODULE hModule;
    if ( !hModule )
        hModule = LoadLibraryW(dllFile);

    if ( !hModule )
        return 0;

    return GetProcAddress(hModule, funName);
}

template<typename T = int, typename ARG1 = DWORD>
T call_fun1(LPCSTR funName, ARG1 arg)
{
    typedef T ( CALLBACK* pfn_def )( ARG1 handle );
    pfn_def pFun = (pfn_def)GetBassFun(funName);
    if ( !pFun )
        return 0;
    return pFun(arg);
}

template<typename T = int, typename ARG1 = DWORD, typename ARG2 = DWORD>
T call_fun2(LPCSTR funName, ARG1 arg, ARG2 arg2)
{
    typedef T ( CALLBACK* pfn_def )( ARG1 arg, ARG2 arg2 );
    pfn_def pFun = (pfn_def)GetBassFun(funName);
    if ( !pFun )
        return 0;
    return pFun(arg, arg2);
}
template<typename T = int, typename ARG1 = DWORD, typename ARG2 = DWORD, typename ARG3 = DWORD>
T call_fun3(LPCSTR funName, ARG1 arg, ARG2 arg2, ARG3 arg3)
{
    typedef T(CALLBACK* pfn_def)( ARG1 arg, ARG2 arg2, ARG3 arg3 );
    pfn_def pFun = (pfn_def)GetBassFun(funName);
    if ( !pFun )
        return 0;
    return pFun(arg, arg2, arg3);
}

bass::~bass()
{
    typedef BOOL(CALLBACK* pfn_def)(  );
    pfn_def pFun = (pfn_def)GetBassFun("BASS_Free");
    if ( !pFun )
        return;
    pFun();
}

bool bass::init()
{
    typedef BOOL(CALLBACK* pfn_def)( int device, DWORD freq, DWORD flags, HWND win, const void *dsguid );
    pfn_def pFun = (pfn_def)GetBassFun("BASS_Init");
    if ( !pFun )
        return 0;
    return pFun(-1, 44100, 0, 0, 0);
}

bool bass::play(bool isContinue)
{
    return call_fun2("BASS_ChannelPlay", m_handle, !isContinue);
}

bool bass::pause()
{
    return call_fun1("BASS_ChannelPause", m_handle);
}

bool bass::stop(bool isFree)
{
    bool ret = call_fun1("BASS_ChannelStop", m_handle);
    if ( isFree )
        call_fun1("BASS_StreamFree", m_handle);
    return ret;
}

bool bass::load(LPCVOID pData, int size)
{
    if ( m_handle )
        stop(true);
    typedef int ( CALLBACK* pfn_def )( BOOL mem, const void *file, QWORD offset, QWORD length, DWORD flags );
    pfn_def pFun = (pfn_def)GetBassFun("BASS_StreamCreateFile");
    if ( !pFun )
        return 0;
    m_handle = pFun(true, pData, 0, (QWORD)size, 0);
    return m_handle != 0;
}

inline int len2int(DWORD handle, int len)
{
    double ret = call_fun2<double>("BASS_ChannelBytes2Seconds", handle, (QWORD)len);
    ret *= 1000.0;
    return (int)(LONG64)ret;
}

int bass::getlen()
{
    int len = call_fun2("BASS_ChannelGetLength", m_handle, BASS_POS_BYTE);
    return len2int(m_handle, len);
}

int bass::setpos(int pos)
{
    double len = (double)(pos / 1000);
    QWORD ret = call_fun2<QWORD>("BASS_ChannelSeconds2Bytes", m_handle, len);
    return call_fun3("BASS_ChannelSetPosition", m_handle, ret, BASS_POS_BYTE);
}

int bass::getpos()
{
    int ret = call_fun2("BASS_ChannelGetPosition", m_handle, BASS_POS_BYTE);
    return len2int(m_handle, ret);
}

int bass::setvol(int vol)
{
    float vl = ( (float)vol ) / 100.f;
    return call_fun3("BASS_ChannelSetAttribute", m_handle, BASS_ATTRIB_VOL, vl);
}

int bass::getvol()
{
    float vl = 0.f;
    call_fun3("BASS_ChannelGetAttribute", m_handle, BASS_ATTRIB_VOL, &vl);
    return (int)( vl * 100.f );
}

// 获取当前歌单数据, 根据传递的参数执行保存到磁盘或者返回数据
// 数据格式: 成员数 + 用户名(名字以服务器为准) + 每个结构写出的成员数 + {数据} + {数据} + ...
// 数据 = LIST_DATA_STRUCT 结构的成员, 这个结构的部分成员是为了显示用, 需要解析后分配空间
bool ListSong_GetListData(bool isWrite, _str* retData)
{
    if ( g_user.empty() )
    {
        MessageBoxW(0, L"写出数据失败, 请检查路径是否设置, 或者当前有没有项目", __FUNCTIONW__, 0);
        return false;
    }

    USER_ARR_DATA& userData = g_user[0];
    std::vector<LIST_DATA_STRUCT>& arr = userData.arr;

    int count = arr.size();
    _str buf(count * 1024); // 一个成员按1kb分配空间


    r2_write_int(buf, count);
    r2_write_strW(buf, userData.username);
    r2_write_int(buf, FILE_FLAG_SUNITEM_COUNT);   // TODO 写入每个结构的成员数, 目前是8个, 就是下面循环写出的数量
    for ( LIST_DATA_STRUCT& item : arr )
    {
        /*01*/  r2_write_int(buf, item.nLevel);
        /*02*/  r2_write_int(buf, item.nCombo);
        /*03*/  r2_write_int(buf, item.nTimer);
        /*04*/  r2_write_int(buf, item.nBpm);
        /*05*/  r2_write_int(buf, item.clrText);
        /*06*/  r2_write_strW(buf, item.pszSongName);
        /*07*/  r2_write_strW(buf, item.pszSingerName);
        /*08*/  r2_write_strW(buf, item.pszRemark);
        //MARK 写入的数据如果需要增加就写这里
    }

    Czlib zlib; uLongf nCompressSize = 0;
    zlib.compressEx(buf.c_str(), buf.size(), 0, 0, &nCompressSize);
    if ( !nCompressSize )
    {
        MessageBoxW(0, L"写出数据失败1", __FUNCTIONW__, 0);
        return false;
    }

    _str compressBuf(nCompressSize + 4);
    PDWORD pFlags = (PDWORD)compressBuf.data();
    *pFlags = FILE_HEAD_FLAGS;
    Bytef* pCompressData = zlib.compressEx(buf.c_str(), buf.size(), compressBuf.data() + 4, nCompressSize, &nCompressSize);
    if ( !pCompressData || !nCompressSize )
    {
        MessageBoxW(0, L"写出数据失败2", __FUNCTIONW__, 0);
        return false;
    }
    compressBuf.resize(nCompressSize + 4);
    pCompressData = (Bytef*)pFlags;
    if ( isWrite )
    {
        wstr file(260);
        file.assign(g_runPath).append(L"SongList.kdfp");
        FILE* f;
        errno_t err = _wfopen_s(&f, file.c_str(), L"wb+");
        if ( err != 0 || !f )
        {
            wstr errDbg;
            errDbg.Format(L"写出文件失败, 错误码 = %d\r\n"
                          L"请检查 % s 文件是否被占用\r\n"
                          L"文件: " __FILEW__ L"\r\n"
                          L"函数: " __FUNCTIONW__ L"\r\n行号: "
                          , err, file.c_str(), __LINE__);
            MessageBoxW(0, errDbg.c_str(), L"打开文件失败", 0);
            return false;
        }
        fseek(f, 0, SEEK_SET);
        fwrite(pCompressData, 1, nCompressSize + 4, f);
        fclose(f);
    }
    if ( retData ) retData->swap(compressBuf);
    return true;
}

// 加载磁盘中的数据
bool ListSong_LoadDiskData()
{
    if ( !g_runPath[0] )
    {
        MessageBoxW(0, L"读取本地数据失败, 路径未设置", __FUNCTIONW__, 0);
        return false;
    }

    g_user.resize(1);

    USER_ARR_DATA& userData = g_user[0];
    userData.arr.clear();
    wstr file(260);
    file.assign(g_runPath).append(L"SongList.kdfp");
    if ( !FilePathExists(file.c_str()) )
        return false;

    _str buf;
    if ( !read_file(file.c_str(), buf) )
    {
        MessageBoxW(0, L"读取本地数据失败, 读取文件失败", __FUNCTIONW__, 0);
        return false;
    }

    return ListSong_ParseListData(userData, true, buf);
}


// 把磁盘或者服务器返回的数据解析到列表上
// userData = 解析的结果保存到这里, 内部不会清除这个数组, 需要外面清除
bool ListSong_ParseListData(USER_ARR_DATA& userData, bool isUser, const _str& data)
{
    if ( data.empty() )
        return false;
    PDWORD pFlags = (PDWORD)data.data();
    if ( *pFlags != FILE_HEAD_FLAGS )
        return false;

    LPBYTE pData = (LPBYTE)( pFlags + 1 );

    Czlib zlib; uLongf nUncompressSize = 0;
    zlib.uncompressEx(pData, 0, 0, &nUncompressSize);
    if ( !nUncompressSize )
    {

        return false;
    }

    _str UncompressBuf(nUncompressSize);
    LPBYTE buf = zlib.uncompressEx(pData, UncompressBuf.data(), nUncompressSize, &nUncompressSize);
    if ( !buf )
    {

        return false;
    }

    int count = r2_read_int(buf);
    userData.username = g_buf.AddString(r2_read_strW(buf));
    int nItemCount = r2_read_int(buf);
    if ( nItemCount < FILE_FLAG_SUNITEM_COUNT ) return false;


    userData.arr.reserve(userData.arr.size() + (size_t)count);

    for ( int i = 0; i < count; i++ )
    {
        LIST_DATA_STRUCT item;

        //! 下面7个成员, 是写入时的成员
        //////////////////////////////////////////////////////////////////////////
        item.nLevel         = r2_read_int(buf);
        item.nCombo         = r2_read_int(buf);
        item.nTimer         = r2_read_int(buf);
        item.nBpm           = r2_read_int(buf);
        item.clrText        = r2_read_int(buf);
        item.pszSongName    = g_buf.AddString(r2_read_strW(buf));
        item.pszSingerName  = g_buf.AddString(r2_read_strW(buf));
        item.pszRemark      = g_buf.AddString(r2_read_strW(buf));
        //////////////////////////////////////////////////////////////////////////


        //! 下面这些成员都是重新分配的文本指针
        double lvd = (double)item.nLevel / 2.0;
        item.pszLevel = g_buf.AddFormat(L"%.1f☆", lvd);
        item.pszCombo = g_buf.AddFormat(L"%d", item.nCombo);
        item.pszBpm = g_buf.AddFormat(L"%d", item.nBpm);
        item.pszTimer = timer2str(item.nTimer);
        item.pszShowName = g_buf.AddFormat(L"%s - %s", item.pszSongName, item.pszSingerName);
        item.pszShowNameLv = g_buf.AddFormat(L"%s %s", item.pszLevel, item.pszShowName);

        item.userName = userData.username;   // 其他用户也走这里解析, 需要记录用户名
        item.isUser(isUser);

        userData.arr.push_back(item);
    }
    g_data.resize(userData.arr.size());

    return true;
}

