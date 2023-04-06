#include "pch.h"
#include "include/control/CListView.h"
#include "include/control/CListBox.h"
#include "include/control/CCombobox.h"
#include "include/control/CEdit.h"
#include "include/control/CInputBox.h"
#include "include/control/CButton.h"
#include "include/control/CMenu.h"
#include "include/control/Ctips.h"
#include "include/control/CStatic.h"
#include <algorithm>
#include "include/drawImage.h"
#include <shellapi.h>
#pragma comment(lib, "Shell32.lib")
#include "include/CDLLInject.h"

#define TEXT_STATIC L"歌曲备注, 写关键字方便搜索, 一般写一些玩这首歌的说明, 比如能不能A, 远近多少, 能不能加速等..."


struct LIST_EDIT_DATA
{
    int     iItem;
    int     iSubItem;
    HWND    hEdit;
    RECT    rcItem;
};

#define LISTBOX_SONGLIST_WIDTH 400  // 歌曲列表框的宽度

#define ID_BTN_UPDATE       1000
#define ID_BTN_FIND         1001
#define ID_EDIT_FIND        1002

#define ID_MENU_GETDATA     1999    // 从服务器拉取数据
#define ID_MENU_EDIT        1998    // 编辑当前项目
#define ID_MENU_AUTOPLAY    1997    // 选中自动播放歌曲
#define ID_MENU_STOP        1996    // 停止播放
#define ID_MENU_SORT_LEVEL  1995    // 按星级排序
#define ID_MENU_SORT_NAME   1994    // 按歌名排序
#define ID_MENU_SORT_ARTIST 1993    // 按歌手排序
#define ID_MENU_UPDATE      1992    // 提交数据到服务器
#define ID_MENU_LOGIN       1991    // 登录, 需要自动登录
#define ID_MENU_PLAY        1990    // 播放歌曲

#define ID_MENU_ADD         2000    // 添加项目
#define ID_MENU_DEL         2001    // 删除项目
#define ID_MENU_CLEAR       2002    // 清除项目
#define ID_MENU_ADDMY       2003    // 将当前歌曲添加到自己的歌单中
#define ID_MENU_ADDMYALL    2004    // 将当前歌单添加到自己的歌单中
#define ID_MENU_SORT_LV     2005    // 排序, 按等级排序
#define ID_MENU_SORT_COMBO  2006    // 排序, 按连击排序
#define ID_MENU_SORT_TIMER  2007    // 排序, 按时长排序
#define ID_MENU_SORT_SONG   2008    // 排序, 按歌名排序
#define ID_MENU_SORT_SINGER 2009    // 排序, 按歌手排序
#define ID_MENU_SORT_BPM    2010    // 排序, 按BPM排序
#define ID_MENU_HELP        2011    // 帮助


#define ID_MENU_IMPORTFILE  2500    // 导入歌单
#define ID_MENU_ZANZHU      2501    // 赞助
#define ID_MENU_OPENLOGIN   2502    // 打开登录器
#define ID_MENU_OPENR2HOCK  2503    // 打开助手
#define ID_MENU_SETTING     2504    // 设置
#define ID_MENU_INJECT      2505    // 启用




static CListView m_list;
static CListBox m_list_song;       // 歌曲列表
static CCombobox m_combobox;
static CMyEdit m_edit;              // 查找歌曲编辑框
static CMyEdit m_editRemark;        // 备注文本编辑框
static CMyStatic m_songImage;       // 歌曲对应的图片
static CMyEdit m_editInsert;        // 编辑的编辑框
static HFONT hFont;
static LIST_EDIT_DATA m_editData;   // 正在编辑的数据
static WNDPROC m_oldProcList;       // 超列原来的消息回调
static HWND m_hStatic;              // 备注编辑框上面的标签句柄
static bool m_isLocalData;          // 当前是否是加载本地数据
static int m_index;                 // 当前选中项的索引
static CMenu m_menu;                // 菜单
static CMenu m_menuWnd;             // 窗口的菜单, 这个是好几个弹出菜单组合的
static CMenu m_menuAll;             // 所有歌曲列表菜单
static CMenu m_menu_song;           // 歌曲列表的菜单
static bool m_isEdit;               // 是否允许修改, 不是自己的账户不允许编辑
static Ctips m_tips;
static int m_indexListSong;         // 歌曲名列表的选中项, 等显示的时候需要定位到这个项目
static int m_rClickItem;            // 右键点击弹出菜单时的项目索引
static int m_rClickSubItem;         // 右键点击弹出菜单时的列索引
static POINT m_rClickPoint;         // 右键按下时鼠标的位置
static int m_ServerSongCount;       // 服务器返回的歌曲数量
static bool m_isR2SongList;         // 组合框选项改变的时候赋值, 最后一个选项就是音速曲库
static bool m_isDblClkEditEnd;      // 结束编辑是否是双击列表框触发的
static Gdiplus::Bitmap* m_image;    // 显示的图片

