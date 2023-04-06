#include "pch.h"
#include "include/CFileRW.h"
#include "tinyxml2.h"
#include "include/CStringBuffer.h"
#include "include/GetPinyin.h"


#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "Winmm.lib")

typedef struct ENUM_PAK_CALLBACK_ARG
{
    int     code;
    LPCWSTR file;
    LPCWSTR getName;
    _str* buf;
}*PENUM_PAK_CALLBACK_ARG;



static CStringBufferW m_buf;
static wstr m_mp3File;
static bass m_bass;
static _str m_mp3Data;

int CALLBACK _get_mu_sic1(const ENUMPAKFILEW* e, PENUM_PAK_CALLBACK_ARG pArg);
bool parse_xml(_str& xmlData);

int Mp3GetTime(_str& mp3Data);



// 从pak或者文件里获取内容, 如果文件存在直接读取文件, 文件不存在就解压pak, 从内存里获取, pak里的文件不会落地
// pakFile = pak的文件路径
// getName = 要从pak里获取的文件名
// retData = 读取到的文件数据
bool _get_pak_file(LPCWSTR pakFile, LPCWSTR getName, _str& retData);

// 重新设置所有用户的路径
void re_set_list_data_path();
void _set_data(PLIST_DATA_STRUCT item1, PLIST_DATA_STRUCT item2);

void parse_song_list()
{
    // 只执行一次
    static bool first;
    if ( first )
        return;

    g_songListShow.reserve(7000);
    g_r2SongList.reserve(7000);
    m_buf.reserve(1024 * 1024 * 3); // 3M缓冲区
    m_buf.clear();


    first = true;
    auto Thread_ParseXml = [](LPVOID pArg) -> DWORD
    {
        // 每次都直接从xml文件里加载
        _str retData;
        _get_file(L"rnr_script/music.txt", retData);
        //read_file(LR"(J:\游戏\R2P\528\data\rnr_script\music.txt)", retData);
        parse_xml(retData);
        g_data.resize(g_r2SongList.size());
        SendMessageW(g_hWndDlg, WM_APP, 121007124, 123);
        return 0;
    };

    HANDLE hThread = CreateThread(0, 0, Thread_ParseXml, 0, 0, 0);
    if ( hThread )
        CloseHandle(hThread);
}


int FindSongFromSongList(LPCWSTR str, int type)
{
    if ( !str || !str[0] )
    {
        for ( LIST_DATA_STRUCT& item : g_r2SongList )
            g_songListShow.push_back(&item);
        return (int)g_songListShow.size();
    }

    g_songListShow.clear();
    for ( LIST_DATA_STRUCT& item : g_r2SongList )
    {
        bool isOk = false;
        if ( !isOk && __query(type, FIND_SONGLIST_NAME) )
            isOk = find_text(item.pszShowName, str, &item);

        if ( !isOk && __query(type, FIND_SONGLIST_ARTIST) )
            isOk = find_text(item.pszSingerName, str, &item);

        if ( !isOk && __query(type, FIND_SONGLIST_LEAVE) )
            isOk = find_text(item.pszLevel, str, &item);

        if ( isOk )
            g_songListShow.push_back(&item);
    }
    return (int)g_songListShow.size();
}



int CALLBACK _get_mu_sic1(const ENUMPAKFILEW* e, PENUM_PAK_CALLBACK_ARG pArg)
{
    if ( !e ) return 1;
    static int n;
    //wstr::dbg(L"%04d %s\n", ++n, e->FileName);
    if ( _wcsicmp(e->FileName, pArg->getName) == 0 )
    {
        pArg->buf->resize(e->Size);
        GetFileFromPakOffset2W(pArg->buf->data(), e->Size, pArg->file, e->Offset, e->CompressedSize, e->Type);
        return 0;
    }
    return 1;
}

