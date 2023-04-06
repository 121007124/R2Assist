#include "../include/tstr.h"
#include "../include/hook_detours/ApiHook.h"
#include "../include/WaitObject.h"

#define IPC_MAP_SIZE        0x1000  // 文件映射的尺寸

#define R2_MSG_EXIT         0x6401  // 退出线程
#define R2_MSG_SEL          0x6402  // 选歌, 外部进程向音速进程投递通知, wParam = 歌曲ID, lParam = 0
#define R2_MSG_SELCALLBACK  0x6403  // 选歌回调, 音速进程选好歌曲后向外部进程通知, wParam = 歌曲ID, lParam = 0

typedef struct _R2_IPC_MAPFILE
{
    DWORD pid_r2;   // 音速的进程ID
    DWORD tid_r2;   // 音速接收线程消息的线程ID

    DWORD pid_out;  // 音速这个进程向外投递通知的进程ID
    DWORD tid_out;  // 音速这个进程向外投递通知的线程ID

}R2_IPC_MAPFILE, * PR2_IPC_MAPFILE;

static int* m_pThis;// 类指针
static int m_pfn;   // 方法地址, thiscall

static apiHook hook_SelSong;
typedef int(__fastcall* PFN_SelSong_Hook)( int pThis, int edx, int arg1, int index, int arg3, int arg4 );
static PFN_SelSong_Hook SelSong_Hook;
int __fastcall MySelSong_Hook(int pThis, int edx, int arg1, int index, int arg3, int arg4);
static HANDLE   m_hMap;     // 文件映射句柄
static DWORD    m_tid;      // 等待消息事件的线程ID

static R2_IPC_MAPFILE m_ipc;    // 发送端, 接收端的结构

// 调用传递的数据, 不管调用什么, 最前面的字节都是这个结构, 所有投递消息的结构都是已这个结构为基类
typedef struct _IEXT_IPC_MESSAGE
{
    HANDLE  hEvent;         // 用来通知调用方, 功能已经处理完毕
    DWORD   pid;            // 调用方进程ID
    DWORD   tid;            // 调用方线程ID, 有值的话, 易语言处理完毕会通知这个线程

}IEXT_IPC_MESSAGE, * PIEXT_IPC_MESSAGE;



DWORD CALLBACK _ipc_init(LPVOID pThreadArgument);

// 创建等待线程消息的线程
DWORD _ipc_init_wait_thread();


// 等待消息的线程, 由被调试的进程投递消息过来, 这里处理
DWORD CALLBACK ThreadProc_WaitMessage(LPVOID pArg);

// 处理线程消息
bool Thread_message(UINT message, WPARAM wParam, LPARAM lParam);
// 向外部进程投递事件
bool PostEvent(UINT message, WPARAM wParam, LPARAM lParam);


// 选歌函数, 传递歌曲的索引
int WINAPI SelectSong(int index);

BOOL APIENTRY DllMain(HMODULE hModule,
                      DWORD  ul_reason_for_call,
                      LPVOID lpReserved
)
{
    switch ( ul_reason_for_call )
    {
    case DLL_PROCESS_ATTACH:
    {
        HANDLE hThread = CreateThread(0, 0, _ipc_init, 0, 0, 0);
        if ( hThread )CloseHandle(hThread);
        break;
    }
    case DLL_THREAD_ATTACH:
        break;
    case DLL_THREAD_DETACH:
        break;
    case DLL_PROCESS_DETACH:
        hook_SelSong.unHook();
        break;
    }
    return TRUE;
}

int __fastcall MySelSong_Hook(int pThis, int edx, int arg1, int index, int arg3, int arg4)
{
    // 把参数信息写入文件映射里, 然后通知外部程序
    PostEvent(R2_MSG_SELCALLBACK, index, 0);
    return SelSong_Hook(pThis, edx, arg1, index, arg3, arg4);
}