INT_PTR CALLBACK Window_OnNotify(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK Window_DefWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK Window_ListViewProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK Window_ListViewEditProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK Window_SongImageProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK Window_EditRemarkProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK Window_OnCommand(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK Window_ListSongProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
void Window_SelSongCallback(int index);


// 排序歌曲名列表, 传递排序的方式
void ListSong_SortList(int mode);

// 排序歌单列表, 传递排序方式
void ListView_SortList(int mode);

// 向超列的父窗口发送编辑结束的通知
void _send_notify_edit_end(bool isChange);

void UpdateBtnClick();
void _init_var();
bool edit_command(int code);
void btn_find();
// 重新根据组合框选中项加载数组
int reLoad_show_list(std::vector<PLIST_DATA_STRUCT>& arr, LPCWSTR* username = 0);

const LPCWSTR editInsertTips =
L"使用以下方法可以更快的搜索到歌曲\r\n"
L"匹配头使用 ^xxx, 比如搜索 \"r\" 开头的歌曲, 那么就使用 \"^r\"\r\n"
L"匹配尾使用 $xxx, 比如搜索 \"r\" 结尾的歌曲, 那么就使用 \"$r\"\r\n"
L"搜索歌曲星级使用 *xx, 比如搜索 8 歌曲, 那么就使用 *16\r\n"
L"    星级需要乘以2, 比如 3.5星, 搜索就应该写 7, 因为3.5*2=7\r\n"
L"    支持带比较符号, 比如 *>, *<, *>=, *<=, 分别是大于, 小于, 大于等于, 小于等于\r\n"
L"更多搜索方式等待后续更新\r\n"
;




// 根据组合框选中项更新显示的列表
void UpdateListData(bool isUpdateTitle);

void Dlg_rcWindow(RECT* prc, LPCWSTR keyName);

HWND dlg_lv_Load(HWND hParent)
{
    if ( IsWindow(g_hWndDlg) )
        return g_hWndDlg;

    struct __DLGTEMPLATE : DLGTEMPLATE
    {
        WORD menuid;
        WORD cls;
        wchar_t caption[1024];
    };

    __DLGTEMPLATE dlg;
    memset(&dlg, 0, sizeof(dlg));
    dlg.style = WS_SYSMENU | WS_CAPTION | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_VISIBLE | WS_THICKFRAME;
    dlg.dwExtendedStyle = WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE | WS_EX_CONTROLPARENT | WS_EX_APPWINDOW;
    dlg.cdit = 0;
    memcpy(dlg.caption, L"在线歌单", 10);

    g_hWndDlg = CreateDialogIndirectParamW(g_hInst, &dlg, 0, Window_DefWndProc, 0);

    return g_hWndDlg;
}

inline LPCWSTR ListView_GetTextFromColIndex(int iItem, int iSubItem)
{
    LPCWSTR pszText = L"";
    if ( iItem < 0 || iItem >= (int)g_data.size() )
        return pszText;

    PLIST_DATA_STRUCT& data = g_data[iItem];
    switch ( iSubItem )
    {
    case LIST_LEVEL:
        pszText = (LPWSTR)( data->pszLevel );
        break;
    case LIST_COMBO:
        pszText = (LPWSTR)data->pszCombo;
        break;
    case LIST_TIME:
        pszText = (LPWSTR)data->pszTimer;
        break;
    case LIST_SONGNAME:
        pszText = (LPWSTR)data->pszSongName;
        break;
    case LIST_BPM:
        pszText = (LPWSTR)data->pszBpm;
        break;
    case LIST_USERNAME:
        pszText = (LPWSTR)data->userName;
        break;
    case LIST_SINGER:
        pszText = (LPWSTR)data->pszSingerName;
        break;
    case LIST_SHOWNAME:
        pszText = (LPWSTR)data->pszShowName;
        break;
    default:
        break;
    }
    return pszText;
}

// 返回是否重新分配了内存
inline bool _set_list_buf_text(LPCWSTR* ppszText, LPCWSTR text)
{
    LPWSTR pstr = (LPWSTR)*ppszText;
    size_t len1 = wcslen(pstr);
    size_t len2 = wcslen(text);
    if ( len1 >= len2 )
    {
        // 够存放, 直接存放进去
        memcpy(pstr, text, len2 * sizeof(wchar_t) + sizeof(wchar_t));
        return false;
    }
    else
    {
        // 不够存放, 创建新的缓冲区存放
        *ppszText = g_buf.AddString(text, len2);
        return true;
    }
}

inline bool ListView_SetTextFromColIndex(int iItem, int iSubItem, LPCWSTR text)
{
    if ( iItem < 0 || iItem >= (int)g_data.size() )
        return false;
    PLIST_DATA_STRUCT& data = g_data[iItem];
    LPCWSTR* pszText = 0;
    switch ( iSubItem )
    {
    case LIST_LEVEL:
        pszText = &data->pszLevel;
        break;
    case LIST_COMBO:
        pszText = &data->pszCombo;
        break;
    case LIST_TIME:
        pszText = &data->pszTimer;
        break;
    case LIST_SONGNAME:
        pszText = &data->pszSongName;
        break;
    case LIST_USERNAME:
        pszText = &data->userName;
        break;
    case LIST_SINGER:
        pszText = &data->pszSingerName;
        break;
    case LIST_SHOWNAME:
        pszText = &data->pszShowName;
        break;
    default:
        return false;
    }

    LPWSTR pstr = (LPWSTR)*pszText;
    if ( wcscmp(pstr, text) == 0 )  // 文本一样, 不修改
        return false;

    wstr tmpStr;
    switch ( iSubItem )
    {
    case LIST_COMBO:
    {
        // 连击数, 需要同时修改连击数成员
        data->nCombo = wstr::stoi(text);
        break;
    }
    case LIST_TIME:
    {
        // 时间, 需要修改编辑框内容, 如果有 ':' , 那就获取分号左右两边的数值, 如果没有分号, 那就把数值转换成xx:xx
        LPCWSTR pos = wcschr(text, ':');
        if ( !pos )
            pos = wcschr(text, '：');
        
        if ( pos )
        {
            // 有分号, 取左右两边的数值
            tmpStr = text;
            tmpStr.replace('：', ':');
            data->nTimer = str2time(tmpStr.c_str());
            text = tmpStr.c_str();
        }
        else
        {
            // 没有分号, 把文本转成数值再转换成分
            data->nTimer = wstr::stoi(text);
            text = timer2str(data->nTimer);
            break;
        }
        break;
    }
    default:
        break;
    }


    data->clrText = COLOR_TEXT_ERR;
    bool isAlloc = _set_list_buf_text(pszText, text);
    if ( iSubItem == LIST_SONGNAME || iSubItem == LIST_SINGER )
    {
        data->pszShowName = g_buf.AddFormat(L"%s - %s", data->pszSongName, data->pszSingerName);
        data->pszShowNameLv = g_buf.AddFormat(L"%s %s", data->pszLevel, data->pszShowName);
        // 为了省事, 直接重新分配, 为了省内存的话, 这里就不能这么用
        if ( isAlloc )
        {

        }
        else
        {

        }
    }
    ListSong_GetListData(true);     // 数据被修改了, 每次都写到磁盘
    m_list.InvalidateRect();
    return true;
}

INT_PTR CALLBACK Window_OnCustomDraw(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    LPNMLVCUSTOMDRAW lpNMCustomDraw = (LPNMLVCUSTOMDRAW)lParam;
    int index = lpNMCustomDraw->nmcd.dwItemSpec;
    if (index < 0 || index >= (int)g_data.size())
        return false;
    switch (lpNMCustomDraw->nmcd.dwDrawStage)
    {
    case CDDS_PREPAINT:
    {
        SetWindowLongPtrW(hWnd, DWLP_MSGRESULT, CDRF_NOTIFYITEMDRAW);
        return true;
    }
    case CDDS_ITEMPREPAINT:
    case CDDS_SUBITEM | CDDS_ITEMPREPAINT:
    {
        PLIST_DATA_STRUCT pItem = g_data[index];
        if ( pItem && pItem->isUser() )
        {
            lpNMCustomDraw->clrText = pItem->clrText;
            lpNMCustomDraw->clrTextBk = ( pItem->clrText == COLOR_TEXT_ERR ) ? COLOR_TEXT_BACK_ERR : COLOR_TEXT_BACK;
        }

        SetWindowLongPtrW(hWnd, DWLP_MSGRESULT, CDRF_NOTIFYPOSTPAINT);
        return true;
    }
    default:
        return false;
    }
    return false;
}

INT_PTR CALLBACK Window_OnNotify(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    LPNMHDR hdr = (LPNMHDR)lParam;
    switch ( hdr->code )
    {
    case NM_CUSTOMDRAW:
        return Window_OnCustomDraw(hWnd, message, wParam, lParam);
    case LVN_GETDISPINFOW:
    {
        LPNMLVDISPINFOW info = (LPNMLVDISPINFOW)lParam;
        const int index = info->item.iItem;
        if ( index < 0 || index >= (int)g_data.size() )
            return (INT_PTR)false;
        int iSubItem = info->item.iSubItem;
        if ( iSubItem == LIST_SONGNAME )
            iSubItem = LIST_SHOWNAME;
        info->item.pszText = (LPWSTR)ListView_GetTextFromColIndex(index, iSubItem);


        if ( info->item.pszText )
            info->item.cchTextMax = wcslen(info->item.pszText);
        break;
    }
    case LVN_BEGINLABELEDIT:    // 开始编辑
    {
        POINT pt;
        RECT& rcItem = m_editData.rcItem;
        int iSubItem = 0, iItem = 0;
        LPNMLVDISPINFOW pdi = (LPNMLVDISPINFOW)lParam;
        if ( pdi->item.mask != 121007124 )
        {
            GetCursorPos(&pt);
            ScreenToClient(pdi->hdr.hwndFrom, &pt);
        }
        else
        {
            pt = m_rClickPoint;
        }
        const int colCount = m_list.GetColumnCount();
        iItem = pdi->item.iItem;
        int colWidth = 0;
        m_list.GetItemRect(iItem, &rcItem);
        for ( int i = 0; i < colCount; i++ )
        {
            int width = m_list.GetColumnWidth(i);
            colWidth += width;
            if ( colWidth > pt.x )
            {
                iSubItem = i;
                rcItem.left = colWidth - width;
                rcItem.right = colWidth;
                break;
            }
        }
    
        m_editData.iSubItem = -1;
        PLIST_DATA_STRUCT pItem = g_data[iItem];

        m_tips.SetTipsText(m_editInsert, L"");
        // 只允许歌名这一列被修改
        if ( !pItem->isUser() || iSubItem != LIST_SONGNAME)
        {
            SetWindowLongPtrW(hWnd, DWLP_MSGRESULT, 1); // 不是自己的数据不显示编辑
            m_editInsert.hide();
            m_list_song.hide();
            return true;
        }


        // 剩下的就是可以修改
        m_editData.iSubItem = iSubItem;
        m_editData.iItem = iItem;

        LPCWSTR pszText = ListView_GetTextFromColIndex(m_editData.iItem, LIST_SHOWNAME);

        m_editData.rcItem.left += 3;
        //return false;
        m_editInsert.SetText(pszText);
        m_editInsert.Move(m_editData.rcItem);
        m_editInsert.show();
        m_editInsert.SetSel(0, -1);

        if ( iSubItem == LIST_SONGNAME )
        {
            m_tips.SetTipsText(m_editInsert, editInsertTips);
            RECT rc;
            m_editInsert.GetWindowRect(&rc);
            int width = rc.right - rc.left;
            if ( width < LISTBOX_SONGLIST_WIDTH )
            {
                int offset = ( LISTBOX_SONGLIST_WIDTH - width ) / 2;
                width = LISTBOX_SONGLIST_WIDTH;
                rc.left -= offset;
                rc.right += offset;
            }
            rc.top = rc.bottom;
            rc.bottom = rc.top + 200;
            m_list_song.Move(rc);
            m_list_song.SetIndex(m_indexListSong);
            m_list_song.SetTopIndex(m_indexListSong);
            ListSong_SortList(g_sortMode);

            SetWindowPos(m_list_song, 0, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, SWP_NOZORDER | SWP_SHOWWINDOW | SWP_NOACTIVATE);
            //m_list_song.show();
        }
        else if ( iSubItem == LIST_TIME )
        {
            // 这个目前永远不会走进来
            const LPCWSTR tmpTips =
                L"时间支持输入xxx秒, 比如123, 内部会自动转换成 02:03\r\n"
                L"也支持输入 xx:xx, 比如 02:03, 中英文的冒号都可以\r\n"
                ;
            m_tips.SetTipsText(m_editInsert, tmpTips);
        }
        m_editInsert.SetFocus();
        SetWindowLongPtrW(hWnd, DWLP_MSGRESULT, 1); // 返回不允许修改, 自己处理编辑框
        return true;
    }
    case LVN_ENDLABELEDIT:      // 结束编辑
    {
        LPNMLVDISPINFOW pdi = (LPNMLVDISPINFOW)lParam;
        if ( pdi->item.pszText && m_isDblClkEditEnd == false )
        {
            LPCWSTR pszText = ListView_GetTextFromColIndex(m_editData.iItem, LIST_SHOWNAME);
            if ( wcscmp(pdi->item.pszText, pszText) != 0 )
                ListView_SetTextFromColIndex(m_editData.iItem, m_editData.iSubItem, pdi->item.pszText);
        }
        m_isDblClkEditEnd = false;
        pdi->item.pszText = 0;
        break;
    }
    case LVN_ITEMCHANGED:
    {
        LPNMLISTVIEW pnmv = (LPNMLISTVIEW)lParam;
        if ( pnmv->uNewState == 0 && !__query(pnmv->uOldState, LVIS_SELECTED) )
            return false;
        
        m_editInsert.hide();
        m_list_song.hide();
        int oldIndex = m_index;
        if ( m_isEdit && oldIndex != -1 && oldIndex != pnmv->iItem )
        {
            // 原来有选中项, 并且当前选中的和之前选中的不同, 需要更改一下备注文本
            wstr text = m_editRemark.GetText();
            // 更新备注文本, 这个函数内部会判断是否需要更新
            //ListView_SetTextFromColIndex(m_index, LIST_REMARKS, text.c_str());
        }

        LPCWSTR remarkText = L"";
        m_index = pnmv->iItem;
        m_isEdit = false;
        DWORD isEdit = m_editRemark.GetStyle(false) | ES_READONLY;
        if ( m_index != -1 )
        {
            PLIST_DATA_STRUCT pItem = g_data[m_index];
            remarkText = pItem->pszRemark;
            if ( pItem->isUser() )
            {
                isEdit &= ~ES_READONLY;
                m_isEdit = true;
            }
            wstr imgFile(260);
            imgFile.assign(_str::A2W(g_argData->pszR2Path)).append(pItem->pszImage);
            _str imageData;
            _get_file(pItem->pszImage, imageData);
            if ( m_image ) delete m_image;
            m_image = 0;
            if ( imageData.size() )
                m_image = _img_create_from_memory(imageData.c_str(), imageData.size());
            
            m_songImage.InvalidateRect();
        }



        m_editRemark.SetStyle(isEdit, false);
        m_editRemark.SetWindowTextW(remarkText);

        break;
    }
    case NM_RCLICK:
    {
        LPNMITEMACTIVATE lpnmitem = (LPNMITEMACTIVATE)lParam;
        if ( lpnmitem->hdr.hwndFrom != m_list )
            break;
        const int indexComboBox = m_combobox.GetSel();
        const int countComboBox = m_combobox.GetCount();
        if ( m_isR2SongList )
        {
            // 选中的是最后一个项目
            m_menuAll.pop();
            break;
        }

        int index = lpnmitem->iItem;
        PUSER_ARR_DATA userData = (PUSER_ARR_DATA)m_combobox.GetParam(indexComboBox);
        PLIST_DATA_STRUCT pItem = 0;
        if ( index > -1 )
            pItem = g_data[index];
        
        int isCurrentUser = 0;
        if ( pItem && pItem->isUser() )
            isCurrentUser = 1;

        m_rClickItem = index;
        m_rClickSubItem = lpnmitem->iSubItem;

        GetCursorPos(&m_rClickPoint);
        ScreenToClient(m_list, &m_rClickPoint);

        bool isEnableUpdate = !g_token.empty();
        if ( isEnableUpdate )
        {
            m_menu.SetTitle(ID_MENU_GETDATA, L"从服务器拉取数据");
            m_menu.SetTitle(ID_MENU_UPDATE, L"提交数据到服务器");
        }
        else
        {
            m_menu.SetTitle(ID_MENU_GETDATA, L"从服务器拉取数据\t需要登录");
            m_menu.SetTitle(ID_MENU_UPDATE, L"提交数据到服务器\t需要登录");
        }
        m_menu.enabled(ID_MENU_GETDATA, isEnableUpdate);
        if ( isEnableUpdate && g_user[0].arr.size() > 0 )
        {
            isEnableUpdate = false;
            for ( LIST_DATA_STRUCT& _it : g_user[0].arr )
            {
                if ( _it.clrText == COLOR_TEXT_ERR )
                {
                    isEnableUpdate = true;
                    break;
                }
            }
        }
        m_menu.enabled(ID_MENU_UPDATE, isEnableUpdate);


        int isAddEnable = isCurrentUser ?  0 : 1;
        m_menu.enabled(ID_MENU_ADD, isCurrentUser);
        m_menu.enabled(ID_MENU_DEL, isCurrentUser);
        m_menu.enabled(ID_MENU_CLEAR, isCurrentUser);
        m_menu.enabled(ID_MENU_EDIT, isCurrentUser);
        m_menu.enabled(ID_MENU_ADDMY, isAddEnable);
        m_menu.enabled(ID_MENU_ADDMYALL, isAddEnable);
        m_menu.pop();
        break;
    }
    case NM_DBLCLK:
    {
        POINT pt;
        LPNMITEMACTIVATE lpnmitem = (LPNMITEMACTIVATE)lParam;
        if ( lpnmitem->iItem < 0 )
            break;
        int index = g_data[lpnmitem->iItem]->nIndex;
        if ( !PostEvent(R2_MSG_SEL, index, 0) )
        {
            // 调用失败就注入进程, 然后再次调用
            Window_OnCommand(hWnd, WM_COMMAND, ID_MENU_INJECT, 0);
            PostEvent(R2_MSG_SEL, index, 0);
        }

        break;
        GetCursorPos(&pt);
        ScreenToClient(m_list, &pt);
        int iItem = lpnmitem->iItem;
        int iSubItem = m_list.GetSubItemFromPoint(pt.x, pt.y);
        HWND hFind = g_argData->hWndFindEditWnd;
        HWND hEdit = hFind ? GetDlgItem(hFind, 140) : 0;
        if ( !hEdit )
        {
            auto pfn_isfind = [](HWND _hWnd) -> HWND
            {
                HWND hChild = GetDlgItem(_hWnd, 150);
                if ( !hChild )return 0;

                wchar_t str[260];
                GetClassNameW(_hWnd, str, 260);
                if ( _wcsicmp(str, L"WTWindow") != 0 )
                    return 0;
                GetWindowTextW(hChild, str, 260);
                if ( wcscmp(str, L"收藏") != 0 )
                    return 0;
                hChild = GetDlgItem(_hWnd, 140);
                if ( !hChild )return 0;
                HWND hEditRet = hChild;
                GetClassNameW(hChild, str, 260);
                if ( _wcsicmp(str, L"edit") != 0 )
                    return 0;
                hChild = GetDlgItem(_hWnd, 100);
                if ( !hChild )return 0;
                GetClassNameW(hChild, str, 260);
                if ( _wcsicmp(str, L"combobox") != 0 )
                    return 0;
                return hEditRet;
            };

            HWND hParent = GetDesktopWindow();
            HWND hChild = GetWindow(hParent, GW_CHILD);
            while ( hChild )
            {
                hEdit = pfn_isfind(hChild);
                if ( hEdit )
                    break;
                hChild = GetWindow(hChild, GW_HWNDNEXT);
            }
        }

        if ( hEdit )
        {
            PLIST_DATA_STRUCT pItem = g_data[iItem];
            LPCWSTR name = pItem->pszSongName;
            SetWindowTextW(hEdit, name);
            SendMessageW(hEdit, WM_SETTEXT, 0, (LPARAM)name);
        }

        break;
    }
    case LVN_COLUMNCLICK:
    {
        //TODO 做排序后这里需要往下执行
        LPNMLISTVIEW pnmv = (LPNMLISTVIEW)lParam;
        // 有从大到小标志就改成从小到大
        g_sortModeListView = __query(g_sortModeListView, SORT_UPTOLOW) ? SORT_LOWTOUP : SORT_UPTOLOW;
        switch ( pnmv->iSubItem )
        {
        case LIST_LEVEL:
            g_sortModeListView |= SORT_MODE_LEVEL;
            break;
        case LIST_COMBO:
            g_sortModeListView |= SORT_MODE_COMBO;
            break;
        case LIST_TIME:
            g_sortModeListView |= SORT_MODE_TIMER;
            break;
        case LIST_SONGNAME:
            g_sortModeListView |= SORT_MODE_NAME;
            break;
        case LIST_BPM:
            g_sortModeListView |= SORT_MODE_BPM;
            break;
        default:
            return true;
        }
        ListView_SortList(g_sortModeListView);
        break;
    }
    default:
        return (INT_PTR)false;
    }

    return (INT_PTR)true;
}
INT_PTR CALLBACK Window_DefWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static int titleInc;
    static wstr szTitle;
    static RECT rcCbb;
    static int cbbHeight;
    static int cbbWidth;
    const int colWidth[] = { 50, 50, 110, 0, 50, 100 };
    
    switch ( message )
    {
    case WM_APP:
    {
        if ( wParam == 121007124 )
        {
            DWORD count = m_ServerSongCount;
            KillTimer(hWnd, 10086);
            LPCWSTR cbx_first_name = L"全部用户歌单";
            if ( lParam == 20752843 )
            {
                // 已经从服务器获取数据完毕, 
                // 从服务器获取数据
                m_isLocalData = false;
                szTitle.assign(L"歌单列表 - 从服务器获取了 ").append(g_user.size()).append(L"个歌单, 一共").append(count).append(L" 首歌");
            }
            else
            {
                // 从本地加载数据
                ListSong_LoadDiskData();
                count = (DWORD)g_data.size();
                count = (DWORD)g_r2SongList.size();
                szTitle.assign(L"歌单列表 - 从本地加载了 ").append(g_user.size()).append(L"个歌单, 一共").append(count).append(L" 首歌");
                cbx_first_name = L"本地歌单";
                m_isLocalData = true;
            }
            
            SetWindowTextW(hWnd, szTitle.c_str());
            m_combobox.DelAll();
            //m_combobox.AddString(cbx_first_name);
            int index = 0;

            // 把所有用户的歌单存放到组合框里, 获取歌单列表的时候需要把自己的歌单存放到数组的第一个成员里
            //for ( USER_ARR_DATA& userData : g_user )
            //{
            //    //m_combobox.AddString(userData.username, (LPARAM)&userData);
            //    index = 1;
            //}
            m_combobox.AddString(L"音速曲库", 123);

            m_combobox.SetSel(index);
            UpdateListData(false);
            break;
        }
        return (INT_PTR)false;
    }
    case WM_APP+1:  // 选歌回调, 音速进程选好歌曲后向外部进程通知, wParam = 歌曲ID, lParam = 0
        Window_SelSongCallback((int)wParam);
        break;
    case WM_INITDIALOG:
    {
        RECT rc;
        Dlg_rcWindow(&rc, L"rcDlg");
        const int width = rc.right - rc.left;
        const int height = rc.bottom - rc.top;
        if ( !hFont )
            hFont = CWndBase::CreateFontW();
        _init_var();

        m_tips.create(TTS_ALWAYSTIP, WS_EX_LAYERED | WS_EX_TRANSPARENT);
        m_tips.Send(TTM_SETMAXTIPWIDTH, 0, 500);
        m_tips.SetSleep(TTDT_AUTOPOP, 0x7fff);

        DWORD style = WS_TABSTOP | WS_BORDER | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE | WS_CHILD |
            LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS | LVS_OWNERDATA | LVS_EDITLABELS;
        DWORD styleEx = WS_EX_LTRREADING | WS_EX_RIGHTSCROLLBAR | WS_EX_LEFT;
        DWORD styleList = LVS_EX_GRIDLINES | LVS_EX_SUBITEMIMAGES | LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP | LVS_EX_DOUBLEBUFFER;
        m_list.m_hWnd = CreateWindowExW(styleEx, WC_LISTVIEWW, L"歌单列表", style, 0, 0, 0, 0, hWnd, (HMENU)(LONG_PTR)0, g_hInst, 0);
        m_oldProcList = (WNDPROC)SetWindowLongPtrW(m_list.m_hWnd, GWLP_WNDPROC, (LONG_PTR)Window_ListViewProc);

        m_list.SetListStyle(styleList);

        m_list.InsertColumn(LIST_LEVEL   , L"等级"    , colWidth[LIST_LEVEL   ]);
        m_list.InsertColumn(LIST_COMBO   , L"连击"    , colWidth[LIST_COMBO   ]);
        m_list.InsertColumn(LIST_TIME    , L"持续时间" , colWidth[LIST_TIME    ]);
        m_list.InsertColumn(LIST_SONGNAME, L"歌名 - 歌手", colWidth[LIST_SONGNAME]);
        //m_list.InsertColumn(LIST_REMARKS , L"备注"    , colWidth[LIST_REMARKS ]);
        m_list.InsertColumn(LIST_BPM     , L"BPM"    , colWidth[LIST_BPM ]);
        m_list.InsertColumn(LIST_USERNAME, L"歌单作者" , colWidth[LIST_USERNAME]);
        

        //! 显示歌曲列表的列表
        style = WS_BORDER | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE | WS_POPUP | WS_VSCROLL |
            LBS_NODATA | LBS_OWNERDRAWFIXED | LBS_NOTIFY;
        styleList = LVS_EX_SUBITEMIMAGES | LVS_EX_INFOTIP;
        m_list_song.m_hWnd = CreateWindowExW(0, WC_LISTBOXW, L"歌曲列表", style, 0, 0, 0, 0, m_list.m_hWnd, 0, g_hInst, 0);
        //m_list_song.SetListStyle(styleList);

        m_list_song.hide();
        WNDPROC oldProc = (WNDPROC)SetWindowLongPtrW(m_list_song.m_hWnd, GWLP_WNDPROC, (LONG_PTR)Window_ListSongProc);
        m_list_song.SetPropW(L"proc", oldProc);

        style = WS_TABSTOP | WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | WS_VSCROLL | CBS_DROPDOWNLIST | CBS_AUTOHSCROLL | CBS_HASSTRINGS;
        styleEx = WS_EX_CLIENTEDGE;
        m_combobox.m_hWnd = CreateWindowExW(styleEx, WC_COMBOBOXW, L"用户列表", style, 66, 8, 150, 200, hWnd, 0, 0, 0);
        SendMessageW(m_combobox.m_hWnd, WM_SETFONT, (WPARAM)hFont, 0);
        m_tips.Insert(m_combobox.m_hWnd, L"第一个是所有用户, 第二个是自己的账户\r\n可以根据需要显示某个人或者全部的歌单", 0, 0);


        GetWindowRect(m_combobox.m_hWnd, &rcCbb);
        cbbHeight = rcCbb.bottom - rcCbb.top;
        cbbWidth = rcCbb.right - rcCbb.left;
        rcCbb.left = 66;
        rcCbb.top = 8;
        rcCbb.bottom = rcCbb.top + cbbHeight;
        rcCbb.right = rcCbb.left + cbbWidth;

        style = WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | SS_RIGHT | SS_CENTERIMAGE;
        HWND hStatic = CreateWindowExW(0, WC_STATICW, L"用户歌单", style, 8, 8, 50, cbbHeight, hWnd, 0, 0, 0);
        SendMessageW(hStatic, WM_SETFONT, (WPARAM)hFont, 0);

        hStatic = CreateWindowExW(0, WC_STATICW, L"搜索歌曲", style, 224, 8, 50, cbbHeight, hWnd, 0, 0, 0);
        SendMessageW(hStatic, WM_SETFONT, (WPARAM)hFont, 0);

        int top = rcCbb.bottom + 5;
        style = WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | SS_LEFT | SS_CENTERIMAGE;
        m_hStatic = CreateWindowExW(0, WC_STATICW, TEXT_STATIC, style, 8, top, 100, 20, hWnd, 0, 0, 0);
        SendMessageW(m_hStatic, WM_SETFONT, (WPARAM)hFont, 0);


        styleEx = WS_EX_CLIENTEDGE | WS_EX_CONTROLPARENT;
        style = WS_TABSTOP | WS_MAXIMIZEBOX | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE | WS_CHILD
            | ES_LEFT | ES_AUTOHSCROLL;

        m_edit.create(styleEx, 0, style, 282, 8, 150, cbbHeight, hWnd, ID_EDIT_FIND);
        const LPCWSTR editTips =
            L"搜索多个关键字请使用逗号或者||分割, 比如 \"加速,儿歌||AC\"\r\n"
            L"    ^模式, 匹配头, 比如输入 \"^re\",  那么就会列出所有\"re\"开头的歌曲/歌手\r\n"
            L"    $模式, 匹配尾, 比如输入 \"$re\",  那么就会列出所有\"re\"结尾的歌曲/歌手\r\n"
            L"    #模式, 搜索连击数, 比如输入 \"#1104\" 就会列出连击是1104的歌曲\r\n"
            L"    *模式, 搜索等级, 比如输入 \"*9.5\" 就会列出所有9.5星的歌曲\r\n"
            L"    !模式, 搜索BPM, 比如输入 \"!180\" 就会列出所有BPM等于180的歌曲\r\n"
            L"    %模式, 搜索时长, 比如输入 \"*120\" 就会列出所有120秒的歌曲\r\n"
            L"    &&并且条件, 需要多个条件同时满足, 请参考下面的例子\r\n"
            L"    连击/星级/时长/BPM 支持比较符号, >, <, >=, <=, 分别是大于, 小于, 大于等于, 小于等于\r\n"
            L"        输入, \"#>1000\" 会列出所有连击大于1000的歌曲\r\n"
            L"        输入, \"!<130\" 会列出所有BPM小于130的歌曲\r\n"
            L"        输入, \"*>=8.5\" 会列出所有8.5和大于8.5星的歌曲\r\n"
            L"        输入, \"*>=8.5 && !>200\" 会列出8星以上并且BPM大于200的歌曲, 不包含8星\r\n"
            L"    搜索支持多条件搜索, && 是并且的条件, 逗号\",\"或者\"||\"是或者的条件, 比如:\r\n"
            L"        输入, \"%>=120 && %<=150 && #>999 || !>220\"\r\n"
            L"        这个搜索模式会把 大于等于120秒, 并且小于等于150秒的歌曲\r\n"
            L"        并且 连击大于999, 又或者 BPM大于220的歌曲\r\n"
            L"        这个模式会列出时长在 120-150秒之内连击大于999的歌曲\r\n"
            L"        还会列出BPM大于220的歌曲\r\n"
            L"    更多搜索方式等待后续更新\r\n\r\n"
            ;
        m_tips.Insert(m_edit, editTips, 0, 0);

        styleEx = WS_EX_CLIENTEDGE | WS_EX_CONTROLPARENT;
        style = WS_TABSTOP | WS_MAXIMIZEBOX | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE | WS_CHILD
            | ES_LEFT | ES_AUTOHSCROLL | ES_WANTRETURN;
        m_editInsert.create(styleEx, 0, style, 0, 0, 0, 0, m_list.m_hWnd);
        m_editInsert.hide();

        m_tips.Insert(m_editInsert, editInsertTips, 0, 0);
        oldProc = (WNDPROC)SetWindowLongPtrW(m_editInsert.m_hWnd, GWLP_WNDPROC, (LONG_PTR)Window_ListViewEditProc);
        m_editInsert.SetPropW(L"proc", oldProc);
        

        style = WS_TABSTOP | WS_MAXIMIZEBOX | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE | WS_CHILD
            | ES_LEFT | ES_MULTILINE;
        m_editRemark.create(styleEx, 0, style, 432, 8, cbbHeight + 8, cbbHeight, hWnd);
        oldProc = (WNDPROC)SetWindowLongPtrW(m_editRemark.m_hWnd, GWLP_WNDPROC, (LONG_PTR)Window_EditRemarkProc);
        m_editRemark.SetPropW(L"proc", oldProc);

        style = WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE | WS_CHILD | SS_REALSIZEIMAGE | SS_CENTERIMAGE;
        m_songImage.create(0, 0, style, 432, 8, cbbHeight + 8, cbbHeight, hWnd);
        oldProc = (WNDPROC)SetWindowLongPtrW(m_songImage.m_hWnd, GWLP_WNDPROC, (LONG_PTR)Window_SongImageProc);
        m_songImage.SetPropW(L"proc", oldProc);


        MoveWindow(hWnd, rc.left, rc.top, width, height, true);

        titleInc = 0;
        SetWindowTextW(hWnd, L"歌单列表 - 正在获取歌单列表");
        SetTimer(hWnd, 10086, 150, 0);

        m_menuWnd.create(hWnd, CreateMenu());
        HMENU hMenuPop = CreatePopupMenu();
        AppendMenuW(hMenuPop, MF_STRING, ID_MENU_INJECT, L"启用外部选歌功能");
        AppendMenuW(hMenuPop, MF_STRING, ID_MENU_IMPORTFILE, L"导入外部歌单");
        AppendMenuW(hMenuPop, MF_STRING, ID_MENU_OPENLOGIN, L"打开登录器");
        AppendMenuW(hMenuPop, MF_STRING, ID_MENU_OPENR2HOCK, L"打开助手");
        AppendMenuW(hMenuPop, MF_STRING, ID_MENU_SETTING, L"设置");
        m_menuWnd.add(hMenuPop, L"文件");

        hMenuPop = CreatePopupMenu();
        AppendMenuW(hMenuPop, MF_STRING, ID_MENU_ZANZHU, L"友情赞助");
        AppendMenuW(hMenuPop, MF_STRING, ID_MENU_HELP, L"帮助");
        m_menuWnd.add(hMenuPop, L"帮助");
        SetMenu(hWnd, m_menuWnd);

        m_menu_song.create(hWnd);
        m_menu_song.add(ID_MENU_AUTOPLAY, L"选中自动播放");
        m_menu_song.add((UINT)0, 0, MF_SEPARATOR);
        m_menu_song.add(ID_MENU_SORT_LEVEL, L"按星级排序");
        m_menu_song.add(ID_MENU_SORT_NAME, L"按歌名排序");
        m_menu_song.add(ID_MENU_SORT_ARTIST, L"按歌手排序");

        m_menu.create(hWnd);
        m_menu.add(ID_MENU_STOP, L"停止播放");
        m_menu.add(ID_MENU_LOGIN, L"登录");
        m_menu.add(ID_MENU_GETDATA, L"从服务器拉取数据");
        m_menu.add(ID_MENU_UPDATE, L"提交数据到服务器");
        m_menu.add(ID_MENU_EDIT, L"编辑项目(选中后隔一秒左右再点击一次可快速进入编辑)");

        m_menu.add((UINT)0, 0, MF_SEPARATOR);
        hMenuPop = CreatePopupMenu();
        AppendMenuW(hMenuPop, MF_STRING, ID_MENU_SORT_LV, L"按等级排序");
        AppendMenuW(hMenuPop, MF_STRING, ID_MENU_SORT_COMBO, L"按连击排序");
        AppendMenuW(hMenuPop, MF_STRING, ID_MENU_SORT_TIMER, L"按时长排序");
        AppendMenuW(hMenuPop, MF_STRING, ID_MENU_SORT_SONG, L"按歌名排序");
        AppendMenuW(hMenuPop, MF_STRING, ID_MENU_SORT_SINGER, L"按歌手排序");
        AppendMenuW(hMenuPop, MF_STRING, ID_MENU_SORT_BPM, L"按BPM排序");
        m_menu.add(hMenuPop, L"排序");
        
        m_menu.add((UINT)0, 0, MF_SEPARATOR);
        m_menu.add(ID_MENU_ADD, L"添加歌曲");
        m_menu.add(ID_MENU_DEL, L"删除歌曲");
        m_menu.add(ID_MENU_CLEAR, L"清空歌曲");
        m_menu.add(ID_MENU_ADDMY, L"将选中歌曲添加到自己的歌单中");
        m_menu.add(ID_MENU_ADDMYALL, L"将当前歌单添加到自己的歌单中");
        m_menu.add((UINT)0, 0, MF_SEPARATOR);
        m_menu.add(ID_MENU_HELP, L"帮助");

        m_menuAll.create(hWnd);
        m_menuAll.add(ID_MENU_PLAY, L"播放");
        m_menuAll.add(ID_MENU_STOP, L"停止播放");
        m_menuAll.add((UINT)0, 0, MF_SEPARATOR);
        m_menuAll.add(ID_MENU_SORT_LV, L"按等级排序");
        m_menuAll.add(ID_MENU_SORT_COMBO, L"按连击排序");
        m_menuAll.add(ID_MENU_SORT_TIMER, L"按时长排序");
        m_menuAll.add(ID_MENU_SORT_SONG, L"按歌名排序");
        m_menuAll.add(ID_MENU_SORT_SINGER, L"按歌手排序");
        m_menuAll.add(ID_MENU_SORT_BPM, L"按BPM排序");
        //m_menuAll.add((UINT)0, 0, MF_SEPARATOR);
        //m_menuAll.add(ID_MENU_ADDMY, L"添加到自己的歌单中");
        m_menuAll.add((UINT)0, 0, MF_SEPARATOR);
        m_menuAll.add(ID_MENU_ZANZHU, L"友情赞助");
        m_menuAll.add(ID_MENU_HELP, L"帮助");

        break;
    }
    case WM_TIMER:
    {
        szTitle.reserve(100);
        szTitle.assign(L"歌单列表 - 正在获取歌单列表").append(++titleInc, '.');
        SetWindowTextW(hWnd, szTitle.c_str());
        if ( titleInc > 10 )
            titleInc = 0;
        break;
    }
    case WM_SIZE:
    {
        const int cxClient = LOWORD(lParam);
        const int cyClient = HIWORD(lParam);
        const int listTop = 200;    // 超列顶部预留区域, 用来做搜索, 增删改查什么的
        const int listBottom = 0;   // 超列底部预留区域, 用来显示详细信息的
        const int border = 8;

        int maxColWidth = 23;   // 滚动条宽度, 不让列表出现横向滚动条
        for ( int i : colWidth )
            maxColWidth += i;

        const int listColWidth = cxClient - maxColWidth - border * 2;
        m_list.SetColumnWidth(LIST_SONGNAME, listColWidth);

        m_edit.Move(282, 8, cxClient - border - 282, cbbHeight);

        int top = rcCbb.bottom + 5;
        MoveWindow(m_hStatic, 8, top, cxClient - border * 2, 20, TRUE);
        top = rcCbb.bottom + 25;
        const int imgWidth = 256;
        const int imgHeight = 128;
        const int editWidth = cxClient - border * 2 - imgWidth - border;
        const int editHeight = listTop - top - 5;
        m_editRemark.Move(border, top, editWidth, editHeight);

        top = top + ( editHeight - imgHeight ) / 2;
        m_songImage.Move(editWidth + border * 2, top, imgWidth, imgHeight);
        m_list.Move(border, listTop, cxClient - border * 2, cyClient - listTop - listBottom - border);
        break;
    }
    case WM_COMMAND:
    {
        const int code = HIWORD(wParam);
        const int id = LOWORD(wParam);
        switch ( code )
        {
        case CBN_SELCHANGE:
        {
            UpdateListData(true);
            break;
        }
        default:
            return (INT_PTR)Window_OnCommand(hWnd, message, wParam, lParam);
        }

        break;
    }
    case WM_GETMINMAXINFO:
    {
        LPMINMAXINFO pos = (LPMINMAXINFO)lParam;

        // 允许调整的最小尺寸
        pos->ptMinTrackSize.x = 600;    // 最小拖动大小
        pos->ptMinTrackSize.y = 480;

        break;
    }
    case WM_CLOSE:
    {
        DestroyWindow(hWnd);
        break;
    }
    case WM_DESTROY:
    {
        m_tips.Destroy();
        RECT rc;
        wchar_t buf[100];
        GetWindowRect(hWnd, &rc);
        swprintf_s(buf, 100, L"%d,%d,%d,%d", rc.left, rc.top, rc.right, rc.bottom);
        mp3PlayStop(false);
        g_sortMode = SORT_MODE_NONE;
        g_ini->write(INI_APPNAME, L"rcDlg", buf);
        if ( !g_token.empty() )
            newThread(ThreadProc_OffLine, 0, true);
        //PostQuitMessage(0);
        PostMessageA(g_argData->hWndR2Hock, WM_CLOSE, 0, 0);
        break;
    }
    case WM_NOTIFY:
        return Window_OnNotify(hWnd, message, wParam, lParam);
    default:
        return (INT_PTR)false;
    }
    return (INT_PTR)true;
}

int reLoad_show_list(std::vector<PLIST_DATA_STRUCT>& arr, LPCWSTR* username)
{
    int index = m_combobox.GetSel();
    int flag = m_combobox.GetParam(index);
    arr.clear();
    if ( m_isLocalData && index == 0 )
        index = 1;
    m_isR2SongList = false;
    if ( flag == 123 )
    {
        m_isR2SongList = true;
        arr.reserve(g_r2SongList.size());
        for ( LIST_DATA_STRUCT& item : g_r2SongList )
            arr.push_back(&item);
        return index;
    }

    if ( index == 0 )
    {
        // 显示所有用户
        if ( username )
        {
            *username = L"所有用户";
        }
        arr.reserve(100);
        for ( USER_ARR_DATA& userData : g_user )
        {
            for ( LIST_DATA_STRUCT& item : userData.arr )
            {
                arr.push_back(&item);
            }
        }
    }
    else
    {
        // 显示指定用户的歌单
        int count = (int)g_user.size();
        if ( index <= count || index > 0 )
        {
            arr.reserve(count);

            USER_ARR_DATA& userData = g_user[index - 1];
            if ( username )
                *username = userData.username;
            for ( LIST_DATA_STRUCT& item : userData.arr )
            {
                arr.push_back(&item);
            }
        }
    }
    return index;
}

// 根据组合框选中项更新显示的列表
void UpdateListData(bool isUpdateTitle)
{
    LPCWSTR username = 0;
    const int index = reLoad_show_list(g_data, &username);



    if ( !m_edit.GetText().empty() )
    {
        // 有值, 切换后还继续搜索
        btn_find();
        const int indexList = m_list.GetIndex();
        if ( indexList != -1 && indexList < (int)g_data.size() )
            m_editRemark.SetText(g_data[indexList]->pszRemark);
        m_index = -1;
        return;
    }

    int count = (int)g_data.size();

    // 显示的是音速歌单
    if ( isUpdateTitle )
    {
        wstr szTitle;
        username = m_isR2SongList ? L"音速曲库" : username;
        szTitle.Format(L"歌单列表 - 加载 [%s] 的歌单, 一个有 %d 首歌", username, count);
        SetWindowTextW(g_hWndDlg, szTitle.c_str());
    }

    const int indexList = m_list.GetIndex();
    if ( indexList != -1 && indexList < (int)g_data.size() )
        m_editRemark.SetText(g_data[indexList]->pszRemark);
    m_index = -1;

    m_list.SetItemCount(count);
    m_list.InvalidateRect();
}

// 获取窗口位置
void Dlg_rcWindow(RECT* prc, LPCWSTR keyName)
{
    RECT& rc = *prc;
    const LPCWSTR def = L"0,0,600,480";
    wstr str = g_ini->read(INI_APPNAME, keyName, def);
    swscanf_s(str.c_str(), L"%d,%d,%d,%d", &rc.left, &rc.top, &rc.right, &rc.bottom);

    int width = 600;
    int height = 480;

    int cx = GetSystemMetrics(SM_CXSCREEN);                     // 屏幕宽度
    int cy = GetSystemMetrics(SM_CYMAXIMIZED);                  // 屏幕高度 - 任务栏
    int cxScreen = GetSystemMetrics(SM_CXVIRTUALSCREEN);        // 所有屏幕宽度总和
    int cyScreen = GetSystemMetrics(SM_CYVIRTUALSCREEN);        // 所有屏幕高度总和

    if ( str == def )
    {
        rc.left = ( cx - width ) / 2;
        rc.top = ( cy - height ) / 2;
        rc.right = rc.left + width;
        rc.bottom = rc.top + height;
    }
    else
    {
        if ( rc.right - rc.left < width )
            rc.right = rc.left + width;
        if ( rc.bottom - rc.top < height )
            rc.bottom = rc.top + height;
    }

    width = rc.right - rc.left;
    height = rc.bottom - rc.top;

    if ( height > cxScreen )      // 如果高度大于屏幕高度, 则设置为屏幕高度
        rc.top = cxScreen - height;
    if ( width > cxScreen )       // 如果宽度大于屏幕宽度, 则设置为屏幕宽度
        rc.left = cxScreen - width;
    if ( rc.left < -width )       // 如果左边位置小于窗口宽度的负数, 表示窗口左边加宽度不会显示到屏幕上
        rc.left = 0;
    if ( rc.top < -height )       // 如果顶边位置小于窗口高度的负数, 表示窗口顶边加高度不会显示到屏幕上
        rc.top = 0;
    if ( rc.left > cxScreen )     // 如果左边位置大于所有屏幕总宽度, 则设置左边位置 = 屏幕总宽度 - 窗口宽度
        rc.left = cxScreen - width;
    if ( rc.top > cyScreen )      // 如果顶边位置大于所有屏幕总高度, 则设置顶边位置 = 屏幕总高度 - 窗口高度
        rc.top = cyScreen - height;

    rc.right = rc.left + width;
    rc.bottom = rc.top + height;
    return;

}


// 显示用户歌单的
LRESULT CALLBACK Window_ListViewProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch ( message )
    {
    case WM_NOTIFY:
    {
        LPNMHDR hdr = (LPNMHDR)lParam;
        switch ( hdr->code )
        {
        case LVN_GETDISPINFOW:
        {
            LPNMLVDISPINFOW info = (LPNMLVDISPINFOW)lParam;
            const int index = info->item.iItem;
            if ( index < 0 || index >= (int)g_songListShow.size() )
                return (INT_PTR)false;

            PLIST_DATA_STRUCT& item = g_songListShow[index];
            info->item.pszText = (LPWSTR)item->pszShowName;

            if ( info->item.pszText )
                info->item.cchTextMax = wcslen(info->item.pszText);
            break;
        }
        case LVN_GETDISPINFOA:
        {
            break;
        }
        default:
            return CallWindowProcW(m_oldProcList, hWnd, message, wParam, lParam);
        }
        break;
    }
    case WM_COMMAND:
    {
        const int id = LOWORD(wParam);
        const int code = HIWORD(wParam);
        HWND hChild = (HWND)lParam;
        if ( hChild != m_editInsert && hChild != m_list_song )
            return CallWindowProcW(m_oldProcList, hWnd, message, wParam, lParam);
        switch ( code )
        {
        case EN_CHANGE:
        {
            wstr str = m_editInsert.GetText();
            LPWSTR find = str.data();
            while ( *find )
            {
                wchar_t& ch = *find++;
                switch ( ch )
                {
                case L'：': ch = ':'; break;
                case L'，': ch = ','; break;
                case L'》': ch = '>'; break;
                case L'《': ch = '<'; break;
                default: break;
                }
            }
            int count = FindSongFromSongList(str.c_str(), FIND_SONGLIST_NAME);
            m_list_song.SetItemCount(count);
            m_list_song.InvalidateRect();
            break;
        }
        case LBN_DBLCLK:
        {
            const int index = m_list_song.GetIndedx();
            if ( index < 0 )
                break;

            LIST_DATA_STRUCT& item = *g_data[m_index];
            PLIST_DATA_STRUCT pSongData = g_songListShow[index];
            if ( pSongData->nCombo == 0 )
            {
                GetSongInfoFromSongList(pSongData);
            }
            LPCWSTR songNameW = pSongData->pszSongName;
            LPCWSTR SingerNameW = pSongData->pszSingerName;
            bool isPlay = g_ini->read_int(INI_APPNAME, L"isAutoPlay", 1);
            m_isDblClkEditEnd = true;
            mp3PlayStop(false);
            // 双击后只修改 歌名, 
            if ( // 全部匹配的就是没有修改, 需要返回, 只要有一个不同, 那就走到下面修改
                wcscmp(item.pszSongName, songNameW) == 0
                && wcscmp(item.pszSingerName, SingerNameW) == 0
                && item.nLevel == pSongData->nLevel
                && item.nCombo == pSongData->nCombo
                && item.nTimer == pSongData->nTimer
                && item.nBpm == pSongData->nBpm
                )
            {
                m_list_song.hide();
                m_editInsert.hide();
                m_editInsert.SetText(L"");
                break;
            }

            // 双击后需要更新用户的歌名, 歌曲时长, 歌曲连击, 根据备注文本给备注修改/增加歌手等其他一些信息
            _set_list_buf_text(&item.pszSongName, songNameW);
            _set_list_buf_text(&item.pszSingerName, SingerNameW);
            _set_list_buf_text(&item.pszTimer, pSongData->pszTimer);
            _set_list_buf_text(&item.pszCombo, pSongData->pszCombo);
            _set_list_buf_text(&item.pszLevel, pSongData->pszLevel);
            _set_list_buf_text(&item.pszShowName, pSongData->pszShowName);
            item.nCombo = pSongData->nCombo;
            item.nTimer = pSongData->nTimer;
            item.nLevel = pSongData->nLevel;
            item.nBpm   = pSongData->nBpm;
            item.pszBpm = g_buf.AddFormat(L"%d", item.nBpm);


            // 新的备注文本
            wstr remark(520);
            remark = item.pszRemark;
            {
                // 基础信息: 歌曲 - 歌手, 等级, 连击, 时长\r\n
                // 其他关键字:
                wstr info = remark.substr(L"基础信息:", L"\r\n");
                if ( info.empty() )
                {
                    // 没有基础信息, 那就把基础信息插入到前面
                    wstr str(100);
                    str.append(L"基础信息:").append(item.pszShowName);
                    str.append(L", *").append(item.nLevel);
                    str.append(L", #").append(item.nCombo);
                    str.append(L", %").append(item.nTimer);
                    str.append(L", !").append(item.nBpm);
                    str.append(L"\r\n");
                    remark.insert(0, str);
                }
                else
                {
                    // 原来有歌手歌曲, 那就修改冒号到换行的内容
                    wstr str(100);
                    str.append(item.pszShowName);
                    str.append(L", *").append(item.nLevel);
                    str.append(L", #").append(item.nCombo);
                    str.append(L", %").append(item.nTimer);
                    remark.replace(info, str);
                }

                if ( remark.find(L"其他关键字:") == wstr::npos )
                {
                    // 没有其他关键字, 增加一个
                    if ( remark.back() != '\n' )
                        remark.append(L"\r\n");
                    remark.append(L"其他关键字:\r\n");
                }
                m_editRemark.SetText(remark.c_str());
                m_editInsert.SetText(item.pszShowName);
                _set_list_buf_text(&item.pszRemark, remark.c_str());
            }

            item.clrText = COLOR_TEXT_ERR;
            ListSong_GetListData(true);
            m_list.InvalidateRect();

            m_list_song.hide();
            m_editInsert.hide();
            break;
        }
        case LBN_SELCHANGE:
        {
            // 原来不是选中, 那点击后就是选中, 播放歌曲
            const int index = m_list_song.GetIndedx();
            if ( index >= 0 )
            {
                if ( g_ini->read_int(INI_APPNAME, L"isAutoPlay", 1) )
                {
                    GetR2SongListData(g_songListShow[index], MP3_FLAGS_PLAY15);
                }
            }
            break;
        }
        case LBN_KILLFOCUS:
        {
            HWND hWndCocus = GetFocus();
            if ( hWndCocus != m_editInsert && m_isDblClkEditEnd == false )
            {
                // 列表框失去了焦点, 并且当前焦点不是编辑框
                _send_notify_edit_end(true);
            }
            break;
        }
        default:
            return CallWindowProcW(m_oldProcList, hWnd, message, wParam, lParam);
        }
        break;
    }
    case WM_MEASUREITEM:
    {
        LPMEASUREITEMSTRUCT draw = (LPMEASUREITEMSTRUCT)lParam;
        if ( draw->CtlType != ODT_LISTBOX )
            break;

        draw->itemWidth = LISTBOX_SONGLIST_WIDTH;
        draw->itemHeight = 20;

        return 1;
    }
    case WM_DRAWITEM:
    {
        LPDRAWITEMSTRUCT draw = (LPDRAWITEMSTRUCT)lParam;
        if ( draw->CtlType != ODT_LISTBOX )
            break;
        const int index = (int)draw->itemID;
        if ( index < 0 || index >= (int)g_songListShow.size() )
            break;

        PLIST_DATA_STRUCT pItem = g_songListShow[index];
        HDC hdc = draw->hDC;
        RECT& rcItem = draw->rcItem;
        HGDIOBJ hOldFont = SelectObject(hdc, hFont);
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(0, 0, 0));
        if ( __query(draw->itemState, ODS_SELECTED) )
        {
            // 选中
            HBRUSH hbrBack = CreateSolidBrush(RGB(0, 120, 215));
            FillRect(hdc, &rcItem, hbrBack);
            DeleteObject(hbrBack);
            SetTextColor(hdc, RGB(255, 255, 255));
        }
        else if ( __query(draw->itemState, ODS_HOTLIGHT) )
        {
            // 热点
            HBRUSH hbrBack = CreateSolidBrush(COLOR_TEXT_BACK_ERR);
            FillRect(hdc, &rcItem, hbrBack);
            DeleteObject(hbrBack);
        }
        else
        {
            // 正常状态, 填充白色背景, 绘画黑色字体
            FillRect(hdc, &rcItem, GetStockBrush(WHITE_BRUSH));
        }

        DrawTextW(hdc, pItem->pszShowNameLv, -1, &rcItem, DT_LEFT | DT_SINGLELINE | DT_VCENTER);
        SelectObject(hdc, hOldFont);
        return 1;
    }
    default:
        return CallWindowProcW(m_oldProcList, hWnd, message, wParam, lParam);
    }
    return 0;
}

