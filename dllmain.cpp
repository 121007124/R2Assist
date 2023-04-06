// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "pch.h"
#include "include/WaitObject.h"

#pragma comment(lib, "gdiplus.lib")
#define IPC_MAP_SIZE        0x1000  // 文件映射的尺寸




typedef struct _R2_IPC_MAPFILE
{
    DWORD pid_r2;   // 音速的进程ID
    DWORD tid_r2;   // 音速接收线程消息的线程ID

    DWORD pid_out;  // 音速这个进程向外投递通知的进程ID
    DWORD tid_out;  // 音速这个进程向外投递通知的线程ID

}R2_IPC_MAPFILE, * PR2_IPC_MAPFILE;

HINSTANCE g_hInst;
CPrivateProfile* g_ini;
std::vector<PLIST_DATA_STRUCT> g_data;
std::vector<LIST_DATA_STRUCT> g_r2SongList;  // 游戏的曲库
HANDLE g_isLoadR2SongDone;                   // 是否加载游戏的曲库完毕, 加载完毕这个值会是

std::vector<USER_ARR_DATA> g_user;           // 用户信息, 二维数组, 第一层是记录用户的歌单数, 第二层是记录歌单信息
PR2PLUS_DLL_STRUCT g_argData;
CStringBuffer g_buf;
_str g_token;
LPCWSTR g_userNameW;                         // 用户名, 从文件或者服务器返回的数据解析出来的, g_buf清空后这个变量失效
LPCSTR g_userName;
LPCSTR g_password;
std::vector<PLIST_DATA_STRUCT> g_songListShow;// 所有歌曲列表, 显示用的, 成员的指针指向 g_songList 的成员
int g_sortMode;
int g_sortModeListView;
wchar_t g_runPath[260];
wchar_t g_runFile[260];
HWND g_hWndDlg;

static SERVER_ARG arg;
static R2PLUS_DLL_STRUCT buf;
static R2_IPC_MAPFILE m_ipc;
static DWORD m_tid;             // 等待音速进程通知的线程ID
static HANDLE m_hMap;           // 和音速交互使用的内存映射

EXTERN_C _declspec( dllexport ) LPVOID WINAPI R2Puls_Dll_Interface(LPVOID pArg);

DWORD CALLBACK ThreadProc_WaitMessage(LPVOID pArg);
bool Thread_message(UINT message, WPARAM wParam, LPARAM lParam);


int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nShowCmd)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    g_hInst = hInstance;
    R2Puls_Dll_Interface(0);
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
        g_hInst = hModule;
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

int CALLBACK sadjkhkjs(int a, int b)
{
    return 0;
}

void _load_window()
{
    buf.hWndParent = (HWND)333016;
    buf.pszIniFileName = R"(C:\Users\n\Desktop\R2Plus_Dll.ini)";
    buf.pszR2Path = 0;
    buf.pszR2Path = "I:\\r2\\R2练习机\\";
    buf.pfnLoadLogin = sadjkhkjs;

    arg.code = SERVER_LOADLIST;
    arg.pRet = &buf;
    g_argData = &buf;

    //R2Puls_Dll_121007124(&arg);

}


DWORD CALLBACK ThreadProc_WaitMessage(LPVOID pArg)
{
    HANDLE hEvent = pArg;
    SetEvent(hEvent);   // 通知创建线程的地方, 线程已经执行
    DWORD currentPid = GetCurrentProcessId();
    MSG msg;
    while ( GetMessageW(&msg, 0, 0, 0) )
    {
        switch ( msg.message )
        {
        case R2_MSG_EXIT:       // 退出线程
        {
            m_tid = 0;
            return 0;
        }
        default:
            Thread_message(msg.message, msg.wParam, msg.lParam);
            break;
        }
    }
    return 0;
}

bool Thread_message(UINT message, WPARAM wParam, LPARAM lParam)
{
    switch ( message )
    {
    case R2_MSG_SELCALLBACK:    // 选歌回调, 音速进程选好歌曲后向外部进程通知, wParam = 歌曲ID, lParam = 0
    {
        int index = (int)wParam;
        PostMessageW(g_hWndDlg, WM_APP + 1, wParam, 0);
        // 根据这个ID把对应的歌曲显示出来
        break;
    }
    default:
        break;
    }
    return false;
}