DWORD CALLBACK _ipc_init(LPVOID pThreadArgument)
{
    m_pThis = (int*)0x013B89D0;
    m_pfn = 0x00433073;
    HMODULE hGame = GetModuleHandleW(L"Game.exe");
    if ( !hGame )
        return 0;

    if ( hGame != (HMODULE)0x400000 )
    {
        // 是pro版, 需要计算地址
        m_pThis = (int*)( ( (LPBYTE)hGame + 0x1BE2BD0 ) );
        m_pfn = (int)( ( (LPBYTE)hGame + 0x8B033C ) );
    }

    if ( m_hMap ) return 0;   // 只初始化一次

    // 线程没创建就创建一个, 然后赋值, 这里在判断语句里赋值
    if ( !m_tid && !( m_tid = _ipc_init_wait_thread() ) )
        return 0;


    SelSong_Hook = (PFN_SelSong_Hook)m_pfn;
    hook_SelSong.hook(&(PVOID&)SelSong_Hook, MySelSong_Hook);
    DWORD pid = GetCurrentProcessId();
    const LPCWSTR mapName = L"{4B123240-229D-4270-891D-B576FBECD3CB}";
    m_hMap = CreateFileMappingW(INVALID_HANDLE_VALUE, 0, PAGE_READWRITE, 0, IPC_MAP_SIZE, mapName);
    if ( !m_hMap )
        return 0;

    PR2_IPC_MAPFILE pData = (PR2_IPC_MAPFILE)MapViewOfFile(m_hMap, FILE_MAP_WRITE, 0, 0, 0);
    if ( pData )
    {
        m_ipc.pid_r2 = GetCurrentProcessId();
        m_ipc.tid_r2 = m_tid;
        pData->pid_r2 = m_ipc.pid_r2;
        pData->tid_r2 = m_ipc.tid_r2;
        UnmapViewOfFile(pData);
    }
    return 0;
}

int WINAPI SelectSong(int index)
{
    int t = *m_pThis;
    return SelSong_Hook(t, 0, 6, index, 258, 2);
}

DWORD _ipc_init_wait_thread()
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
    case R2_MSG_SEL:        // 选歌, 外部进程向音速进程投递通知, wParam = 歌曲ID, lParam = 0
    {
        SelectSong((int)wParam);
        break;
    }
    default:
        break;
    }
    return false;
}

bool PostEvent(UINT message, WPARAM wParam, LPARAM lParam)
{
    if ( !m_ipc.tid_out )
    {
        PR2_IPC_MAPFILE pData = (PR2_IPC_MAPFILE)MapViewOfFile(m_hMap, FILE_MAP_WRITE, 0, 0, 0);
        if ( !pData ) return false;
        m_ipc.pid_out = pData->pid_out;
        m_ipc.tid_out = pData->tid_out;
    }
    if ( !m_ipc.tid_out )
        return false;
    bool ret = PostThreadMessageW(m_ipc.tid_out, message, wParam, lParam);
    if ( !ret )
    {
        // 投递失败, 那就把外部进程当成是非正常死亡, 清理掉线程ID
        PR2_IPC_MAPFILE pData = (PR2_IPC_MAPFILE)MapViewOfFile(m_hMap, FILE_MAP_WRITE, 0, 0, 0);
        if ( pData )
        {
            if ( pData->tid_out && pData->tid_out != m_ipc.tid_out )
            {
                // 记录里的ID和文件映射里的ID不同, 需要读出来重新尝试发送
                m_ipc.tid_out = pData->tid_out;
                m_ipc.pid_out = pData->pid_out;
                ret = PostThreadMessageW(m_ipc.tid_out, message, wParam, lParam);
            }

            // 文件映射记录的和程序记录的一样, 或者尝试发送后还是失败, 那就把文件映射里的数据清空
            if ( !ret )
            {
                pData->tid_out = 0;
                pData->tid_out = 0;
                m_ipc.tid_out = 0;
                m_ipc.tid_out = 0;
            }
            UnmapViewOfFile(pData);
        }
        else
        {
            m_ipc.tid_out = 0;
            m_ipc.tid_out = 0;
        }
    }
    return ret;
}