// 向超列的父窗口发送编辑结束的通知
void _send_notify_edit_end(bool isChange)
{
    NMLVDISPINFOW di = { 0 };
    di.hdr.hwndFrom     = m_list;
    di.hdr.idFrom       = 0;
    di.hdr.code         = LVN_ENDLABELEDIT;
    di.item.mask        = LVIF_TEXT;
    di.item.iItem       = m_editData.iItem;
    di.item.iSubItem    = m_editData.iSubItem;
    if ( isChange )
    {
        wstr title = m_editInsert.GetText();
        if ( !title.empty() )
        {
            LPCWSTR text = ListView_GetTextFromColIndex(m_editData.iItem, m_editData.iSubItem);
            int textLen = (int)title.size();
            if ( wcscmp(title.c_str(), text) != 0 )
            {
                di.item.pszText = (LPWSTR)g_buf.AddString(title.c_str(), textLen);
                di.item.cchTextMax = textLen;
            }
        }
    }
    m_editInsert.hide();
    m_list_song.hide();
    SendMessageW(g_hWndDlg, WM_NOTIFY, 0, (LPARAM)&di);
}

//! 超列中编辑歌名使用的编辑框消息回调
LRESULT CALLBACK Window_ListViewEditProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    WNDPROC oldProc = (WNDPROC)GetPropW(hWnd, L"proc");
    switch ( message )
    {
    case WM_KILLFOCUS:
    {
        HWND hWndFocus = GetFocus();
        if ( hWndFocus != m_list_song )
        {
            _send_notify_edit_end(true);
        }
        m_tips.hide();
        break;
    }
    case WM_CHAR:
    {
        switch ( wParam )
        {
        case VK_RETURN:
        {
            _send_notify_edit_end(true);
            return 0;
        }
        case VK_ESCAPE:
        {
            _send_notify_edit_end(false);
            return 0;
        }
        default:
            break;
        }

        //wstr::dbg(L"%d\n", wParam);
        break;
    }
    default:
        break;
    }
    return CallWindowProcW(oldProc, hWnd, message, wParam, lParam);
}