// 这个是接口函数, 为后面的版本预留, 目前暂时用不到
EXTERN_C _declspec( dllexport ) LPVOID WINAPI R2Puls_Dll_Interface(LPVOID pArg)
{
    HMODULE hModule = GetModuleHandleW(0);
    const int fileBuffer = sizeof(g_runFile) / sizeof(g_runFile[0]);
    GetModuleFileNameW(hModule, g_runFile, fileBuffer);     // 先获取到程序运行路径

    wcscpy_s(g_runPath, fileBuffer, g_runFile);            // 拷贝到路径, 倒找 '\', 然后把'\' 赋值为0 
    wchar_t* pos = wcsrchr(g_runPath, '\\');
    if ( pos ) *pos++ = 0;

    if ( !m_hMap )
    {
        const LPCWSTR mapName = L"{4B123240-229D-4270-891D-B576FBECD3CB}";
        m_hMap = CreateFileMappingW(INVALID_HANDLE_VALUE, 0, PAGE_READWRITE, 0, IPC_MAP_SIZE, mapName);
        if ( !m_hMap )
        {
            tstr_MessageBox(0, L"出错了", 0, L"创建文件映射失败, 无法与音速进行通讯");
            return 0;
        }
    }
    do 
    {
        if ( !m_hMap ) break;

        PR2_IPC_MAPFILE pData = (PR2_IPC_MAPFILE)MapViewOfFile(m_hMap, FILE_MAP_WRITE, 0, 0, 0);
        if ( !pData )break;

        auto _ipc_init_wait_thread = []() -> DWORD
        {
            DWORD tid = 0;
            HANDLE hEvent = CreateEventW(0, 0, 0, 0);
            if ( !hEvent ) return 0;
            HANDLE hThread = CreateThread(0, 0, ThreadProc_WaitMessage, hEvent, 0, &tid);
            if ( hThread )
            {
                // 等待线程跑起来
                WaitObj(hEvent, true);
            }
            CloseHandle(hEvent);
            return tid;
        };

        m_tid = _ipc_init_wait_thread();
        m_ipc.pid_out = GetCurrentProcessId();
        m_ipc.tid_out = m_tid;
        m_ipc.tid_r2 = pData->tid_r2;
        m_ipc.pid_r2 = pData->pid_r2;

        pData->pid_out = m_ipc.pid_out;
        pData->tid_out = m_ipc.tid_out;

        UnmapViewOfFile(pData);
    } while (false);

    _load_window();
    return 0;
}


BOOL FindFirstFileExists(LPCSTR lpPath, DWORD dwFilter)
{
    WIN32_FIND_DATAA fd;
    HANDLE hFind = FindFirstFileA(lpPath, &fd);
    BOOL bFilter = ( FALSE == dwFilter ) ? TRUE : fd.dwFileAttributes & dwFilter;
    BOOL RetValue = ( ( hFind != INVALID_HANDLE_VALUE ) && bFilter ) ? TRUE : FALSE;
    FindClose(hFind);
    return RetValue;
}

BOOL FindFirstFileExistsW(LPCWSTR lpPath, DWORD dwFilter)
{
    WIN32_FIND_DATAW fd;
    HANDLE hFind = FindFirstFileW(lpPath, &fd);
    BOOL bFilter = ( FALSE == dwFilter ) ? TRUE : fd.dwFileAttributes & dwFilter;
    BOOL RetValue = ( ( hFind != INVALID_HANDLE_VALUE ) && bFilter ) ? TRUE : FALSE;
    FindClose(hFind);
    return RetValue;
}
/*
 * 检查一个  路径 是否存在（绝对路径、相对路径，文件或文件夹均可）
 * 存在则返回 1 (TRUE)
 */
BOOL FilePathExists(LPCSTR lpPath)
{
    return FindFirstFileExists(lpPath, 0);
}
BOOL FilePathExists(LPCWSTR lpPath)
{
    return FindFirstFileExistsW(lpPath, 0);
}

// 取文本行数
// ptr = 文本指针, \0结尾
// removeEmptyLine = 删除空行, 为真则忽略空行
int GetTextLineCount(LPCSTR pStr, bool removeEmptyLine)
{
    LPCSTR ptr = pStr;
    if ( !ptr || !ptr[0] ) return 0;
    int count = 1;  // 文本有值, 那不管有没有换行, 最少有一行
    if ( removeEmptyLine && ( *ptr == '\r' || *ptr == '\n' ) )
        count = 0;  // 如果第一行是空行, 且是清除空行, 那就初始行数为0
    while ( *ptr )
    {
        const char& ch = *ptr++;
        if ( ch == '\r' || ch == '\n' )
        {
            // \r\n 指针指向\n后面
            if ( ch == '\r' && *ptr == '\n' )
                ++ptr;
            while ( removeEmptyLine && ( *ptr == '\r' || *ptr == '\n' ) )
                ++ptr;  // 空行的话, 换行后面就是 '\r' 或者 '\n', 这些连着的都清除

            ++count;
        }
    }
    return count;
}

