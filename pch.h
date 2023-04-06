// pch.h: 这是预编译标头文件。
// 下方列出的文件仅编译一次，提高了将来生成的生成性能。
// 这还将影响 IntelliSense 性能，包括代码完成和许多代码浏览功能。
// 但是，如果此处列出的文件中的任何一个在生成之间有更新，它们全部都将被重新编译。
// 请勿在此处添加要频繁更新的文件，这将使得性能优势无效。

#ifndef PCH_H
#define PCH_H

// 添加要在此处预编译的标头
#include "framework.h"
#include <vector>
#include "include/CPrivateProfile.h"
#include "include/CStringBuffer.h"
#include "include/R2plus_Include.h"
#include <comdef.h>
#include <gdiplus.h>

#define RGB2ARGB(_cr,a)     ((DWORD)(BYTE)(a) << 24) | ((_cr & 0x000000ff) << 16) | ((_cr & 0x0000ff00)) | ((_cr & 0x00ff0000) >> 16)

#define INI_APPNAME L"DllConfig"

#define LIST_LEVEL          0   // 等级
#define LIST_COMBO          1   // 连击数
#define LIST_TIME           2   // 持续时间
#define LIST_SONGNAME       3   // 歌名
#define LIST_BPM            4   // BPM
#define LIST_USERNAME       5   // 歌单作者
#define LIST_SINGER         6   // 歌手
#define LIST_SHOWNAME       7   // 显示的文本

#define R2_MSG_EXIT         0x6401  // 退出线程
#define R2_MSG_SEL          0x6402  // 选歌, 外部进程向音速进程投递通知, wParam = 歌曲ID, lParam = 0
#define R2_MSG_SELCALLBACK  0x6403  // 选歌回调, 音速进程选好歌曲后向外部进程通知, wParam = 歌曲ID, lParam = 0



enum LISTDATA_FLAGS
{
    LISTDATA_FLAGS_USERDATA = 0x0001,   // 表示当前项目数据是当前用户的数据


};