LRESULT CALLBACK Window_SongImageProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    WNDPROC oldProc = (WNDPROC)GetPropW(hWnd, L"proc");
    switch ( message )
    {
    case WM_PAINT:
    {
        RECT rc;
        PAINTSTRUCT ps;
        BeginPaint(hWnd, &ps);
        GetClientRect(hWnd, &rc);
        COLORREF clr = GetSysColor(COLOR_BTNFACE);
        if ( m_image )
        {
            Gdiplus::Rect rcP(rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top);
            Gdiplus::Graphics gp(ps.hdc);
            gp.Clear(RGB2ARGB(clr, 255));
            gp.DrawImage(m_image, rcP);
        }
        else
        {
            HBRUSH hbr = CreateSolidBrush(clr);
            FillRect(ps.hdc, &rc, hbr);
            DeleteObject(hbr);

            int index = m_list.GetIndex();
            if ( index >= 0 && index < (int)g_data.size() )
            {
                PLIST_DATA_STRUCT pItem = g_data[index];
                wstr text;
                text.assign(L"可能pak文件被加密了, 没找到图片\r\n图片路径为: \r\n");// .append(_str::A2W(g_argData->pszR2Path));
                text.append(pItem->pszImage).append(L"\r\n");
                text.replace('/', '\\');
                SelectObject(ps.hdc, CWndBase::DefaultFont());
                SetBkMode(ps.hdc, TRANSPARENT);
                SetTextColor(ps.hdc, RGB(255, 0, 0));
                DrawTextW(ps.hdc, text.c_str(), text.size(), &rc, DT_EDITCONTROL | DT_WORDBREAK);
            }
        }
        EndPaint(hWnd, &ps);
        return 0;
    }
    default:
        return CallWindowProcW(oldProc, hWnd, message, wParam, lParam);
    }
    return 0;
}
// 备注编辑框消息回调, 需要判断是否允许输入字符, 在失去焦点的时候需要判断数据是否修改
LRESULT CALLBACK Window_EditRemarkProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    WNDPROC oldProc = (WNDPROC)GetPropW(hWnd, L"proc");
    switch ( message )
    {
    case WM_KILLFOCUS:
    {
        // 备注文本编辑框失去焦点, 需要判断是否需要更改内容
        wstr text = m_editRemark.GetText();
        // 更新备注文本, 这个函数内部会判断是否需要更新
        //ListView_SetTextFromColIndex(m_index, LIST_REMARKS, text.c_str());

        break;
    }
    case WM_CHAR:
    {
        if ( !m_isEdit )
        {
            bool isReturn = true;
            const int key[] =
            {
                0x01, 0x03,
            };
            for ( int k : key )
            {
                if ( wParam == k )
                {
                    isReturn = false;
                    break;
                }
            }
            if ( isReturn )
                return 0;
            // 不允许编辑, 只允许部分字符输入
            wstr::dbg(L"允许的键: %02X\n", wParam);
        }
        return CallWindowProcW(oldProc, hWnd, message, wParam, lParam);
    }
    default:
        break;
    }
    return CallWindowProcW(oldProc, hWnd, message, wParam, lParam);
}