// 从指定文件名获取数据, 防止目录下的文件过期, 直接从pak文件里获取, 文件名必须是解析music.txt时记录的文件名
// 文件名第一层就是pak文件名
bool _get_file(LPCWSTR file, _str& retData, LPCWSTR _path)
{
    retData.clear();
    LPCWSTR pos = wcschr(file, L'/');
    if ( !pos )
        return false;
    wstr path(_str::A2W(g_argData->pszR2Path));

    if ( _path && _path[0] )
        path = _path;

    LPCWSTR pos2 = wcsrchr(file, '/');   // 指向xml文件名, 包括扩展名
    wstr name = wstr::left(pos2 + 1, L".");  // 对应的文件名, 如果目录下没有这个文件, 需要去pak里取
    wstr pakName(file, pos - file);     // 对应的pak文件名, 不带扩展名
    wstr FileName(260);

    
    FileName.assign(path).append(pakName).append(L".pak");
    _get_pak_file(FileName.c_str(), file, retData);
    if ( retData.size() > 0 )
        return true;

    wstr tmpPath(260);
    tmpPath.append(path).append(L"*.pak");

    WIN32_FIND_DATA fd = { 0 };
    DWORD dwFileAttributes = FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_SYSTEM;

    HANDLE hFind = FindFirstFileW(tmpPath.c_str(), &fd);
    if ( INVALID_HANDLE_VALUE == hFind ) return false;

    do
    {
        if ( ( dwFileAttributes & fd.dwFileAttributes ) == fd.dwFileAttributes )   // 匹配类型
        {
            if ( _wcsnicmp(fd.cFileName, pakName.c_str(), (int)pakName.size()) == 0 )
            {
                FileName.assign(path).append(fd.cFileName);
                _get_pak_file(FileName.c_str(), file, retData);
                if ( retData.size() > 0 )
                    break;
            }
        }
    } while ( FindNextFileW(hFind, &fd) );

    FindClose(hFind);
    return retData.size() > 0;
}

bool _get_pak_file(LPCWSTR pakFile, LPCWSTR getName, _str& retData)
{
    // 文件不存在, 需要到pak里获取
    ENUM_PAK_CALLBACK_ARG arg = { 0 };
    arg.code = 1;
    arg.file = pakFile;
    arg.getName = getName;
    arg.buf = &retData;
    EnumFileFromPakW(pakFile, (PFN_EnumPakProcW)_get_mu_sic1, &arg);
    return true;
}

bool is_file_exists(LPCSTR file)
{
    LPCSTR pos = strchr(file, '/');
    if ( !pos )
        return false;
    LPCSTR pos2 = strrchr(file, '/');   // 指向xml文件名, 包括扩展名
    _str name = _str::left(pos2 + 1, ".");  // 对应的文件名, 如果目录下没有这个文件, 需要去pak里取
    _str pakName(file, pos - file);     // 对应的pak文件名, 不带扩展名
    _str xmlData;

    _str FileName(260);
    FileName.assign(g_argData->pszR2Path).append(file);
    bool isJoin = FilePathExists(FileName.c_str());
    if ( !isJoin )
    {
        FileName.assign(g_argData->pszR2Path).append(pakName).append(".pak");
        _get_pak_file(FileName.a2w().c_str(), _str::A2W(file).c_str(), xmlData);
    }
    else
    {
        read_file(FileName.a2w().c_str(), xmlData);
    }

    if ( xmlData.empty() )
        return false;

    return isJoin;
}