typedef struct LIST_DATA_STRUCT
{
    //! 下面这些成员需要写出保存, 写出和读入都会有这些数据
    //////////////////////////////////////////////////////////////////////////
    int     nLevel;         // 歌曲等级
    int     nCombo;         // 连击数
    int     nTimer;         // 持续时间
    int     nBpm;           // 歌曲bpm
    LPCWSTR pszSongName;    // 歌名
    LPCWSTR pszSingerName;  // 歌手
    LPCWSTR pszRemark;      // 备注
    LPCWSTR pszBpm;         // bpm
    int     isShow;         // 应该是是否显示这个歌曲, 这个应该是服务器控制上下架的标志
    int     Stat;           // 还不知道干嘛用

    //////////////////////////////////////////////////////////////////////////


    //! 以下成员需要加载数据后分配文本指针
    //////////////////////////////////////////////////////////////////////////
    LPCWSTR pszLevel;       // 等级
    LPCWSTR pszCombo;       // 连击数
    LPCWSTR pszTimer;       // 持续时间
    LPCWSTR pszShowName;    // 显示的名字, 一般是 歌曲 - 歌手
    //////////////////////////////////////////////////////////////////////////


    //! 下面这些成员的程序运行时使用的一些成员
    //////////////////////////////////////////////////////////////////////////
    LPCWSTR userName;       // 歌单作者
    UINT    flags;          // 标志, LISTDATA_FLAGS 开头枚举常量
    UINT    clrText;        // 文本颜色, 已经修改的项目就一种颜色, 已经修改完毕的另一种颜色
    //////////////////////////////////////////////////////////////////////////

    //! 下面是一些music.txt里的信息, 这些信息方便找到歌曲的mp3信息
    //////////////////////////////////////////////////////////////////////////
    int     nIndex;         // 歌曲索引, music.txt 里的索引, 目前还不知道这个索引是干嘛的
    LPCWSTR pszIndex;       // 歌曲索引文本, music.txt 里的索引, 目前还不知道这个索引是干嘛的
    LPCWSTR pszFile;        // 歌曲xml路径, music.txt 里 File 属性指向的文件, 一般是 xxx/xxx.xml, 比如 e_song09/e_song09.xml
    LPCWSTR pszImage;       // 图片路径, music.txt 里 Image 属性指向的文件, 一般是 xxx/xxx.bmp, 比如 e_song09/music_34.bmp
    LPCWSTR pszMp3;         // 实际获取MP3的路径
    //LPCWSTR pszR2n;         // xxx.r2n 的路径
    LPCWSTR pszPak;         // xxx.pak 的路径

    LPCWSTR  pszShowNameLv; // 显示的文本, 带星级, 星级 - 歌曲 - 歌手
    //////////////////////////////////////////////////////////////////////////

    LIST_DATA_STRUCT()
    {
        memset(this, 0, sizeof(*this));
#define _tmp_const(_s) this->_s = L""

        _tmp_const(pszSongName);
        _tmp_const(pszSingerName);
        _tmp_const(pszRemark);
        _tmp_const(pszBpm);
        _tmp_const(pszLevel);
        _tmp_const(pszCombo);
        _tmp_const(pszTimer);
        _tmp_const(pszShowName);
        _tmp_const(userName);

        _tmp_const(pszFile);
        _tmp_const(pszImage);
        _tmp_const(pszMp3);
        //_tmp_const(pszR2n);
        _tmp_const(pszPak);
        _tmp_const(pszShowNameLv);

#undef _tmp_const

    }


    // 判断这个项目是否是自己的数据
    inline bool isUser()    { return __query(flags, LISTDATA_FLAGS_USERDATA); }

    // 修改项目是否是当前用户的数据
    inline bool isUser(bool _isUser)
    {
        if ( _isUser )
            flags |= LISTDATA_FLAGS_USERDATA;
        else
            flags &= ~LISTDATA_FLAGS_USERDATA;
        return false;
    }

    // 判断结构是否相等, 除了颜色不判断, 剩下的都比较
    inline bool isEqual(LIST_DATA_STRUCT& item)
    {
        LIST_DATA_STRUCT& item2 = item;
        LIST_DATA_STRUCT& item1 = *this;
        if (    // 判断两个结构是否相等
            wcscmp(item2.pszSongName, item1.pszSongName) != 0
            || wcscmp(item2.pszSingerName, item1.pszSingerName) != 0
            || wcscmp(item2.pszRemark, item1.pszRemark) != 0
            || item2.nLevel != item1.nLevel
            || item2.nCombo != item1.nCombo
            || item2.nTimer != item1.nTimer
            || item2.nBpm != item1.nBpm
            )
        {
            return false;
        }
        return true;
    }

}*PLIST_DATA_STRUCT;

typedef int(__stdcall* PFN_LoadLogin)( int code, int arg );
typedef struct R2PLUS_DLL_STRUCT
{
    HWND    hWndParent;     // 父窗口句柄
    LPCSTR  pszIniFileName; // 配置文件路径
    LPCSTR  userName;       // 当前用户名
    HWND    hWndR2Hock;     // R2Hock的主窗口句柄
    HWND    hWndFind;       // 查找歌曲的窗口句柄
    HWND    hWndR2;         // QQ音速主窗口句柄
    HWND    hWndFindEditWnd;// 搜索歌曲编辑框外层的窗口, 获取编辑框就获取ID = 140 的子窗口
    LPCSTR  pszR2Path;      // r2plus程序目录
    LPCSTR  pszR2FindSong;  // 歌曲搜索路径
    LPCSTR  pszR2Hock;      // r2hock路径

    PFN_LoadLogin pfnLoadLogin; // 加载登录窗口的函数
}*PR2PLUS_DLL_STRUCT;

typedef struct USER_STRUCT
{
    HWND    hWndParent;     // 父窗口句柄
    LPCSTR  pszIniFileName; // 配置文件路径
    LPCSTR  userName;       // 当前用户名
    HWND    hWndR2Hock;     // R2Hock的主窗口句柄
    HWND    hWndFind;       // 查找歌曲的窗口句柄
    HWND    hWndR2;         // QQ音速主窗口句柄
}*PUSER_STRUCT;