bool read_file(LPCWSTR file, _str& retData)
{
    if ( !file || !file[0] ) return false;
    FILE* f;
    errno_t err = _wfopen_s(&f, file, L"rb+");
    if ( !f )
        return false;

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    retData.resize(size);
    size = (long)fread_s(retData.data(), size, 1, size, f);
    retData.resize(size);

    fclose(f);
    return true;
}

DWORD newThread(LPTHREAD_START_ROUTINE lpStartAddress, LPVOID pArg, bool isWait)
{
    HANDLE hThread = CreateThread(0, 0, lpStartAddress, pArg, 0, 0);
    DWORD ret = 0;
    if ( hThread )
    {
        WaitObj(hThread, isWait);
        GetExitCodeThread(hThread, &ret);
        CloseHandle(hThread);
    }
    return ret;
}


EXTERN_C _declspec( dllexport ) LPVOID WINAPI R2Puls_Dll_121007124(PSERVER_ARG pArg)
{
    switch ( pArg->code )
    {
    case SERVER_LOADLIST:
    {
        // 加载列表, 不登录, 先加载本地的列表, 登录的话需要写个登录窗口, 右键菜单从服务器拉取那里打开登录窗口
        R2Puls_Dll_Interface(0);
        static HWND hWnd;
        g_argData = (PR2PLUS_DLL_STRUCT)pArg->pRet;

        if ( !IsWindow(hWnd) )  // 窗口失效的话需要清空缓冲区
            g_buf.clear();
        if ( !g_ini )
        {
            ULONG_PTR uToken = 0;
            Gdiplus::GdiplusStartupInput input = { 0 };
            input.GdiplusVersion = 1;
            Gdiplus::GdiplusStartup(&uToken, &input, 0);
            wstr wPath;
            static CPrivateProfile ini;
            wPath = _str::A2W(g_argData->pszIniFileName);
            ini.open(wPath.c_str());
            g_ini = &ini;
            g_data.reserve(200);
            g_buf.reserve(1024 * 500);  // 500K的缓冲区

            // 解析music.txt, 把所有歌曲的信息都记录起来
            parse_song_list();
        }

        hWnd = dlg_lv_Load(g_argData->hWndParent);
        SendMessageW(hWnd, WM_APP, 121007124, 123);

        return hWnd;

    }
    case SERVER_LOGIN:
    {
        newThread(ThreadProc_Login, pArg, true);
        break;
    }
    case SERVER_REGIS:
    {
        newThread(ThreadProc_Regis, pArg, true);
        return (LPVOID)pArg->code;
        break;
    }
    case SERVER_MAKECODE:
    {
        newThread(ThreadProc_MakeCode, pArg, true);
        return (LPVOID)pArg->code;
    }
    case SERVER_FREE:
    {
        if ( pArg->pRet )
            free(pArg->pRet);
        break;
    }
    default:
        break;
    }

    return 0;
}

bool PostEvent(UINT message, WPARAM wParam, LPARAM lParam)
{
    if ( !m_ipc.tid_r2 )
    {
        PR2_IPC_MAPFILE pData = (PR2_IPC_MAPFILE)MapViewOfFile(m_hMap, FILE_MAP_WRITE, 0, 0, 0);
        if ( !pData ) return false;
        m_ipc.pid_r2 = pData->pid_r2;
        m_ipc.tid_r2 = pData->tid_r2;
    }
    if ( !m_ipc.tid_r2 )
        return false;
    bool ret = PostThreadMessageW(m_ipc.tid_r2, message, wParam, lParam);
    if ( !ret )
    {
        // 投递失败, 那就把外部进程当成是非正常死亡, 清理掉线程ID
        PR2_IPC_MAPFILE pData = (PR2_IPC_MAPFILE)MapViewOfFile(m_hMap, FILE_MAP_WRITE, 0, 0, 0);
        if ( pData )
        {
            if ( pData->tid_r2 && pData->tid_r2 != m_ipc.tid_r2 )
            {
                // 记录里的ID和文件映射里的ID不同, 需要读出来重新尝试发送
                m_ipc.tid_r2 = pData->tid_r2;
                m_ipc.pid_r2 = pData->pid_r2;
                ret = PostThreadMessageW(m_ipc.tid_r2, message, wParam, lParam);
            }

            // 文件映射记录的和程序记录的一样, 或者尝试发送后还是失败, 那就把文件映射里的数据清空
            if ( !ret )
            {
                pData->tid_r2 = 0;
                pData->tid_r2 = 0;
                m_ipc.tid_r2 = 0;
                m_ipc.tid_r2 = 0;
            }
            UnmapViewOfFile(pData);
        }
        else
        {
            m_ipc.tid_r2 = 0;
            m_ipc.tid_r2 = 0;
        }
    }
    return ret;
}