inline void update_list()
{
    const int count = (int)g_data.size();
    m_list.SetItemCount(count);
    m_list.InvalidateRect();
}


inline void _re_g_data()
{
    int index = m_combobox.GetSel();
    PUSER_ARR_DATA userData = (PUSER_ARR_DATA)m_combobox.GetParam(index);
    if ( (int)userData == 123 )
    {
        // 是音速歌单
        g_data.clear();
        for ( LIST_DATA_STRUCT& item : g_r2SongList )
        {
            g_data.push_back(&item);
        }
    }
    else if ( userData == 0 )    // 当前显示的是所有的项目, g_data从全部的成员里重新设置
    {
        g_data.clear();
        for ( USER_ARR_DATA& userData : g_user )
        {
            for ( LIST_DATA_STRUCT& item : userData.arr )
            {
                g_data.push_back(&item);
            }
        }
    }
    else
    {
        // 显示指定用户的歌单
        std::vector<LIST_DATA_STRUCT>& arr = userData->arr;
        g_data.resize(arr.size());
        int i = 0;
        for ( LIST_DATA_STRUCT& item : arr )
            g_data[i++] = &item;
    }

}

// 返回1表示歌名相同, 不添加到数组, 只修改
// 返回0表示没有任何地方相同, 需要添加到数组
// 返回2表示所有地方都相同, 不加入数组不修改
inline int _item_compare(LIST_DATA_STRUCT& item1, LIST_DATA_STRUCT& item2)
{
#define _COMPARE_STR(_s) (wstr::compare(item1._s, item2._s, true) == 0)
     
    int ret = 0;
    if ( _COMPARE_STR(pszSongName) )
        ++ret;
    

    if ( _COMPARE_STR(pszCombo) && _COMPARE_STR(pszTimer) && _COMPARE_STR(pszSongName) && _COMPARE_STR(pszRemark) )
        ++ret;
    return ret;
}