bool parse_xml(_str& xmlData)
{
    tinyxml2::XMLDocument doc;
    if ( doc.Parse(xmlData.c_str()) != tinyxml2::XMLError::XML_SUCCESS )
    {
        return false;
    }

    tinyxml2::XMLNode* node = doc.RootElement();
    if ( !node )return false;

    int countNode = 0;
    while ( node )
    {
        tinyxml2::XMLElement* ele = node->ToElement();
        if ( ele )
        {
            const tinyxml2::XMLAttribute* attr = ele->FirstAttribute();
            LIST_DATA_STRUCT item;
            bool isJoin = false;
            int vlCount = 0;    // 属性计次, 超过多少次之后加入到全局数组里
            LPCSTR songName = "", singerName = "";
            while ( attr )
            {
                LPCSTR name = attr->Name();
                LPCSTR value = attr->Value();
                if ( _stricmp(name, "Index") == 0 )
                {
                    item.pszIndex = (LPCWSTR)value;
                    item.nIndex = _str::stoi(value);
                }
                else if ( _stricmp(name, "Name") == 0 )
                {
                    item.pszSongName = (LPCWSTR)value;
                    vlCount++;
                    songName = value;
                }
                else if ( _stricmp(name, "File") == 0 )
                {
                    item.pszFile = (LPCWSTR)value;
                    // 这里应该需要判断文件是否存在, 不存在就不加入列表, 这里为了演示都加入了
                    //isJoin = is_file_exists(value);
                    //if ( !isJoin )
                    //    vlCount = -10;
                    vlCount++;
                }
                else if ( _stricmp(name, "Image") == 0 )
                {
                    item.pszImage = (LPCWSTR)value;
                    vlCount++;
                }
                else if ( _stricmp(name, "Level") == 0 )
                {
                    item.pszLevel = (LPCWSTR)value;
                    item.nLevel = _str::stoi(value);
                }
                else if ( _stricmp(name, "Artist") == 0 )
                {
                    item.pszSingerName = (LPCWSTR)value;
                    singerName = value;
                    vlCount++;
                }
                else if ( _stricmp(name, "BPM") == 0 )
                {
                    item.pszBpm = (LPCWSTR)value;
                    item.nBpm = _str::stoi(value);
                }
                else if ( _stricmp(name, "Time") == 0 )
                {
                    item.nTimer = _str::stoi(value) / 60;
                }
                else if ( _stricmp(name, "Combo") == 0 )
                {
                    item.pszCombo = (LPCWSTR)value;
                    item.nCombo = _str::stoi(value);
                }
                else if ( _stricmp(name, "Show") == 0 )
                {
                    item.isShow = _str::stoi(value);
                }
                else if ( _stricmp(name, "Stat") == 0 )
                {
                    item.Stat = _str::stoi(value);
                }
                attr = attr->Next();
            }

            if ( vlCount == 4 )
            {
                isJoin = true;
                for ( LIST_DATA_STRUCT& it : g_r2SongList )
                {
                    // 有重复的索引都排除
                    if ( it.nIndex == item.nIndex )
                    {
                        isJoin = false;
                        break;
                    }
                }

                if ( isJoin )
                {
                    // 将秒数转换成 xx:xx 这种形式
                    wchar_t tmp[50];
                    auto timer2str_ = [&tmp](int t) -> LPCWSTR
                    {
                        int m = t / 60;
                        int s = t % 60;
                        swprintf_s(tmp, 50, L"%d分%d秒", m, s);
                        return tmp;
                    };
                    double level = ( (double)item.nLevel ) / 2.0;
                    item.pszSongName    = m_buf.AddStringWFromAnsi((LPCSTR)item.pszSongName);
                    item.pszSingerName  = m_buf.AddStringWFromAnsi((LPCSTR)item.pszSingerName);
                    item.pszFile        = m_buf.AddStringWFromAnsi((LPCSTR)item.pszFile);
                    item.pszImage       = m_buf.AddStringWFromAnsi((LPCSTR)item.pszImage);
                    item.pszLevel       = m_buf.AddFormat(L"%.1f", level);
                    item.pszBpm         = m_buf.AddFormat(L"%d", item.nBpm);
                    item.pszShowName    = m_buf.AddFormat(L"%s - %s", item.pszSongName, item.pszSingerName);
                    item.pszShowNameLv  = m_buf.AddFormat(L"%.1f☆ %s", level, item.pszShowName);
                    item.pszTimer       = m_buf.AddFormat(L"%d  [%s]", item.nTimer, timer2str_(item.nTimer));
                    item.pszCombo       = m_buf.AddFormat(L"%d", item.nCombo);

                    item.pszRemark = m_buf.AddFormat(L"歌名:\t%s\r\n"
                                                     L"歌手:\t%s\r\n"
                                                     L"等级:\t%s\r\n"
                                                     L"连击:\t%s\r\n"
                                                     L"时长:\t%s\r\n"
                                                     L"BPM:\t%d\r\n"
                                                     , item.pszSongName
                                                     , item.pszSingerName
                                                     , item.pszLevel
                                                     , item.pszCombo
                                                     , item.pszTimer
                                                     , item.nBpm
                    );

                    g_r2SongList.push_back(item);
                }
            }
        }
        node = node->NextSibling();
    }

    return true;
}