typedef struct USER_ARR_STRUCT
{
    ULONG64 id;
    LPCWSTR username;
}*PUSER_ARR_STRUCT;

typedef struct USER_ARR_DATA
{
    LPCWSTR username;
    std::vector<LIST_DATA_STRUCT> arr;
}*PUSER_ARR_DATA;

enum SORT_MODE
{
    SORT_MODE_NONE      = 0x0000,   // 不排序
    SORT_MODE_LEVEL     = 0x0001,   // 等级排序
    SORT_MODE_NAME      = 0x0002,   // 歌名排序
    SORT_MODE_ARTIST    = 0x0004,   // 歌手排序
    SORT_MODE_TIMER     = 0x0008,   // 时长排序
    SORT_MODE_COMBO     = 0x0010,   // 连击排序
    SORT_MODE_BPM       = 0x0020,   // BPM排序

    SORT_UPTOLOW        = 0x10000,  // 从大到小, 没有这个标志就是从小到大
    SORT_LOWTOUP        = 0x00000,  // 从小到大, 默认标志
};


extern HINSTANCE g_hInst;
extern CPrivateProfile* g_ini;
extern std::vector<PLIST_DATA_STRUCT> g_data;
extern std::vector<LIST_DATA_STRUCT> g_r2SongList;  // 游戏的曲库
extern HANDLE g_isLoadR2SongDone;                   // 是否加载游戏的曲库完毕, 加载完毕这个值会是0
extern std::vector<USER_ARR_DATA> g_user;           // 用户信息, 二维数组, 第一层是记录用户的歌单数, 第二层是记录歌单信息
extern PR2PLUS_DLL_STRUCT g_argData;
extern CStringBuffer g_buf;
extern _str g_token;
extern LPCWSTR g_userNameW;                         // 用户名, 从文件或者服务器返回的数据解析出来的, g_buf清空后这个变量失效
extern LPCSTR g_userName;
extern LPCSTR g_password;
extern std::vector<PLIST_DATA_STRUCT> g_songListShow;// 所有歌曲列表, 显示用的, 成员的指针指向 g_songList 的成员
extern int g_sortMode;
extern int g_sortModeListView;
extern wchar_t g_runPath[260];
extern wchar_t g_runFile[260];
extern HWND g_hWndDlg;


EXTERN_C _declspec( dllexport ) LPVOID WINAPI R2Puls_Dll_121007124(PSERVER_ARG pArg);

// 登录线程回调
DWORD CALLBACK ThreadProc_Login(LPVOID pArg);

// 注册线程回调
DWORD CALLBACK ThreadProc_Regis(LPVOID pArg);

// 生成注册码线程回调
DWORD CALLBACK ThreadProc_MakeCode(LPVOID _p);

DWORD CALLBACK ThreadProc_UpdateList(LPVOID _p);

// 离线
DWORD CALLBACK ThreadProc_OffLine(LPVOID _p);

// 从服务器获取歌单列表
DWORD CALLBACK ThreadProc_GetSongList(LPVOID _p);


HWND dlg_lv_Load(HWND hParent);
DWORD newThread(LPTHREAD_START_ROUTINE lpStartAddress, LPVOID pArg, bool isWait);

// 将秒数转换成 xx:xx 这种形式
LPCWSTR timer2str(int t);
// 将 xx:xx 这种形式转换成秒数
int str2time(LPCWSTR str);




// 解析歌曲列表, 加载窗口的时候解析一次, 只解析一次
void parse_song_list();


// 从指定文件名获取数据, 防止目录下的文件过期, 直接从pak文件里获取, 文件名必须是解析music.txt时记录的文件名
// 文件名第一层就是pak文件名
// path = 路径, 默认就是游戏根目录, 如果需要查找MP3之类的, 比如在script目录下就需要指定
bool _get_file(LPCWSTR file, _str& retData, LPCWSTR path = 0);