// 将项目添加到数组中, 会判断数组中有没有这个项目, 有就不添加, 返回是否已添加
inline bool _set_item_data(std::vector<LIST_DATA_STRUCT>& userArr, LIST_DATA_STRUCT& _item)
{
    int change = 0; // 2=返回, 1=修改, 0=添加
    LPCWSTR userName = g_user[0].username;
    PLIST_DATA_STRUCT pItem = 0;
    for ( LIST_DATA_STRUCT& item : userArr )
    {
        int ret = _item_compare(item, _item);
        if ( ret == 2 )
        {
            change = ret;
            break;
        }
        if ( ret == 1 )
        {
            change = ret;
            pItem = &item;
            break;
        }
    }
    if ( change == 2 )
    {
        // 重复项目, 不添加到数组里
        return false;
    }

    
    if ( change == 1 && pItem )
    {
        // 修改项目, 歌曲名不用改
        _set_list_buf_text(&pItem->pszCombo   , _item.pszCombo   );
        _set_list_buf_text(&pItem->pszTimer   , _item.pszTimer   );
        _set_list_buf_text(&pItem->pszRemark  , _item.pszRemark);
        _set_list_buf_text(&pItem->userName, userName);
        return true;
    }

    LIST_DATA_STRUCT item = _item;
    item.pszLevel       = g_buf.AddString(item.pszLevel);
    item.pszCombo       = g_buf.AddString(item.pszCombo   );
    item.pszTimer       = g_buf.AddString(item.pszTimer   );
    item.pszSongName    = g_buf.AddString(item.pszSongName);
    item.pszRemark      = g_buf.AddString(item.pszRemark);
    item.userName       = g_buf.AddString(userName);
    item.pszBpm         = g_buf.AddFormat(L"%d", item.nBpm);
    item.pszShowName    = g_buf.AddFormat(L"%s - %s", item.pszSongName, item.pszSingerName);
    item.pszShowNameLv  = g_buf.AddFormat(L"%s %s", item.pszLevel, item.pszShowName);
    item.clrText = COLOR_TEXT_ERR;
    item.isUser(true);
    userArr.push_back(item);
    return true;
}


inline void menu_command(int id)
{
    if ( g_user.empty() )
        return;
    USER_ARR_DATA& userDataCurrent = g_user[0];
    std::vector<LIST_DATA_STRUCT>& userArr = userDataCurrent.arr;
    LPCWSTR userName = userDataCurrent.username;

    //int index = m_combobox.GetSel();
    //PUSER_ARR_DATA userData = (PUSER_ARR_DATA)m_combobox.GetParam(index);
    //std::vector<LIST_DATA_STRUCT>& arr = userData->arr;
    switch ( id )
    {
    case ID_MENU_ADD:       // 添加项目
    {
        LIST_DATA_STRUCT item;
        item.isUser(true);
        item.userName = g_buf.AddString(userName);
        item.clrText = COLOR_TEXT_ERR;

        userArr.push_back(item);
        _re_g_data();
        break;
    }
    case ID_MENU_DEL:       // 删除项目
    {
        int index = m_list.GetIndex();
        if ( index < 0 )
            return;
        if ( MessageBoxW(g_hWndDlg,
                         L"是否清除当前选中歌曲?\r\n"
                         L"清除后如果提交到服务器后, 数据将不可恢复\r\n\r\n"
                         L"是否要删除?", L"是否删除?",
                         MB_ICONQUESTION | MB_YESNO) == IDNO )
        {
            return; // 返回, 不往下执行, break跳出的话, 后面会把当前歌单写出到磁盘
        }

        userArr.erase(userArr.begin() + index);
        m_index = m_list.GetIndex();
        if ( m_index == -1 )
        {
            m_editRemark.SetText(L"");
        }
        else
        {
            PLIST_DATA_STRUCT pItem = g_data[m_index];
            m_editRemark.SetText(pItem->pszRemark);
        }

        _re_g_data();
        break;
    }
    case ID_MENU_CLEAR:     // 清除项目
    {
        if ( MessageBoxW(g_hWndDlg,
                         L"是否清除当前歌单所有歌曲?\r\n"
                         L"清除后如果提交到服务器后, 数据将不可恢复\r\n\r\n"
                         L"是否要清空?", L"是否清空?",
                         MB_ICONQUESTION | MB_YESNO) == IDNO )
        {
            break;
        }

        m_editRemark.SetText(L"");
        userArr.clear();
        g_data.clear();
        m_index = -1;
        break;
    }
    case ID_MENU_ADDMY:     // 将当前歌曲添加到自己的歌单中
    {
        USER_ARR_DATA& userDataCurrent = g_user[0];
        std::vector<LIST_DATA_STRUCT>& userArr = userDataCurrent.arr;
        int index = m_list.GetIndex();
        PLIST_DATA_STRUCT& item = g_data[index];
        _set_item_data(userArr, *item);
        _re_g_data();
        break;
    }
    case ID_MENU_ADDMYALL:  // 将当前歌单添加到自己的歌单中
    {
        for ( PLIST_DATA_STRUCT pItem : g_data )
        {
            if ( pItem->isUser() )
                continue;   // 跳过自己的歌单
            _set_item_data(userArr, *pItem);
        }
        _re_g_data();
        break;
    }
    default:
        break;
    }
    update_list();
    ListSong_GetListData(true); // 保存到磁盘
}
struct SORT_STRUCT
{
    int mode;
    PLIST_DATA_STRUCT pItem1;
    PLIST_DATA_STRUCT pItem2;
    SORT_STRUCT(int mode) :mode(mode), pItem1(0) , pItem2(0) { }
    inline bool operator()(const LIST_DATA_STRUCT& a, const LIST_DATA_STRUCT& b)
    {
        return operator()((PLIST_DATA_STRUCT)&a, (PLIST_DATA_STRUCT)&b);
    }
    inline bool operator()(const PLIST_DATA_STRUCT& a, const PLIST_DATA_STRUCT& b)
    {
        pItem1 = a;
        pItem2 = b;
        switch ( LOWORD(mode) )
        {
        case SORT_MODE_NONE:
            return false;
        case SORT_MODE_LEVEL: return cmp(pItem1->nLevel, pItem2->nLevel);
        case SORT_MODE_COMBO: return cmp(pItem1->nCombo, pItem2->nCombo);
        case SORT_MODE_TIMER: return cmp(pItem1->nTimer, pItem2->nTimer);
        case SORT_MODE_BPM:   return cmp(pItem1->nBpm, pItem2->nBpm);
        case SORT_MODE_ARTIST:return ret(pItem1->pszSingerName, pItem2->pszSingerName);
        default:
            break;
        }

        // 前面没有匹配的后面根据歌名排序
        return ret(pItem1->pszSongName, pItem2->pszSongName);
    }
    inline bool ret(LPCWSTR name1, LPCWSTR name2)
    {
        int cmp = wcscmp(name1, name2);
        bool isGreater = __query(mode, SORT_UPTOLOW);   // 是否从大到小排序
        if ( isGreater ) return cmp > 0;
        return cmp < 0;
    }
    // 根据排序标志返回对应的值, 数值比较, 相等就返回歌名比较
    inline bool cmp(int v1, int v2)
    {
        if ( v1 == v2 )
            return ret(pItem1->pszSongName, pItem2->pszSongName);
        bool isGreater = __query(mode, SORT_UPTOLOW);   // 是否从大到小排序
        if ( isGreater )
            return v1 > v2;
        return v1 < v2;
    }
};

void ListView_SortList(int mode)
{
    if ( g_data.size() < 2 )
        return; // 就一个成员没必要排序了

    const int index = m_combobox.GetSel();
    if ( index == 1 && m_edit.GetText().empty() && 0 )
    {
        std::vector<LIST_DATA_STRUCT>& arr = g_user[0].arr;
        std::sort(arr.begin(), arr.end(), SORT_STRUCT(mode));
        _re_g_data();
    }
    else
    {
        std::sort(g_data.begin(), g_data.end(), SORT_STRUCT(mode));
    }
    LPCWSTR text = L"";
    if ( m_index >= 0 && m_index < (int)g_data.size() )
        text = g_data[m_index]->pszRemark;
    
    m_editRemark.SetText(text);
    m_list.InvalidateRect();
}
void ListSong_SortList(int mode)
{
#define _make_fun_name(_var_name, _cmp) auto _var_name = [](const PSONGDATA_STRUCT& a, const PSONGDATA_STRUCT& b) -> bool{ return _cmp; }

    if ( g_songListShow.size() < 2 )
        return; // 就一个成员没必要排序了

    std::sort(g_songListShow.begin(), g_songListShow.end(), SORT_STRUCT(mode));
    m_list_song.InvalidateRect();
}