// 获取mp3的路径, 再解析xml, 获取到连击数
inline bool parse_xml_song(const _str& xml, PLIST_DATA_STRUCT pSongData, wstr& mp3A, wstr& mp3U)
{
    tinyxml2::XMLDocument doc;
    if ( doc.Parse(xml.c_str()) != tinyxml2::XMLError::XML_SUCCESS )
    {
        return false;
    }
    tinyxml2::XMLNode* node = doc.RootElement();
    if ( !node )return false;

    node = node->FirstChild();
    if ( !node )return false;

    int count = 0;
    while ( node )
    {
        tinyxml2::XMLElement* ele = node->ToElement();
        if ( ele )
        {
            LPCSTR nodeName = node->Value();
            if ( strcmp(nodeName, "BPM") == 0 )
            {
                const tinyxml2::XMLAttribute* a = ele->FindAttribute("BPM");
                LPCSTR szBpm = "0";
                if ( a )
                    szBpm = a->Value();
                pSongData->nBpm = _str::stoi(szBpm);
            }
            else if ( strcmp(nodeName, "BGM") == 0 )
            {
                const tinyxml2::XMLAttribute* a = ele->FindAttribute("Name");
                LPCSTR szMp3 = "";
                if ( a )
                    szMp3 = a->Value();
                mp3A = _str::A2W(szMp3).c_str();
                mp3U = _str::U2W(szMp3).c_str();
            }
            else if ( strcmp(nodeName, "AREA") == 0 )
            {
                const tinyxml2::XMLAttribute* a = ele->FindAttribute("Kind");
                const int nKind = _str::stoi(a->Value());

                const int Kinds[] =
                {
                    16, // 上
                    17, // 下
                    19, // 右
                    18, // 左

                    26, // 左星星
                    27, // 右星星

                    20, // 左上
                    21, // 右上
                    22, // 左下
                    23, // 右下

                    24, // 跳台

                    32, // 上下
                    33, // 左右
                    34, // 左右

                    128, 129, 130, // 左上, 按住到放开
                    131, 132, 133, // 右上, 按住到放开
                    134, 135, 136, // 上, 按住到放开
                    137, 138, 139, // 下, 按住到放开
                    140, 141, 142, // 左, 按住到放开
                    143, 144, 145, // 右, 按住到放开
                };

                bool isFind = false;

                for ( int i : Kinds )
                {
                    if ( nKind == i )
                    {
                        isFind = true;
                        break;
                    }
                }
                
                if ( !isFind )
                {
                    isFind = true;
                }
                switch ( nKind )
                {
                case 128: case 130: // 左上, 按住到放开
                case 131: case 133: // 右上, 按住到放开
                case 134: case 136: // 上, 按住到放开
                case 137: case 139: // 下, 按住到放开
                case 140: case 142: // 左, 按住到放开
                case 143: case 145: // 右, 按住到放开
                case 16: // 上
                case 17: // 下
                case 19: // 右
                case 18: // 左
                case 26: // 左星星
                case 27: // 右星星
                case 20: // 左上
                case 21: // 右上
                case 22: // 左下
                case 23: // 右下
                case 32: // 上下
                case 33: // 左右
                case 34: // 左右
                    count++;
                    break;
                case 24: // 跳台
                    count++;
                    count++;
                    break;
                default:
                    break;
                }

            }
        }
        node = node->NextSibling();
    }
    pSongData->nCombo = count;
    pSongData->pszCombo = m_buf.AddFormat(L"%d", count);
    return mp3A.empty() == false;
}