enum FIND_SONGLIST
{
    FIND_SONGLIST_NAME      = 1,    // 从歌曲名查找
    FIND_SONGLIST_ARTIST    = 2,    // 从歌手查找
    FIND_SONGLIST_LEAVE     = 4,    // 从等级查找
    FIND_SONGLIST_ALL = FIND_SONGLIST_NAME | FIND_SONGLIST_ARTIST | FIND_SONGLIST_LEAVE,    // 从所有字段查找
};
// 从歌曲列表里查找歌曲, 返回找到的数量, 找到的歌曲会写入 g_songListShow 这个数组
// str = 查找的内容
// type = 查找的模式
int FindSongFromSongList(LPCWSTR str, int type);


bool read_file(LPCWSTR file, _str& retData);

bool mp3PlayStop(bool isPlay);

enum MP3_FLAGS
{
    MP3_FLAGS_NONE      = 0x0000,
    MP3_FLAGS_PLAY      = 0x0001,
    MP3_FLAGS_STOP      = 0x0002,
    MP3_FLAGS_PLAY15    = 0x0003,
};
bool GetR2SongListData(PLIST_DATA_STRUCT pItem, MP3_FLAGS flags);

// 根据这个结构, 把连击mp3路径, 其他信息记录到这个结构里
bool GetSongInfoFromSongList(PLIST_DATA_STRUCT pSongData);


// 解压LZSS压缩的字串, 返回写入输出缓冲区的数据长度
// InBuf  = 输入缓冲区指针,内容需初始化
// InLen  = 输入缓冲区长度,需指定
// OutBuf = 输出缓冲区指针,内容不需初始化
// OutLen = 输出缓冲区长度,需指定,为0则全部解压,如果OutLen指定为0,而输出缓冲区不够大,则可能引起程序崩溃。
int lzssdecompress(LPVOID InBuf, int InLen, LPVOID OutBuf, int OutLen);

// 解压LZSS压缩的字串, 返回写入输出缓冲区的数据长度
// InBuf  = 输入缓冲区指针,内容需初始化
// InLen  = 输入缓冲区长度,需指定
// OutBuf = 输出缓冲区指针,内容不需初始化
// OutLen = 输出缓冲区长度,需指定,为0则全部解压,如果OutLen指定为0,而输出缓冲区不够大,则可能引起程序崩溃。
int lzssdecompress_xor(LPVOID InBuf, int InLen, LPVOID OutBuf, int OutLen);


// 解压LZSS压缩的字串, 返回写入输出缓冲区的数据长度
// InBuf  = 输入缓冲区指针,内容需初始化
// InLen  = 输入缓冲区长度,需指定
// OutBuf = 输出缓冲区指针,内容不需初始化
// OutLen = 为指向输出缓冲区长度的指针,内容不需初始化,返回压缩后内容的大小
// CompLevel = 压缩级别,0为官方原版,1为高压版
int lzsscompress(LPVOID InBuf, int InLen, LPVOID OutBuf, int OutLen, int CompLevel);


// 解压一个PAK文件到当前目录, 0=成功, 1=打开文件错误, 2=不是一个PAK文件
// filename
int decodepakfileW(LPCWSTR filename);

// 压缩一个目录到当前目录,并保存为<目录名>.pak, 返回值永远是0
int encodepakfileW(LPCWSTR filename);

// 压缩一个目录到当前目录,并保存为<目录名>.pak, 返回值永远是0
// CompLevel = 压缩级别,0为官方原版,1为高压版
int encodepakfile2W(LPCWSTR filename, int CompLevel);

// 从一个PAK文件中读取一个文件, 返回值: 若buflen为0,则返回文件长度;否则把文件内容存放在buf中,返回读取到的文件长度;若buflen小于文件大小,则不读取文件直接返回0
// buf = 输出缓冲区指针,内容不需初始化
// buflen = 输出缓冲区长度,需指定
// fn = PAK文件名,需指定
// fnwant = 需要读取的文件名,需指定
int GetFileFromPakW(LPVOID buf, int buflen, LPCWSTR fn, LPCWSTR fnwant);

// 从一个PAK文件中的某个位置直接读取解压后的数据, 返回值: 把解压后的数据存放在buf中,返回成功解压的字节数;若buf不足以完全存放解压后的数据,则截断后续数据
// buf = 输出缓冲区指针,内容不需初始化
// buflen = 输出缓冲区长度,需指定
// fn = PAK文件名,需指定
// offset = PAK文件内开始解压的位置,需指定
// compsize = 从offset开始解压多少个字节,需指定
// algo = 压缩算法,0为不压缩,1为LZSS压缩
int GetFileFromPakOffset2W(LPVOID buf, int bufLen, LPCWSTR fn, int offset, int compsize, int algo);