INT_PTR CALLBACK Window_OnCommand(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    const int code = HIWORD(wParam);
    const int id = LOWORD(wParam);
    HWND hChild = (HWND)lParam;
    switch ( id )
    {
    case ID_MENU_UPDATE:
    case ID_BTN_UPDATE:     // 更新按钮, 需要更新到服务器上
        UpdateBtnClick();
        break;
    case ID_BTN_FIND:       // 查找
        btn_find();
        break;
    case ID_EDIT_FIND:
        return edit_command(code);
    case ID_MENU_EDIT:
    {
        if ( m_rClickItem > -1 && ( m_rClickSubItem >= LIST_COMBO && m_rClickSubItem <= LIST_BPM ) )
        {
            NMLVDISPINFOW di = { 0 };
            di.hdr.hwndFrom = m_list;
            di.hdr.idFrom = 0;
            di.hdr.code = LVN_BEGINLABELEDIT;

            POINT pt;
            GetCursorPos(&pt);
            ScreenToClient(m_list, &pt);

            di.item.mask = 121007124;
            di.item.iItem = m_rClickItem;
            di.item.iSubItem = m_rClickSubItem;
            SendMessageW(g_hWndDlg, WM_NOTIFY, 0, (LPARAM)&di);

        }
        break;
    }
    case ID_MENU_AUTOPLAY:
    {
        // 这个得到的是选中后改变前的状态
        bool isCheck = m_menu_song.GetCheck(id);
        LPCWSTR isPlay;
        if ( isCheck )
        {
            // 原来是选中, 点击后就是取消选中, 那么就需要停止播放
            isPlay = L"0";
            mp3PlayStop(false);
        }
        else
        {
            // 原来不是选中, 那点击后就是选中, 播放歌曲
            isPlay = L"1";
            const int index = m_list_song.GetIndedx();
            if ( index >= 0 )
            {
                GetR2SongListData(g_songListShow[index], MP3_FLAGS_PLAY);
            }
        }
        g_ini->write(INI_APPNAME, L"isAutoPlay", isPlay);
        m_menu_song.SetCheck(id, !isCheck);
        break;
    }
    case ID_MENU_SORT_LEVEL:    // 按星级排序
    case ID_MENU_SORT_NAME:     // 按歌名排序
    case ID_MENU_SORT_ARTIST:   // 按歌手排序
    {
        g_sortMode = __query(g_sortMode, SORT_UPTOLOW) ? SORT_LOWTOUP : SORT_UPTOLOW;
        switch ( id )
        {
        case ID_MENU_SORT_LEVEL:
            g_sortMode |= SORT_MODE_LEVEL;
            break;
        case ID_MENU_SORT_NAME:
            g_sortMode |= SORT_MODE_NAME;
            break;
        case ID_MENU_SORT_ARTIST:
            g_sortMode |= SORT_MODE_ARTIST;
            break;
        default:
            return true;
        }
        ListSong_SortList(g_sortMode);

        break;
    }



    case ID_MENU_SORT_LV:
    case ID_MENU_SORT_COMBO:    // 排序, 按连击排序
    case ID_MENU_SORT_TIMER:    // 排序, 按时长排序
    case ID_MENU_SORT_SONG:     // 排序, 按歌名排序
    case ID_MENU_SORT_SINGER:   // 排序, 按歌手排序
    case ID_MENU_SORT_BPM:
    {
        g_sortModeListView = __query(g_sortModeListView, SORT_UPTOLOW) ? SORT_LOWTOUP : SORT_UPTOLOW;
        switch ( id )
        {
        case ID_MENU_SORT_LV:
            g_sortModeListView |= SORT_MODE_LEVEL;
            break;
        case ID_MENU_SORT_COMBO:
            g_sortModeListView |= SORT_MODE_COMBO;
            break;
        case ID_MENU_SORT_TIMER:
            g_sortModeListView |= SORT_MODE_TIMER;
            break;
        case ID_MENU_SORT_SONG:
            g_sortModeListView |= SORT_MODE_NAME;
            break;
        case ID_MENU_SORT_SINGER:
            g_sortModeListView |= SORT_MODE_ARTIST;
            break;
        case ID_MENU_SORT_BPM:
            g_sortModeListView |= SORT_MODE_BPM;
            break;
        default:
            return true;
        }
        ListView_SortList(g_sortModeListView);
        break;
    }
    case ID_MENU_HELP:
    {
        static HWND hDlgHelp;
        if ( IsWindow(hDlgHelp) )
            break;
        struct __DLGTEMPLATE : DLGTEMPLATE
        {
            WORD menuid;
            WORD cls;
            wchar_t caption[1024];
        };

        __DLGTEMPLATE dlg;
        memset(&dlg, 0, sizeof(dlg));
        dlg.style = WS_SYSMENU | WS_CAPTION | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_VISIBLE | WS_THICKFRAME;
        dlg.dwExtendedStyle = WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE | WS_EX_CONTROLPARENT | WS_EX_APPWINDOW;
        dlg.cdit = 0;
        memcpy(dlg.caption, L"帮助", 6);

        static DLGPROC pfn = [](HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)->INT_PTR
        {
            static HWND hEdit;
            switch ( message )
            {
            case WM_INITDIALOG:
            {
                DWORD style = WS_TABSTOP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE | WS_CHILD | WS_VSCROLL
                    | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY | ES_WANTRETURN;
                DWORD styleEx = 0;
                hEdit = CreateWindowExW(styleEx, L"edit", 0, style, 0, 0, 0, 0, hWnd, 0, 0, 0);
                static HFONT hFontEdit;
                if ( !hFontEdit )
                    hFontEdit = CWndBase::CreateFontW(L"微软雅黑", -16);
                
                SendMessageW(hEdit, WM_SETFONT, (WPARAM)hFontEdit, 0);
                const LPCWSTR text =
                    L"左上角用户歌单:\r\n"
                    L"    可以选择查看某个人分享的歌单, 如果不从服务器获取歌单, 那只显示自己的本地歌单\r\n"
                    L"    第一个选项是全部用户的歌单, 可以查看所有在服务器登记记录的用户分享的歌单\r\n"
                    L"    第二个选项是自己的歌单, 只能编辑自己的歌单歌曲, 自己的歌单背景颜色不同\r\n"
                    L"    最后一个选项是音速本地的歌单, 首次打开的时候会遍历音速目录下的所有歌曲\r\n\r\n"
                    L"右上角搜索歌曲, 输入内容直接搜索, 支持以下搜索模式\r\n"
                    L"    ^模式, 匹配头, 比如输入 \"^re\",  那么就会列出所有\"re\"开头的歌曲/歌手\r\n"
                    L"    $模式, 匹配尾, 比如输入 \"$re\",  那么就会列出所有\"re\"结尾的歌曲/歌手\r\n"
                    L"    #模式, 搜索连击数, 比如输入 #1104 就会列出连击是1104的歌曲\r\n"
                    L"    *模式, 搜索等级, 比如输入 \"*9.5\" 就会列出所有9.5星的歌曲\r\n"
                    L"    !模式, 搜索BPM, 比如输入 \"!180\" 就会列出所有BPM等于180的歌曲\r\n"
                    L"    %模式, 搜索时长, 比如输入 \"*120\" 就会列出所有120秒的歌曲\r\n"
                    L"    &&并且条件, 需要多个条件同时满足, 请参考下面的例子\r\n"
                    L"    连击/星级/时长/BPM 支持比较符号, >, <, >=, <=, 分别是大于, 小于, 大于等于, 小于等于\r\n"
                    L"        输入 \"#>1000\" 会列出所有连击大于1000的歌曲\r\n"
                    L"        输入 \"!<130\" 会列出所有BPM小于130的歌曲\r\n"
                    L"        输入 \"*>=8.5\" 会列出所有8.5和大于8.5星的歌曲\r\n"
                    L"        输入 \"*>=8.5 && !>200\" 会列出8星以上并且BPM大于200的歌曲, 不包含8星\r\n"
                    L"    搜索支持多条件搜索, && 是并且的条件, 逗号\",\"或者\"||\"是或者的条件, 比如:\r\n"
                    L"        输入 \"%>=120 && %<=150 && #>999 || !>220\"\r\n"
                    L"        这个搜索模式会把 大于等于120秒, 并且小于等于150秒的歌曲\r\n"
                    L"        并且 连击大于999, 又或者 BPM大于220的歌曲\r\n"
                    L"        这个模式会列出时长在 120-150秒之内连击大于999的歌曲\r\n"
                    L"        还会列出BPM大于220的歌曲\r\n"
                    L"    更多搜索方式等待后续更新\r\n\r\n"
                    L"下来这个中间的大编辑框:\r\n"
                    L"    给歌曲做一些备注, 搜索的时候可以搜索到备注的内容\r\n"
                    L"    比如, \"Oracle\" 这首歌, 我老是记不住歌名, 我只记得他叫\"神谕\"\r\n"
                    L"    那我在 \"Oracle\" 这首歌的备注里记录 \"神谕\", 以后我搜索\"神谕\"就能搜到了\r\n"
                    L"    再比如 Jetking, RetroPoktan, 我只记得叫JK和RP, 那就可以把JK和RP写到备注里\r\n"
                    L"    也可以自己写一些方便搜索或者提醒自己的关键字\r\n\r\n"
                    L"底下的列表:\r\n"
                    L"    显示歌曲列表用的, 点击列标题可以进行排序\r\n"
                    //L"    显示歌单的歌曲用的, 点击列标题可以进行排序\r\n"
                    //L"    不是自己歌单的歌曲, 白色背景黑色字体显示\r\n"
                    //L"    自己歌单的歌曲青色背景, 蓝色字体显示\r\n"
                    //L"    修改过没提交到服务器的歌曲, 红色文本显示, 背景会加深一点\r\n"
                    //L"    选中某个歌曲后, 在歌名那里点击一下, 可以对歌名进行编辑\r\n"
                    //L"    编辑的时候会弹出一个歌曲列表, 双击列表的歌曲可以快速编辑歌曲\r\n\r\n"
                    L"右键菜单:\r\n"
                    L"    基本都是字面意思, 如果有不懂的再说吧....\r\n\r\n"
                    L"有什么建议或者程序有问题的\r\n"
                    L"可以联系福仔, QQ 121007124\r\n\r\n"
                    L"目前有以下想法待实现, 就是时间不是很充足, 也不知道这些功能会不会实现\r\n"
                    L"    1. 点赞功能, 点赞量多的歌曲一般都是做得比较用心, 值得多玩\r\n"
                    L"    2. 点踩功能, 与点赞差不多, 给其他人避坑, 踩得多的基本是粪谱\r\n"
                    L"    3. 收藏, 改备注, 导出收藏, 导入收藏功能\r\n"
                    L"        可以给歌曲做一些备注, 搜索的时候就可以搜索备注了\r\n\r\n"
                    L"目前还存在以下已知问题:\r\n"
                    L"    1. 歌曲列表是直接获取music.txt下的数据, 所以会有部分歌曲游戏里并没有\r\n"
                    L"    2. 只支持528的版本, pro版的选歌call在QQ聊天记录上, Q被封了....\r\n"
                    L"    3. 部分歌曲读不出MP3和图片信息, pak文件被加密, 没有解密工具\r\n"
                    ;

                SetWindowTextW(hEdit, text);

                
                RECT rc;
                Dlg_rcWindow(&rc, L"rcHelp");
                const int width = rc.right - rc.left;
                const int height = rc.bottom - rc.top;

                MoveWindow(hWnd, rc.left, rc.top, width, height, true);
                break;
            }
            case WM_CTLCOLORSTATIC:
            {
                SetWindowLongPtrW(hWnd, DWLP_MSGRESULT, (LONG_PTR)GetStockBrush(WHITE_BRUSH));
                break;
            }
            case WM_SIZE:
            {
                const int cxClient = LOWORD(lParam);
                const int cyClient = HIWORD(lParam);
                MoveWindow(hEdit, 0, 0, cxClient, cyClient, true);
                break;
            }
            case WM_CLOSE:
            {
                RECT rc;
                wchar_t buf[100];
                GetWindowRect(hWnd, &rc);
                swprintf_s(buf, 100, L"%d,%d,%d,%d", rc.left, rc.top, rc.right, rc.bottom);
                g_ini->write(INI_APPNAME, L"rcHelp", buf);

                EndDialog(hWnd, 0);
                DestroyWindow(hWnd);
                break;
            }
            default:
                return 0;
            }
            return 1;
        };


        hDlgHelp = CreateDialogIndirectParamW(g_hInst, &dlg, g_hWndDlg, pfn, 0);
        break;
    }
    case ID_MENU_PLAY:
    {
        const int index = m_list.GetIndex();
        if ( index < 0 )
            break;
        GetR2SongListData(g_data[index], MP3_FLAGS_PLAY);
        break;
    }
    case ID_MENU_STOP:
    {
        mp3PlayStop(false);
        break;
    }
    case ID_MENU_GETDATA:
    {
        if ( g_token.empty() )
            break;

        //m_list.SetItemCount(0);
        //g_user.clear();
        //g_data.clear();
        //_init_var();

        auto pfn_timer = [](HWND hWnd, UINT m, UINT_PTR id, DWORD)
        {
            int* pInc = (int*)id;
            ( *pInc )++;
            wstr title(260);
            title.assign(L"歌单列表 - 正在从服务器获取歌单数据").append(*pInc, '.');
            SetWindowTextW(g_hWndDlg, title.c_str());
        };

        int inc = 0;
        SetTimer(g_hWndDlg, (UINT_PTR)&inc, 120, pfn_timer);
        _str listData;
        DWORD ret = newThread(ThreadProc_GetSongList, &listData, true);
        KillTimer(g_hWndDlg, (UINT_PTR)&inc);
        switch ( ret )
        {
        case -1: MessageBoxW(g_hWndDlg, L"向服务器请求数据失败, 请稍后再试", L"发送数据失败", 0); break;
        default:
            break;
        }

        LPBYTE buf = (LPBYTE)listData.c_str();
        int count = r2_read_int(buf);
        g_user.resize(count);
        m_ServerSongCount = 0;
        USER_ARR_DATA _user;
        for ( int i = 0; i < count; i++ )
        {
            LPCSTR username = r2_read_str(buf);
            LPCWSTR usernameW = g_buf.AddStringWFromAnsi(username);
            int size = r2_read_int(buf);
            _str parseData((LPCSTR)buf, size);
            buf += size;

            USER_ARR_DATA& userData = g_user[i];
            PUSER_ARR_DATA pUserData = &userData;
            if ( i == 0 )
            {
                // 当前用户, 使用另一个结构去存放结果
                pUserData = &_user;
                pUserData->arr.clear();
            }
            else
            {
                userData.arr.clear();
            }
            userData.username = usernameW;
            ListSong_ParseListData(*pUserData, i == 0, parseData);
            m_ServerSongCount += (int)pUserData->arr.size();


            if ( i == 0 )
            {
                for ( LIST_DATA_STRUCT& item2 : userData.arr )
                    item2.clrText = COLOR_TEXT_ERR;  // 本地的数据都当成是没有提交的, 后面加入数组会修改回来

                // 当前用户, 需要排查重复项
                int save = -1;
                for ( LIST_DATA_STRUCT& item1 : _user.arr )
                {
                    item1.clrText = COLOR_TEXT_OK;  // 这个数据是服务器来的, 直接改成一般颜色
                    bool isExist = false;
                    PLIST_DATA_STRUCT pItemLocal = 0;
                    for ( LIST_DATA_STRUCT& item2 : userData.arr )
                    {
                        // 循环和本地的对比, 如果本地的有这首歌曲, 那么 pItemLocal 就是本地歌曲的数据
                        if ( wcscmp(item2.pszSongName, item1.pszSongName) == 0 )
                        {
                            pItemLocal = &item2;
                            break;
                        }
                    }

                    if ( !pItemLocal )
                    {
                        // 本地数组没有这首歌, 直接加入数组
                        userData.arr.push_back(item1);
                        continue;
                    }

                    // 原来的列表有这首歌曲, 判断一下其他是不是完全一样, 如果完全一样就不加入
                    // 有一个不一样就信息框循环保留服务器还是本地

                    // 相等, 跳出循环, 不加入
                    if ( item1.isEqual(*pItemLocal) )
                    {
                        pItemLocal->clrText = COLOR_TEXT_OK;
                        continue;
                    }

                    // 走到这里就是两个项目有不同的地方, 信息框询问保留服务器的还是本地的
                    if ( save == -1 )
                    {
                        save = MessageBoxW(g_hWndDlg,
                                           L"服务器数据与本地数据有不同, 保留本地还是服务器?\r\n\r\n"
                                           L"是 = 保留服务器的数据\r\n"
                                           L"否 = 保留本地的数据\r\n"
                                           , L"保留那一个数据", MB_ICONQUESTION | MB_YESNO);
                    }

                    // 保留本地的数据, 不需要加入到数组里
                    // 保留服务器才加入数组, 本地的话已经在数组里了
                    if ( save == IDYES )
                    {
                        memcpy(pItemLocal, &item1, sizeof(item1));
                    }

                }
            }
        }
        ListSong_GetListData(true);
        SendMessageW(hWnd, WM_APP, 121007124, 20752843);
        break;
    }
    case ID_MENU_LOGIN:
    {
        g_token.clear();
        g_argData->pfnLoadLogin(1, 1);
        if ( g_userName && g_userName[0] )
        {
            if ( !g_token.empty() )
                MessageBoxW(g_hWndDlg, L"登录成功", L"登录成功", 0);
        }
        break;
    }
    case ID_MENU_ADD:       // 添加项目
    case ID_MENU_DEL:       // 删除项目
    case ID_MENU_CLEAR:     // 清除项目
    case ID_MENU_ADDMY:     // 将当前歌曲添加到自己的歌单中
    case ID_MENU_ADDMYALL:  // 将当前歌单添加到自己的歌单中
        menu_command(id);
        break;
    case ID_MENU_ZANZHU:
        ShellExecuteW(0, L"open", L"https://4cd.cc/play.html", L"", L"", 1);
        break;
    case ID_MENU_INJECT:
    {
        HWND hWndR2 = (HWND)g_argData->pfnLoadLogin(4, 0);
        if ( !hWndR2 )
        {
            MessageBoxW(0, L"找不到游戏窗口", L"请先打开游戏, 等游戏不卡顿后再点击启动", MB_ICONMASK);
            break;
        }
        DWORD pid;
        GetWindowThreadProcessId(hWndR2, &pid);
        if ( !hWndR2 )
        {
            tstr_MessageBox(0, L"获取游戏信息失败", MB_ICONMASK, L"获取游戏进程ID失败, 当前获取到的游戏窗口=%d", hWndR2);
            break;
        }

        HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, false, pid);
        if ( !hProc )
        {
            tstr_MessageBox(0, L"打开游戏进程失败", MB_ICONMASK, L"打开游戏进程失败, 进程ID=%d", pid);
            break;
        }
        CDLLInject dll;
        wstr dllFile(260);
        dllFile.assign(_str::A2W(g_argData->pszR2Path)).append(L"R2Inject.dll");
        HMODULE hModule = dll.InjectDll(hProc, dllFile.c_str(), "", 0, 0, 0);
        if ( !hModule )
        {
            tstr_MessageBox(0, L"启用失败", MB_ICONMASK, L"注入进程失败, 进程ID=%d", pid);
        }
        CloseHandle(hProc);
        break;
    }
    case ID_MENU_IMPORTFILE:
        MessageBoxW(0, L"暂未实现", L"等后续更新", 0);
        break;
    case ID_MENU_OPENLOGIN:
        g_argData->pfnLoadLogin(3, 1);
        break;
    case ID_MENU_OPENR2HOCK:
        g_argData->pfnLoadLogin(3, 2);
        break;
    case ID_MENU_SETTING:
        g_argData->pfnLoadLogin(3, 0);
        break;
    default:
        return false;
    }

    return true;
}