bool GetSongInfoFromSongList(PLIST_DATA_STRUCT pSongData)
{
    wstr name = pSongData->pszFile;   // xxx/yyy.xml xxx
    wstr xmlName;   // 要获取的xml文件名, yyy.xml
    {
        LPWSTR pszName = name.data();
        LPWSTR _xmlName = wcschr(pszName, '/');          // yyy.xml
        if ( !_xmlName )
            return false;
        *_xmlName++ = 0;
        xmlName = _xmlName;
    }

    // 把正斜杠改成反斜杠
    xmlName.replace('/', '\\');
    name.resize_strlen();

    wstr path(260), pakFile(260);
    path.assign(_str::A2W(g_argData->pszR2Path)).append(L"rnr_music\\");
    pakFile.assign(path).append(name).append(L".r2n");

    _str bmp, xmlData;
    // 先获取到xml文件数据, 然后根据解析xml获取到bmp, 获取歌曲名, 然后在取解析pak文件获取歌曲数据
    // 获取到歌曲数据后获取歌曲时长
    _get_pak_file(pakFile.c_str(), pSongData->pszFile, xmlData);

    // 如果获取xml失败, 那就把中文转换成首拼, 再次获取, 都获取失败就返回失败
    if ( xmlData.empty() )
    {
        // 获取中文文字的首拼来定位文件
        if ( (BYTE)pSongData->pszFile[0] > 128 )
        {
            char pinyin[128];
            LONG retLen = 0;

            _str nameA = name.w2a();
            GetPinyin(nameA.c_str(), pinyin, 128, &retLen, enmPinyinMode_FirstUpper);

            name.clear();
            for ( int i = 0; i < retLen; i++ )
            {
                const char& ch = pinyin[i];
                if ( ch >= 'A' && ch <= 'Z' )
                {
                    name.push_back(tolower(ch));
                }
            }

            // 这里的pak文件已经把中文改成了首字母文件名了
            pakFile.assign(path).append(name).append(L".r2n");
            _get_pak_file(pakFile.c_str(), pSongData->pszFile, xmlData);
        }

        if ( xmlData.empty() )
        {
#ifdef _DEBUG
            __debugbreak();
#endif
            return false;
        }
    }

    //pSongData->pszR2n = m_buf.AddString(pakFile.c_str());

    // 上面是获取r2n, 这里是获取pak, r2n记录记录障碍信息, 歌曲路径, pak的记录歌曲数据
    pakFile.assign(path).append(name).append(L".pak");
    pSongData->pszPak = m_buf.AddString(pakFile.c_str());

    _str mp3Data;
    mp3Data.clear();
    wstr mp3A, mp3U;
    if ( parse_xml_song(xmlData, pSongData, mp3A, mp3U) )
    {
        wstr tmpGetName(100);

        _get_pak_file(pakFile.c_str(), mp3A.c_str(), mp3Data);
        pSongData->pszMp3 = mp3A.c_str();

        // 先获取A版的文件, 如果失败, 就换成W版再尝试一次, 还失败就把歌曲文件名修改一下再获取
        // 这里最多会尝试4次, A, W, 换名后的A, 换名后的W
        if ( mp3Data.empty() )
        {
            _get_pak_file(pakFile.c_str(), mp3U.c_str(), mp3Data);
            pSongData->pszMp3 = mp3U.c_str();
        }

        if ( mp3Data.empty() )
        {
            LPCWSTR pos = wcschr(mp3A.c_str(), '/');
            if ( pos )
            {
                tmpGetName.assign(name).append(pos);
                pSongData->pszMp3 = tmpGetName.c_str();
                _get_pak_file(pakFile.c_str(), tmpGetName.c_str(), mp3Data);
            }
        }
        if ( mp3Data.empty() )
        {
            LPCWSTR pos = wcschr(mp3U.c_str(), '/');
            if ( pos )
            {
                tmpGetName.assign(name).append(pos);
                pSongData->pszMp3 = tmpGetName.c_str();
                _get_pak_file(pakFile.c_str(), tmpGetName.c_str(), mp3Data);
            }
        }
    }
    if ( mp3Data.empty() )
    {
        pSongData->pszMp3 = 0;
    }
    else
    {
        pSongData->pszMp3 = m_buf.AddString(pSongData->pszMp3);
        pSongData->nTimer = Mp3GetTime(mp3Data);
        pSongData->pszTimer = m_buf.AddFormat(L"%02d:%02d", (int)( pSongData->nTimer / 60 ), (int)( pSongData->nTimer % 60 ));
    }

    return true;
}


int Mp3GetTime(_str& mp3Data)
{
    static bool isInit;
    if ( !isInit )
    {
        m_bass.init();
        isInit = true;
    }

    //const int sss = MAKELONG(MAKEWORD('R', '2'), MAKEWORD('M', 'C'));
    //read_file(L"I:\\音乐\\American Pie.mp3", mp3Data);

    m_bass.load(mp3Data.c_str(), (int)mp3Data.size());
    int len = m_bass.getlen() / 1000;
    return len;
}

bool mp3PlayStop(bool isPlay)
{
    static int s_isPlay;
    auto pfn_timer = [](HWND hWnd, UINT m, UINT_PTR id, DWORD)
    {
        KillTimer(hWnd, id);
        if ( s_isPlay )
        {
            m_bass.play(true);
            m_bass.setpos(15000);
        }
    };

    s_isPlay = isPlay;
    if ( isPlay )
    {
        // 延时500毫秒播放, 500毫秒内停止了, 那就不播放
        SetTimer(g_argData->hWndParent, (ULONG_PTR)&s_isPlay, 500, pfn_timer);
    }
    else
    {
        m_bass.stop(false);
    }

    return true;
}