int GetTextLineCount(LPCSTR pStr, bool removeEmptyLine);

typedef struct ENUMPAKFILEW
{
    UINT Type;
    size_t Offset;
    size_t CompressedSize;
    size_t Size;
    WCHAR FileName[MAX_PATH];
}*PENUMPAKFILEW;
typedef int( CALLBACK* PFN_EnumPakProcW )(const ENUMPAKFILEW* e, LPVOID param);
// 返回值: 返回最后一次枚举过程的返回值（通常是1或-1）。如果PAK文件错误则返回0，内空返回1。
// PakName = PAK文件路径,需指定
// lpEnumFunc = 枚举过程的函数指针  int CALLBACK EnumPakProcA(const char *PakName, const ENUMPAKFILEA *e, void *param);
// param = 传递给枚举过程的参数,可以是任意值
int EnumFileFromPakW(LPCWSTR PakName, PFN_EnumPakProcW lpEnumFunc, LPVOID param);

BOOL FilePathExists(LPCSTR lpPath);
BOOL FilePathExists(LPCWSTR lpPath);

// 获取当前歌单数据, 根据传递的参数执行保存到磁盘或者返回数据
// 数据格式: 成员数 + {数据} + {数据} + ...
// 数据 = LIST_DATA_STRUCT 结构的成员, 这个结构的部分成员是为了显示用, 需要解析后分配空间
bool ListSong_GetListData(bool isWrite, _str* retData = 0);

// 加载磁盘中的数据
bool ListSong_LoadDiskData();

// 把磁盘或者服务器返回的数据解析到列表上
// userData = 解析的结果保存到这里, 内部不会清除这个数组, 需要外面清除
bool ListSong_ParseListData(USER_ARR_DATA& userData, bool isUser, const _str& data);


// 寻找文本, 按指定模式寻找, 返回是否匹配
// sourceStr = 被寻找的文本, 这个是源文本
// sunStr = 寻找的文本, 模式由这个文本控制
//      使用以下方法可以更快的搜索到歌曲
//      匹配头使用 ^xxx, 比如搜索 \"r\" 开头的歌曲, 那么就使用 \"^r\"
//      匹配尾使用 $xxx, 比如搜索 \"r\" 结尾的歌曲, 那么就使用 \"$r\"
//      搜索连击数使用 #xxx, 比如搜索 1104 连击的歌曲, 那么就使用 \"#1104\"
//          搜索连击数大于或者小于某个值, 请使用 #>xxx 或 #<xxx
//      搜索歌曲星级使用 *xx, 比如搜索 8 歌曲, 那么就使用 *16
//          星级需要乘以2, 比如 3.5星, 搜索就应该写 7, 因为3.5*2=7
//      更多搜索方式等待后续更新
bool find_text(LPCWSTR sourceStr, LPCWSTR subStr, PLIST_DATA_STRUCT pItem);
bool find_int(LPCWSTR findStr, int findInt, bool isLevel = false);

// 像音速投递任务
bool PostEvent(UINT message, WPARAM wParam, LPARAM lParam);


class bass
{
    DWORD m_handle;
public:
    bass() :m_handle(0) { }
    ~bass();
    bool init();
    // 播放, isContinue: true=继续播放, false = 重新播放
    bool play(bool isContinue);
    // 暂停
    bool pause();
    // 停止播放, isFree: 是否释放音乐资源, 释放后不能继续播放, 需要重新加载才能继续播放
    bool stop(bool isFree);

    // 加载mp3
    bool load(LPCVOID pData, int size);

    // 取音乐长度, 返回单位为毫秒
    int getlen();

    // 设置播放位置, 单位为毫秒
    int setpos(int pos);
    // 获取播放位置, 单位为毫秒
    int getpos();

    // 设置音量
    int setvol(int vol);
    // 获取音量
    int getvol();


};





#endif //PCH_H