// 生成提交到服务器的数据
void UpdateBtnClick()
{
    std::vector<LIST_DATA_STRUCT>& arr = g_user[0].arr;
    bool isUpdate = false;
    for ( LIST_DATA_STRUCT& item : arr )
    {
        if ( item.clrText == COLOR_TEXT_ERR )
        {
            isUpdate = true;
            break;
        }
    }

    if ( arr.empty() )
    {
        int n = MessageBoxW(g_hWndDlg, L"当前数据为空, 是否要提交到服务器?\r\n"
                            L"提交后将服务器的数据将会被清空, 无法恢复\r\n"
                            L"\r\n是否要清空?\r\n"
                            , L"是否清除服务器数据?", MB_ICONQUESTION | MB_YESNO);
        if ( n == IDNO )
            return;
    }
    else if ( !isUpdate )
    {
        MessageBoxW(g_hWndDlg, L"列表数据没有变动, 不需要提交", L"没有数据需要更新", MB_ICONINFORMATION);
        return;
    }
    int _index = 0, _index2 = 0;
    wstr _dbgText(260);
    int addCount = 0;

    _str listData;
    if ( !ListSong_GetListData(true, &listData) )
    {
        _dbgText.assign(L"获取项目数据失败, 请把重现方式发送给福仔, 让福仔修复");
        MessageBoxW(g_hWndDlg, _dbgText.c_str(), L"获取数据失败", 0);
        return;
    }
    int size = (int)listData.size();
    if ( size == 0 )
    {
        return;
    }
    _str buf(100 + size);
    r2_write_int(buf, SERVER_UPDATELIST);
    r2_write_str(buf, g_token.c_str());
    r2_write_int(buf, size);
    r2_write_data(buf, listData.c_str(), size);

    DWORD ret = newThread(ThreadProc_UpdateList, &buf, true);
    
    switch ( ret )
    {
    case -1: MessageBoxW(g_hWndDlg, L"发送数据到服务器失败", L"发送数据失败", 0); break;
    case -2: MessageBoxW(g_hWndDlg, L"提交失败, 请检查用户名或者密码是否正确", L"提交数据失败", 0); g_token.clear(); break;
    case -3: MessageBoxW(g_hWndDlg, L"提交失败, 登录状态已失效, 请重新登录", L"登录状态已失效", 0); g_token.clear(); break;
    case -10: MessageBoxW(g_hWndDlg, L"登录状态失效, 请重新登录", L"请重新登录", 0); g_token.clear(); break;
    case -11: MessageBoxW(g_hWndDlg, L"未注册的账户", L"未注册的账户", 0); g_token.clear(); break;
    case -12: MessageBoxW(g_hWndDlg, L"提交了空的数据", L"提交了空的数据", 0); break;
    default:
        break;
    }
    if ( ret == 121007124 )
    {
        for ( LIST_DATA_STRUCT& item : arr )
        {
            // 循环把所有项目的颜色改一下
            item.clrText = COLOR_TEXT_OK;
        }
        ListSong_GetListData(true);
        m_list.InvalidateRect();
    }
    else
    {
        MessageBoxW(g_hWndDlg, L"更新失败, 请稍后再试", L"更新失败", 0);
    }
}

void _init_var()
{
    m_editData.iItem = -1;
    m_editData.iSubItem = -1;
    m_index = -1;
}
bool edit_command(int code)
{
    if ( code != EN_CHANGE )
        return false;
    btn_find();

    return true;
}
void btn_find()
{
    wstr findText = m_edit.GetText();
    if ( findText.empty() )
    {
        reLoad_show_list(g_data);
        m_list.SetItemCount((int)g_data.size());
        m_list.InvalidateRect();
        return;
    }

    LPWSTR find = findText.data();
    while ( *find )
    {
        wchar_t& ch = *find++;
        switch ( ch )
        {
        case L'：': ch = ':'; break;
        case L'，': ch = ','; break;
        case L'》': ch = '>'; break;
        case L'《': ch = '<'; break;
        default: break;
        }
    }
    find = findText.data();
    std::vector<std::vector<LPCWSTR>> arrFind;
    arrFind.reserve(30);
    while ( find && *find )
    {
        LPWSTR pos = wcschr(find, ',');
        if ( !pos )
            pos = wcschr(find, '|');
        while ( pos && (*pos == ' ' || *pos == ',' || *pos == '|' ) )
                *pos++ = 0;
        LPWSTR pEnd = find + wcslen(find);
        while ( pEnd )
        {
            --pEnd;
            if ( *pEnd == ' ' )
                *pEnd = 0;
            else
                break;
        }
        arrFind.resize(arrFind.size() + 1);
        std::vector<LPCWSTR>& arr2 = arrFind.back();

        while ( find && *find )
        {
            LPWSTR pos1 = wcsstr(find, L"&&");
            if ( !pos1 )
                pos1 = wcschr(find, '|');
            while ( pos1 && ( *pos1 == '&' || *pos1 == ' ' || *pos1 == '|' ) )
                *pos1++ = 0;
            pEnd = find + wcslen(find);
            while ( pEnd )
            {
                --pEnd;
                if ( *pEnd == ' ' )
                    *pEnd = 0;
                else
                    break;
            }
            arr2.push_back(find);
            find = pos1;
        }

        find = pos;
    }

    std::vector<PLIST_DATA_STRUCT> arr;
    reLoad_show_list(arr);
    g_data.clear();

    for ( PLIST_DATA_STRUCT pItem : arr )
    {
        bool isOk = false;
        for ( std::vector<LPCWSTR>& arr2 : arrFind )
        {
            for (LPCWSTR s : arr2)
            {
                // 循环把每个关键字都查找一遍
                // 这里先搜索显示的歌名, 没有就搜索备注
                // 走到这里的就是并且的逻辑, 所有条件都得满足
                // 数组的每个成员都是一个条件, 数组循环匹配
                // 只要有一个假, 那就匹配失败, 数组执行完后还是真, 那就是匹配成功
                // 如果只有一个成员, 那数组只循环一次.... 多个和一个都可以统一处理
                isOk = find_text(pItem->pszShowName, s, pItem);
                if ( !isOk )
                    isOk = find_text(pItem->pszRemark, s, pItem);
                if ( !isOk )
                    break;
            }
            if ( isOk )
                break;
        }
        
        if ( isOk )
            g_data.push_back(pItem);
    }
    int count = (int)g_data.size();
    m_list.SetItemCount(count);
    m_list.InvalidateRect();
}

bool find_text(LPCWSTR sourceStr, LPCWSTR subStr, PLIST_DATA_STRUCT pItem)
{
    const wchar_t& ch = *subStr;
    size_t srcLen = wcslen(sourceStr);
    size_t subLen = wcslen(subStr);

    switch ( ch )
    {
    case L'^':   // xx开头的文本
    {
        ++subStr; --subLen;
        return _wcsnicmp(sourceStr, subStr, subLen) == 0;
    }
    case L'$':   // xx结尾的文本
    {
        ++subStr; --subLen;
        LPCWSTR pCmpStart = sourceStr + srcLen - subLen;
        return _wcsicmp(pCmpStart, subStr) == 0;
    }
    case L'*':   // 按星搜索
    {
        ++subStr; --subLen;
        return find_int(subStr, pItem->nLevel, true);
    }
    case L'#':   // 按连击数搜索
    {
        ++subStr; --subLen;
        return find_int(subStr, pItem->nCombo);
    }
    case L'%':   // 搜索歌曲时长
    {
        ++subStr; --subLen;
        return find_int(subStr, pItem->nTimer);
    }
    case L'!':
    {
        ++subStr; --subLen;
        return find_int(subStr, pItem->nBpm);
    }
    default:
        // 默认搜索, 搜索整个文本
        return wstr::find(sourceStr, subStr, 0, false) != wstr::npos;
    }
    return false;
}

inline int _float_2_int(LPCWSTR findStr, bool isLevel)
{
    LPCWSTR pos = wcschr(findStr, '.');
    if ( !isLevel || pos == 0 )
        return wstr::stoi(findStr);
    
    double vl = wstr::stod(findStr) * 2;
    return (int)(LONG64)vl;
}

bool find_int(LPCWSTR findStr, int findInt, bool isLevel)
{
    size_t subLen = wcslen(findStr);
    const wchar_t& ch1 = *findStr;
    if ( ch1 == '>' || ch1 == '<' )
    {
        ++findStr; --subLen; // 是搜索大于或者小于的数值
        const wchar_t& ch2 = *findStr;
        if ( ch2 == '=' )
        {
            // 走到这里就是要搜索大于等于或者小于等于某个数值
            ++findStr; --subLen; // >= 或者 <=
            int vl = _float_2_int(findStr, isLevel);
            if ( ch1 == '>' )
                return findInt >= vl;
            return findInt <= vl;
        }

        // 走到这里就是要搜索大于或者小于某个数值
        int vl = _float_2_int(findStr, isLevel);
        if ( ch1 == '>' )
            return findInt > vl;
        return findInt < vl;
    }
    if ( ch1 == '=' )
        findStr++;
    // 搜索等于这个数值
    return _float_2_int(findStr, isLevel) == findInt;
}

LRESULT CALLBACK Window_ListSongProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    WNDPROC oldProc = (WNDPROC)GetPropW(hWnd, L"proc");

    switch ( message )
    {
    case WM_RBUTTONDOWN:
    {
        const int index = m_list_song.GetIndedx();
        bool isPlay = g_ini->read_int(INI_APPNAME, L"isAutoPlay", 1);
        m_menu_song.SetCheck(ID_MENU_AUTOPLAY, isPlay);
        m_menu_song.pop();
        const int ss = LV_VIEW_TILE;
        break;
    }
    default:
        return CallWindowProcW(oldProc, hWnd, message, wParam, lParam);
    }
    return 0;
}


void Window_SelSongCallback(int index)
{
    m_edit.SetText(L"");    // 清空搜索框, 然后查找
    int i = 0;
    PLIST_DATA_STRUCT pItem = 0;
    for ( PLIST_DATA_STRUCT& item : g_data )
    {
        if ( item->nIndex == index )
        {
            pItem = item;
            break;
        }
        i++;
    }
    if ( !pItem )
    {
        tstr_MessageBox(0, L"可能出错了", 0, L"应该是出错了, 音速内选的歌曲这个工具没找到, Index = %d", index);
        return; // 要是走到这里就是有问题
    }

    m_list.SetIndex(i, true);

}