bool GetR2SongListData(PLIST_DATA_STRUCT pItem, MP3_FLAGS flags)
{
    //if ( !pItem->pszR2n || !pItem->pszR2n[0] )
    //{
    //    // 这个项目没有同步, 重新同步所有用户
    //    re_set_list_data_path();
    //}
    //if ( !pItem->pszR2n || !pItem->pszR2n[0] )
    //    return false;
    _str xmlData;
    _get_file(pItem->pszFile, xmlData);

    // 如果获取xml失败, 那就是有问题了, 调试版停下来调试看看
    if ( xmlData.empty() )
    {
#ifdef _DEBUG
        __debugbreak();
#endif
        return false;
    }

    if ( !pItem->pszMp3 || !pItem->pszMp3[0] )
    {
        auto pfn_get_mp3_file = [&]() -> LPCWSTR
        {
            tinyxml2::XMLDocument doc;
            if ( doc.Parse(xmlData.c_str()) != tinyxml2::XMLError::XML_SUCCESS )
                return L"";
            
            tinyxml2::XMLNode* node = doc.RootElement();
            if ( !node )return L"";
            node = node->FirstChild();
            if ( !node )return L"";

            int countNode = 0;
            while ( node )
            {
                LPCSTR nodeName = node->Value();
                if ( nodeName && strcmp(nodeName, "BGM") == 0 )
                {
                    tinyxml2::XMLElement* ele = node->ToElement();
                    if ( ele )
                    {
                        const tinyxml2::XMLAttribute* attr = ele->FirstAttribute();
                        while ( attr )
                        {
                            LPCSTR name = attr->Name();
                            LPCSTR value = attr->Value();
                            if ( name && _stricmp(name, "Name") == 0 )
                                return m_buf.AddStringWFromAnsi(value);
                        }
                    }
                }
                node = node->NextSibling();
            }
            return L"";
        };
        pItem->pszMp3 = pfn_get_mp3_file();
    }

    wstr pakFile(260);
    wstr x_song = wstr::left(pItem->pszMp3, L"/");
    wstr mp3File = wstr::left(pItem->pszFile, x_song.c_str());
    pakFile.assign(_str::A2W(g_argData->pszR2Path)).append(L"rnr_music\\").append(x_song).append(L".pak");

    m_mp3Data.clear();
    _get_pak_file(pakFile.c_str(), pItem->pszMp3, m_mp3Data);
    switch ( flags )
    {
    case MP3_FLAGS_NONE:
        break;
    case MP3_FLAGS_PLAY:
    {
        Mp3GetTime(m_mp3Data);
        m_bass.play(true);
        break;
    }
    case MP3_FLAGS_PLAY15:
    {
        Mp3GetTime(m_mp3Data);
        mp3PlayStop(true);
        break;
    }
    case MP3_FLAGS_STOP:
    {
        mp3PlayStop(false);
        break;
    }
    default:
        break;
    }

    return true;
}




void re_set_list_data_path()
{
    std::vector<PLIST_DATA_STRUCT> arr;
    arr.reserve(3000);

    for ( USER_ARR_DATA& userData : g_user )
    {
        for ( LIST_DATA_STRUCT& item : userData.arr )
            arr.push_back(&item);
    }

    for ( LIST_DATA_STRUCT& item : g_r2SongList )
    {
        for ( auto it = arr.begin(); it != arr.end(); ++it )
        {
            PLIST_DATA_STRUCT& itemUser = *it;
            if ( itemUser->nIndex == item.nIndex )
            {
                _set_data(itemUser, &item);
                arr.erase(it);
                break;
            }
        }
    }
}


void _set_data(PLIST_DATA_STRUCT item1, PLIST_DATA_STRUCT item2)
{
    item1->nIndex   = item2->nIndex;
    item1->pszIndex = item2->pszIndex;
    item1->pszFile  = item2->pszFile;
    item1->pszImage = item2->pszImage;
    item1->pszMp3   = item2->pszMp3;
    //item1->pszR2n   = item2->pszR2n;
    item1->pszPak   = item2->pszPak;
    item1->pszShowNameLv = item2->pszShowNameLv;
}