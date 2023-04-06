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

#define TEXT_STATIC L"������ע, д�ؼ��ַ�������, һ��дһЩ�����׸��˵��, �����ܲ���A, Զ������, �ܲ��ܼ��ٵ�..."


struct LIST_EDIT_DATA
{
    int     iItem;
    int     iSubItem;
    HWND    hEdit;
    RECT    rcItem;
};

#define LISTBOX_SONGLIST_WIDTH 400  // �����б��Ŀ��

#define ID_BTN_UPDATE       1000
#define ID_BTN_FIND         1001
#define ID_EDIT_FIND        1002

#define ID_MENU_GETDATA     1999    // �ӷ�������ȡ����
#define ID_MENU_EDIT        1998    // �༭��ǰ��Ŀ
#define ID_MENU_AUTOPLAY    1997    // ѡ���Զ����Ÿ���
#define ID_MENU_STOP        1996    // ֹͣ����
#define ID_MENU_SORT_LEVEL  1995    // ���Ǽ�����
#define ID_MENU_SORT_NAME   1994    // ����������
#define ID_MENU_SORT_ARTIST 1993    // ����������
#define ID_MENU_UPDATE      1992    // �ύ���ݵ�������
#define ID_MENU_LOGIN       1991    // ��¼, ��Ҫ�Զ���¼
#define ID_MENU_PLAY        1990    // ���Ÿ���

#define ID_MENU_ADD         2000    // �����Ŀ
#define ID_MENU_DEL         2001    // ɾ����Ŀ
#define ID_MENU_CLEAR       2002    // �����Ŀ
#define ID_MENU_ADDMY       2003    // ����ǰ������ӵ��Լ��ĸ赥��
#define ID_MENU_ADDMYALL    2004    // ����ǰ�赥��ӵ��Լ��ĸ赥��
#define ID_MENU_SORT_LV     2005    // ����, ���ȼ�����
#define ID_MENU_SORT_COMBO  2006    // ����, ����������
#define ID_MENU_SORT_TIMER  2007    // ����, ��ʱ������
#define ID_MENU_SORT_SONG   2008    // ����, ����������
#define ID_MENU_SORT_SINGER 2009    // ����, ����������
#define ID_MENU_SORT_BPM    2010    // ����, ��BPM����
#define ID_MENU_HELP        2011    // ����


#define ID_MENU_IMPORTFILE  2500    // ����赥
#define ID_MENU_ZANZHU      2501    // ����
#define ID_MENU_OPENLOGIN   2502    // �򿪵�¼��
#define ID_MENU_OPENR2HOCK  2503    // ������
#define ID_MENU_SETTING     2504    // ����
#define ID_MENU_INJECT      2505    // ����




static CListView m_list;
static CListBox m_list_song;       // �����б�
static CCombobox m_combobox;
static CMyEdit m_edit;              // ���Ҹ����༭��
static CMyEdit m_editRemark;        // ��ע�ı��༭��
static CMyStatic m_songImage;       // ������Ӧ��ͼƬ
static CMyEdit m_editInsert;        // �༭�ı༭��
static HFONT hFont;
static LIST_EDIT_DATA m_editData;   // ���ڱ༭������
static WNDPROC m_oldProcList;       // ����ԭ������Ϣ�ص�
static HWND m_hStatic;              // ��ע�༭������ı�ǩ���
static bool m_isLocalData;          // ��ǰ�Ƿ��Ǽ��ر�������
static int m_index;                 // ��ǰѡ���������
static CMenu m_menu;                // �˵�
static CMenu m_menuWnd;             // ���ڵĲ˵�, ����Ǻü��������˵���ϵ�
static CMenu m_menuAll;             // ���и����б�˵�
static CMenu m_menu_song;           // �����б�Ĳ˵�
static bool m_isEdit;               // �Ƿ������޸�, �����Լ����˻�������༭
static Ctips m_tips;
static int m_indexListSong;         // �������б��ѡ����, ����ʾ��ʱ����Ҫ��λ�������Ŀ
static int m_rClickItem;            // �Ҽ���������˵�ʱ����Ŀ����
static int m_rClickSubItem;         // �Ҽ���������˵�ʱ��������
static POINT m_rClickPoint;         // �Ҽ�����ʱ����λ��
static int m_ServerSongCount;       // ���������صĸ�������
static bool m_isR2SongList;         // ��Ͽ�ѡ��ı��ʱ��ֵ, ���һ��ѡ�������������
static bool m_isDblClkEditEnd;      // �����༭�Ƿ���˫���б�򴥷���
static Gdiplus::Bitmap* m_image;    // ��ʾ��ͼƬ

INT_PTR CALLBACK Window_OnNotify(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK Window_DefWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK Window_ListViewProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK Window_ListViewEditProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK Window_SongImageProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK Window_EditRemarkProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK Window_OnCommand(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK Window_ListSongProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
void Window_SelSongCallback(int index);


// ����������б�, ��������ķ�ʽ
void ListSong_SortList(int mode);

// ����赥�б�, ��������ʽ
void ListView_SortList(int mode);

// ���еĸ����ڷ��ͱ༭������֪ͨ
void _send_notify_edit_end(bool isChange);

void UpdateBtnClick();
void _init_var();
bool edit_command(int code);
void btn_find();
// ���¸�����Ͽ�ѡ�����������
int reLoad_show_list(std::vector<PLIST_DATA_STRUCT>& arr, LPCWSTR* username = 0);

const LPCWSTR editInsertTips =
L"ʹ�����·������Ը��������������\r\n"
L"ƥ��ͷʹ�� ^xxx, �������� \"r\" ��ͷ�ĸ���, ��ô��ʹ�� \"^r\"\r\n"
L"ƥ��βʹ�� $xxx, �������� \"r\" ��β�ĸ���, ��ô��ʹ�� \"$r\"\r\n"
L"���������Ǽ�ʹ�� *xx, �������� 8 ����, ��ô��ʹ�� *16\r\n"
L"    �Ǽ���Ҫ����2, ���� 3.5��, ������Ӧ��д 7, ��Ϊ3.5*2=7\r\n"
L"    ֧�ִ��ȽϷ���, ���� *>, *<, *>=, *<=, �ֱ��Ǵ���, С��, ���ڵ���, С�ڵ���\r\n"
L"����������ʽ�ȴ���������\r\n"
;




// ������Ͽ�ѡ���������ʾ���б�
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
    memcpy(dlg.caption, L"���߸赥", 10);

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

// �����Ƿ����·������ڴ�
inline bool _set_list_buf_text(LPCWSTR* ppszText, LPCWSTR text)
{
    LPWSTR pstr = (LPWSTR)*ppszText;
    size_t len1 = wcslen(pstr);
    size_t len2 = wcslen(text);
    if ( len1 >= len2 )
    {
        // �����, ֱ�Ӵ�Ž�ȥ
        memcpy(pstr, text, len2 * sizeof(wchar_t) + sizeof(wchar_t));
        return false;
    }
    else
    {
        // �������, �����µĻ��������
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
    if ( wcscmp(pstr, text) == 0 )  // �ı�һ��, ���޸�
        return false;

    wstr tmpStr;
    switch ( iSubItem )
    {
    case LIST_COMBO:
    {
        // ������, ��Ҫͬʱ�޸���������Ա
        data->nCombo = wstr::stoi(text);
        break;
    }
    case LIST_TIME:
    {
        // ʱ��, ��Ҫ�޸ı༭������, ����� ':' , �Ǿͻ�ȡ�ֺ��������ߵ���ֵ, ���û�зֺ�, �ǾͰ���ֵת����xx:xx
        LPCWSTR pos = wcschr(text, ':');
        if ( !pos )
            pos = wcschr(text, '��');
        
        if ( pos )
        {
            // �зֺ�, ȡ�������ߵ���ֵ
            tmpStr = text;
            tmpStr.replace('��', ':');
            data->nTimer = str2time(tmpStr.c_str());
            text = tmpStr.c_str();
        }
        else
        {
            // û�зֺ�, ���ı�ת����ֵ��ת���ɷ�
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
        // Ϊ��ʡ��, ֱ�����·���, Ϊ��ʡ�ڴ�Ļ�, ����Ͳ�����ô��
        if ( isAlloc )
        {

        }
        else
        {

        }
    }
    ListSong_GetListData(true);     // ���ݱ��޸���, ÿ�ζ�д������
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
    case LVN_BEGINLABELEDIT:    // ��ʼ�༭
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
        // ֻ���������һ�б��޸�
        if ( !pItem->isUser() || iSubItem != LIST_SONGNAME)
        {
            SetWindowLongPtrW(hWnd, DWLP_MSGRESULT, 1); // �����Լ������ݲ���ʾ�༭
            m_editInsert.hide();
            m_list_song.hide();
            return true;
        }


        // ʣ�µľ��ǿ����޸�
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
            // ���Ŀǰ��Զ�����߽���
            const LPCWSTR tmpTips =
                L"ʱ��֧������xxx��, ����123, �ڲ����Զ�ת���� 02:03\r\n"
                L"Ҳ֧������ xx:xx, ���� 02:03, ��Ӣ�ĵ�ð�Ŷ�����\r\n"
                ;
            m_tips.SetTipsText(m_editInsert, tmpTips);
        }
        m_editInsert.SetFocus();
        SetWindowLongPtrW(hWnd, DWLP_MSGRESULT, 1); // ���ز������޸�, �Լ�����༭��
        return true;
    }
    case LVN_ENDLABELEDIT:      // �����༭
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
            // ԭ����ѡ����, ���ҵ�ǰѡ�еĺ�֮ǰѡ�еĲ�ͬ, ��Ҫ����һ�±�ע�ı�
            wstr text = m_editRemark.GetText();
            // ���±�ע�ı�, ��������ڲ����ж��Ƿ���Ҫ����
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
            // ѡ�е������һ����Ŀ
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
            m_menu.SetTitle(ID_MENU_GETDATA, L"�ӷ�������ȡ����");
            m_menu.SetTitle(ID_MENU_UPDATE, L"�ύ���ݵ�������");
        }
        else
        {
            m_menu.SetTitle(ID_MENU_GETDATA, L"�ӷ�������ȡ����\t��Ҫ��¼");
            m_menu.SetTitle(ID_MENU_UPDATE, L"�ύ���ݵ�������\t��Ҫ��¼");
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
            // ����ʧ�ܾ�ע�����, Ȼ���ٴε���
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
                if ( wcscmp(str, L"�ղ�") != 0 )
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
        //TODO �������������Ҫ����ִ��
        LPNMLISTVIEW pnmv = (LPNMLISTVIEW)lParam;
        // �дӴ�С��־�͸ĳɴ�С����
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
            LPCWSTR cbx_first_name = L"ȫ���û��赥";
            if ( lParam == 20752843 )
            {
                // �Ѿ��ӷ�������ȡ�������, 
                // �ӷ�������ȡ����
                m_isLocalData = false;
                szTitle.assign(L"�赥�б� - �ӷ�������ȡ�� ").append(g_user.size()).append(L"���赥, һ��").append(count).append(L" �׸�");
            }
            else
            {
                // �ӱ��ؼ�������
                ListSong_LoadDiskData();
                count = (DWORD)g_data.size();
                count = (DWORD)g_r2SongList.size();
                szTitle.assign(L"�赥�б� - �ӱ��ؼ����� ").append(g_user.size()).append(L"���赥, һ��").append(count).append(L" �׸�");
                cbx_first_name = L"���ظ赥";
                m_isLocalData = true;
            }
            
            SetWindowTextW(hWnd, szTitle.c_str());
            m_combobox.DelAll();
            //m_combobox.AddString(cbx_first_name);
            int index = 0;

            // �������û��ĸ赥��ŵ���Ͽ���, ��ȡ�赥�б��ʱ����Ҫ���Լ��ĸ赥��ŵ�����ĵ�һ����Ա��
            //for ( USER_ARR_DATA& userData : g_user )
            //{
            //    //m_combobox.AddString(userData.username, (LPARAM)&userData);
            //    index = 1;
            //}
            m_combobox.AddString(L"��������", 123);

            m_combobox.SetSel(index);
            UpdateListData(false);
            break;
        }
        return (INT_PTR)false;
    }
    case WM_APP+1:  // ѡ��ص�, ���ٽ���ѡ�ø��������ⲿ����֪ͨ, wParam = ����ID, lParam = 0
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
        m_list.m_hWnd = CreateWindowExW(styleEx, WC_LISTVIEWW, L"�赥�б�", style, 0, 0, 0, 0, hWnd, (HMENU)(LONG_PTR)0, g_hInst, 0);
        m_oldProcList = (WNDPROC)SetWindowLongPtrW(m_list.m_hWnd, GWLP_WNDPROC, (LONG_PTR)Window_ListViewProc);

        m_list.SetListStyle(styleList);

        m_list.InsertColumn(LIST_LEVEL   , L"�ȼ�"    , colWidth[LIST_LEVEL   ]);
        m_list.InsertColumn(LIST_COMBO   , L"����"    , colWidth[LIST_COMBO   ]);
        m_list.InsertColumn(LIST_TIME    , L"����ʱ��" , colWidth[LIST_TIME    ]);
        m_list.InsertColumn(LIST_SONGNAME, L"���� - ����", colWidth[LIST_SONGNAME]);
        //m_list.InsertColumn(LIST_REMARKS , L"��ע"    , colWidth[LIST_REMARKS ]);
        m_list.InsertColumn(LIST_BPM     , L"BPM"    , colWidth[LIST_BPM ]);
        m_list.InsertColumn(LIST_USERNAME, L"�赥����" , colWidth[LIST_USERNAME]);
        

        //! ��ʾ�����б���б�
        style = WS_BORDER | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE | WS_POPUP | WS_VSCROLL |
            LBS_NODATA | LBS_OWNERDRAWFIXED | LBS_NOTIFY;
        styleList = LVS_EX_SUBITEMIMAGES | LVS_EX_INFOTIP;
        m_list_song.m_hWnd = CreateWindowExW(0, WC_LISTBOXW, L"�����б�", style, 0, 0, 0, 0, m_list.m_hWnd, 0, g_hInst, 0);
        //m_list_song.SetListStyle(styleList);

        m_list_song.hide();
        WNDPROC oldProc = (WNDPROC)SetWindowLongPtrW(m_list_song.m_hWnd, GWLP_WNDPROC, (LONG_PTR)Window_ListSongProc);
        m_list_song.SetPropW(L"proc", oldProc);

        style = WS_TABSTOP | WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | WS_VSCROLL | CBS_DROPDOWNLIST | CBS_AUTOHSCROLL | CBS_HASSTRINGS;
        styleEx = WS_EX_CLIENTEDGE;
        m_combobox.m_hWnd = CreateWindowExW(styleEx, WC_COMBOBOXW, L"�û��б�", style, 66, 8, 150, 200, hWnd, 0, 0, 0);
        SendMessageW(m_combobox.m_hWnd, WM_SETFONT, (WPARAM)hFont, 0);
        m_tips.Insert(m_combobox.m_hWnd, L"��һ���������û�, �ڶ������Լ����˻�\r\n���Ը�����Ҫ��ʾĳ���˻���ȫ���ĸ赥", 0, 0);


        GetWindowRect(m_combobox.m_hWnd, &rcCbb);
        cbbHeight = rcCbb.bottom - rcCbb.top;
        cbbWidth = rcCbb.right - rcCbb.left;
        rcCbb.left = 66;
        rcCbb.top = 8;
        rcCbb.bottom = rcCbb.top + cbbHeight;
        rcCbb.right = rcCbb.left + cbbWidth;

        style = WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | SS_RIGHT | SS_CENTERIMAGE;
        HWND hStatic = CreateWindowExW(0, WC_STATICW, L"�û��赥", style, 8, 8, 50, cbbHeight, hWnd, 0, 0, 0);
        SendMessageW(hStatic, WM_SETFONT, (WPARAM)hFont, 0);

        hStatic = CreateWindowExW(0, WC_STATICW, L"��������", style, 224, 8, 50, cbbHeight, hWnd, 0, 0, 0);
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
            L"��������ؼ�����ʹ�ö��Ż���||�ָ�, ���� \"����,����||AC\"\r\n"
            L"    ^ģʽ, ƥ��ͷ, �������� \"^re\",  ��ô�ͻ��г�����\"re\"��ͷ�ĸ���/����\r\n"
            L"    $ģʽ, ƥ��β, �������� \"$re\",  ��ô�ͻ��г�����\"re\"��β�ĸ���/����\r\n"
            L"    #ģʽ, ����������, �������� \"#1104\" �ͻ��г�������1104�ĸ���\r\n"
            L"    *ģʽ, �����ȼ�, �������� \"*9.5\" �ͻ��г�����9.5�ǵĸ���\r\n"
            L"    !ģʽ, ����BPM, �������� \"!180\" �ͻ��г�����BPM����180�ĸ���\r\n"
            L"    %ģʽ, ����ʱ��, �������� \"*120\" �ͻ��г�����120��ĸ���\r\n"
            L"    &&��������, ��Ҫ�������ͬʱ����, ��ο����������\r\n"
            L"    ����/�Ǽ�/ʱ��/BPM ֧�ֱȽϷ���, >, <, >=, <=, �ֱ��Ǵ���, С��, ���ڵ���, С�ڵ���\r\n"
            L"        ����, \"#>1000\" ���г�������������1000�ĸ���\r\n"
            L"        ����, \"!<130\" ���г�����BPMС��130�ĸ���\r\n"
            L"        ����, \"*>=8.5\" ���г�����8.5�ʹ���8.5�ǵĸ���\r\n"
            L"        ����, \"*>=8.5 && !>200\" ���г�8�����ϲ���BPM����200�ĸ���, ������8��\r\n"
            L"    ����֧�ֶ���������, && �ǲ��ҵ�����, ����\",\"����\"||\"�ǻ��ߵ�����, ����:\r\n"
            L"        ����, \"%>=120 && %<=150 && #>999 || !>220\"\r\n"
            L"        �������ģʽ��� ���ڵ���120��, ����С�ڵ���150��ĸ���\r\n"
            L"        ���� ��������999, �ֻ��� BPM����220�ĸ���\r\n"
            L"        ���ģʽ���г�ʱ���� 120-150��֮����������999�ĸ���\r\n"
            L"        �����г�BPM����220�ĸ���\r\n"
            L"    ����������ʽ�ȴ���������\r\n\r\n"
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
        SetWindowTextW(hWnd, L"�赥�б� - ���ڻ�ȡ�赥�б�");
        SetTimer(hWnd, 10086, 150, 0);

        m_menuWnd.create(hWnd, CreateMenu());
        HMENU hMenuPop = CreatePopupMenu();
        AppendMenuW(hMenuPop, MF_STRING, ID_MENU_INJECT, L"�����ⲿѡ�蹦��");
        AppendMenuW(hMenuPop, MF_STRING, ID_MENU_IMPORTFILE, L"�����ⲿ�赥");
        AppendMenuW(hMenuPop, MF_STRING, ID_MENU_OPENLOGIN, L"�򿪵�¼��");
        AppendMenuW(hMenuPop, MF_STRING, ID_MENU_OPENR2HOCK, L"������");
        AppendMenuW(hMenuPop, MF_STRING, ID_MENU_SETTING, L"����");
        m_menuWnd.add(hMenuPop, L"�ļ�");

        hMenuPop = CreatePopupMenu();
        AppendMenuW(hMenuPop, MF_STRING, ID_MENU_ZANZHU, L"��������");
        AppendMenuW(hMenuPop, MF_STRING, ID_MENU_HELP, L"����");
        m_menuWnd.add(hMenuPop, L"����");
        SetMenu(hWnd, m_menuWnd);

        m_menu_song.create(hWnd);
        m_menu_song.add(ID_MENU_AUTOPLAY, L"ѡ���Զ�����");
        m_menu_song.add((UINT)0, 0, MF_SEPARATOR);
        m_menu_song.add(ID_MENU_SORT_LEVEL, L"���Ǽ�����");
        m_menu_song.add(ID_MENU_SORT_NAME, L"����������");
        m_menu_song.add(ID_MENU_SORT_ARTIST, L"����������");

        m_menu.create(hWnd);
        m_menu.add(ID_MENU_STOP, L"ֹͣ����");
        m_menu.add(ID_MENU_LOGIN, L"��¼");
        m_menu.add(ID_MENU_GETDATA, L"�ӷ�������ȡ����");
        m_menu.add(ID_MENU_UPDATE, L"�ύ���ݵ�������");
        m_menu.add(ID_MENU_EDIT, L"�༭��Ŀ(ѡ�к��һ�������ٵ��һ�οɿ��ٽ���༭)");

        m_menu.add((UINT)0, 0, MF_SEPARATOR);
        hMenuPop = CreatePopupMenu();
        AppendMenuW(hMenuPop, MF_STRING, ID_MENU_SORT_LV, L"���ȼ�����");
        AppendMenuW(hMenuPop, MF_STRING, ID_MENU_SORT_COMBO, L"����������");
        AppendMenuW(hMenuPop, MF_STRING, ID_MENU_SORT_TIMER, L"��ʱ������");
        AppendMenuW(hMenuPop, MF_STRING, ID_MENU_SORT_SONG, L"����������");
        AppendMenuW(hMenuPop, MF_STRING, ID_MENU_SORT_SINGER, L"����������");
        AppendMenuW(hMenuPop, MF_STRING, ID_MENU_SORT_BPM, L"��BPM����");
        m_menu.add(hMenuPop, L"����");
        
        m_menu.add((UINT)0, 0, MF_SEPARATOR);
        m_menu.add(ID_MENU_ADD, L"��Ӹ���");
        m_menu.add(ID_MENU_DEL, L"ɾ������");
        m_menu.add(ID_MENU_CLEAR, L"��ո���");
        m_menu.add(ID_MENU_ADDMY, L"��ѡ�и�����ӵ��Լ��ĸ赥��");
        m_menu.add(ID_MENU_ADDMYALL, L"����ǰ�赥��ӵ��Լ��ĸ赥��");
        m_menu.add((UINT)0, 0, MF_SEPARATOR);
        m_menu.add(ID_MENU_HELP, L"����");

        m_menuAll.create(hWnd);
        m_menuAll.add(ID_MENU_PLAY, L"����");
        m_menuAll.add(ID_MENU_STOP, L"ֹͣ����");
        m_menuAll.add((UINT)0, 0, MF_SEPARATOR);
        m_menuAll.add(ID_MENU_SORT_LV, L"���ȼ�����");
        m_menuAll.add(ID_MENU_SORT_COMBO, L"����������");
        m_menuAll.add(ID_MENU_SORT_TIMER, L"��ʱ������");
        m_menuAll.add(ID_MENU_SORT_SONG, L"����������");
        m_menuAll.add(ID_MENU_SORT_SINGER, L"����������");
        m_menuAll.add(ID_MENU_SORT_BPM, L"��BPM����");
        //m_menuAll.add((UINT)0, 0, MF_SEPARATOR);
        //m_menuAll.add(ID_MENU_ADDMY, L"��ӵ��Լ��ĸ赥��");
        m_menuAll.add((UINT)0, 0, MF_SEPARATOR);
        m_menuAll.add(ID_MENU_ZANZHU, L"��������");
        m_menuAll.add(ID_MENU_HELP, L"����");

        break;
    }
    case WM_TIMER:
    {
        szTitle.reserve(100);
        szTitle.assign(L"�赥�б� - ���ڻ�ȡ�赥�б�").append(++titleInc, '.');
        SetWindowTextW(hWnd, szTitle.c_str());
        if ( titleInc > 10 )
            titleInc = 0;
        break;
    }
    case WM_SIZE:
    {
        const int cxClient = LOWORD(lParam);
        const int cyClient = HIWORD(lParam);
        const int listTop = 200;    // ���ж���Ԥ������, ����������, ��ɾ�Ĳ�ʲô��
        const int listBottom = 0;   // ���еײ�Ԥ������, ������ʾ��ϸ��Ϣ��
        const int border = 8;

        int maxColWidth = 23;   // ���������, �����б���ֺ��������
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

        // �����������С�ߴ�
        pos->ptMinTrackSize.x = 600;    // ��С�϶���С
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
        // ��ʾ�����û�
        if ( username )
        {
            *username = L"�����û�";
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
        // ��ʾָ���û��ĸ赥
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

// ������Ͽ�ѡ���������ʾ���б�
void UpdateListData(bool isUpdateTitle)
{
    LPCWSTR username = 0;
    const int index = reLoad_show_list(g_data, &username);



    if ( !m_edit.GetText().empty() )
    {
        // ��ֵ, �л��󻹼�������
        btn_find();
        const int indexList = m_list.GetIndex();
        if ( indexList != -1 && indexList < (int)g_data.size() )
            m_editRemark.SetText(g_data[indexList]->pszRemark);
        m_index = -1;
        return;
    }

    int count = (int)g_data.size();

    // ��ʾ�������ٸ赥
    if ( isUpdateTitle )
    {
        wstr szTitle;
        username = m_isR2SongList ? L"��������" : username;
        szTitle.Format(L"�赥�б� - ���� [%s] �ĸ赥, һ���� %d �׸�", username, count);
        SetWindowTextW(g_hWndDlg, szTitle.c_str());
    }

    const int indexList = m_list.GetIndex();
    if ( indexList != -1 && indexList < (int)g_data.size() )
        m_editRemark.SetText(g_data[indexList]->pszRemark);
    m_index = -1;

    m_list.SetItemCount(count);
    m_list.InvalidateRect();
}

// ��ȡ����λ��
void Dlg_rcWindow(RECT* prc, LPCWSTR keyName)
{
    RECT& rc = *prc;
    const LPCWSTR def = L"0,0,600,480";
    wstr str = g_ini->read(INI_APPNAME, keyName, def);
    swscanf_s(str.c_str(), L"%d,%d,%d,%d", &rc.left, &rc.top, &rc.right, &rc.bottom);

    int width = 600;
    int height = 480;

    int cx = GetSystemMetrics(SM_CXSCREEN);                     // ��Ļ���
    int cy = GetSystemMetrics(SM_CYMAXIMIZED);                  // ��Ļ�߶� - ������
    int cxScreen = GetSystemMetrics(SM_CXVIRTUALSCREEN);        // ������Ļ����ܺ�
    int cyScreen = GetSystemMetrics(SM_CYVIRTUALSCREEN);        // ������Ļ�߶��ܺ�

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

    if ( height > cxScreen )      // ����߶ȴ�����Ļ�߶�, ������Ϊ��Ļ�߶�
        rc.top = cxScreen - height;
    if ( width > cxScreen )       // �����ȴ�����Ļ���, ������Ϊ��Ļ���
        rc.left = cxScreen - width;
    if ( rc.left < -width )       // ������λ��С�ڴ��ڿ�ȵĸ���, ��ʾ������߼ӿ�Ȳ�����ʾ����Ļ��
        rc.left = 0;
    if ( rc.top < -height )       // �������λ��С�ڴ��ڸ߶ȵĸ���, ��ʾ���ڶ��߼Ӹ߶Ȳ�����ʾ����Ļ��
        rc.top = 0;
    if ( rc.left > cxScreen )     // ������λ�ô���������Ļ�ܿ��, ���������λ�� = ��Ļ�ܿ�� - ���ڿ��
        rc.left = cxScreen - width;
    if ( rc.top > cyScreen )      // �������λ�ô���������Ļ�ܸ߶�, �����ö���λ�� = ��Ļ�ܸ߶� - ���ڸ߶�
        rc.top = cyScreen - height;

    rc.right = rc.left + width;
    rc.bottom = rc.top + height;
    return;

}


// ��ʾ�û��赥��
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
                case L'��': ch = ':'; break;
                case L'��': ch = ','; break;
                case L'��': ch = '>'; break;
                case L'��': ch = '<'; break;
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
            // ˫����ֻ�޸� ����, 
            if ( // ȫ��ƥ��ľ���û���޸�, ��Ҫ����, ֻҪ��һ����ͬ, �Ǿ��ߵ������޸�
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

            // ˫������Ҫ�����û��ĸ���, ����ʱ��, ��������, ���ݱ�ע�ı�����ע�޸�/���Ӹ��ֵ�����һЩ��Ϣ
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


            // �µı�ע�ı�
            wstr remark(520);
            remark = item.pszRemark;
            {
                // ������Ϣ: ���� - ����, �ȼ�, ����, ʱ��\r\n
                // �����ؼ���:
                wstr info = remark.substr(L"������Ϣ:", L"\r\n");
                if ( info.empty() )
                {
                    // û�л�����Ϣ, �ǾͰѻ�����Ϣ���뵽ǰ��
                    wstr str(100);
                    str.append(L"������Ϣ:").append(item.pszShowName);
                    str.append(L", *").append(item.nLevel);
                    str.append(L", #").append(item.nCombo);
                    str.append(L", %").append(item.nTimer);
                    str.append(L", !").append(item.nBpm);
                    str.append(L"\r\n");
                    remark.insert(0, str);
                }
                else
                {
                    // ԭ���и��ָ���, �Ǿ��޸�ð�ŵ����е�����
                    wstr str(100);
                    str.append(item.pszShowName);
                    str.append(L", *").append(item.nLevel);
                    str.append(L", #").append(item.nCombo);
                    str.append(L", %").append(item.nTimer);
                    remark.replace(info, str);
                }

                if ( remark.find(L"�����ؼ���:") == wstr::npos )
                {
                    // û�������ؼ���, ����һ��
                    if ( remark.back() != '\n' )
                        remark.append(L"\r\n");
                    remark.append(L"�����ؼ���:\r\n");
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
            // ԭ������ѡ��, �ǵ�������ѡ��, ���Ÿ���
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
                // �б��ʧȥ�˽���, ���ҵ�ǰ���㲻�Ǳ༭��
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
            // ѡ��
            HBRUSH hbrBack = CreateSolidBrush(RGB(0, 120, 215));
            FillRect(hdc, &rcItem, hbrBack);
            DeleteObject(hbrBack);
            SetTextColor(hdc, RGB(255, 255, 255));
        }
        else if ( __query(draw->itemState, ODS_HOTLIGHT) )
        {
            // �ȵ�
            HBRUSH hbrBack = CreateSolidBrush(COLOR_TEXT_BACK_ERR);
            FillRect(hdc, &rcItem, hbrBack);
            DeleteObject(hbrBack);
        }
        else
        {
            // ����״̬, ����ɫ����, �滭��ɫ����
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

// ���еĸ����ڷ��ͱ༭������֪ͨ
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

//! �����б༭����ʹ�õı༭����Ϣ�ص�
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
                text.assign(L"����pak�ļ���������, û�ҵ�ͼƬ\r\nͼƬ·��Ϊ: \r\n");// .append(_str::A2W(g_argData->pszR2Path));
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
// ��ע�༭����Ϣ�ص�, ��Ҫ�ж��Ƿ����������ַ�, ��ʧȥ�����ʱ����Ҫ�ж������Ƿ��޸�
LRESULT CALLBACK Window_EditRemarkProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    WNDPROC oldProc = (WNDPROC)GetPropW(hWnd, L"proc");
    switch ( message )
    {
    case WM_KILLFOCUS:
    {
        // ��ע�ı��༭��ʧȥ����, ��Ҫ�ж��Ƿ���Ҫ��������
        wstr text = m_editRemark.GetText();
        // ���±�ע�ı�, ��������ڲ����ж��Ƿ���Ҫ����
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
            // ������༭, ֻ�������ַ�����
            wstr::dbg(L"����ļ�: %02X\n", wParam);
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
        // �����ٸ赥
        g_data.clear();
        for ( LIST_DATA_STRUCT& item : g_r2SongList )
        {
            g_data.push_back(&item);
        }
    }
    else if ( userData == 0 )    // ��ǰ��ʾ�������е���Ŀ, g_data��ȫ���ĳ�Ա����������
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
        // ��ʾָ���û��ĸ赥
        std::vector<LIST_DATA_STRUCT>& arr = userData->arr;
        g_data.resize(arr.size());
        int i = 0;
        for ( LIST_DATA_STRUCT& item : arr )
            g_data[i++] = &item;
    }

}

// ����1��ʾ������ͬ, ����ӵ�����, ֻ�޸�
// ����0��ʾû���κεط���ͬ, ��Ҫ��ӵ�����
// ����2��ʾ���еط�����ͬ, ���������鲻�޸�
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


// ����Ŀ��ӵ�������, ���ж���������û�������Ŀ, �оͲ����, �����Ƿ������
inline bool _set_item_data(std::vector<LIST_DATA_STRUCT>& userArr, LIST_DATA_STRUCT& _item)
{
    int change = 0; // 2=����, 1=�޸�, 0=���
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
        // �ظ���Ŀ, ����ӵ�������
        return false;
    }

    
    if ( change == 1 && pItem )
    {
        // �޸���Ŀ, ���������ø�
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
    case ID_MENU_ADD:       // �����Ŀ
    {
        LIST_DATA_STRUCT item;
        item.isUser(true);
        item.userName = g_buf.AddString(userName);
        item.clrText = COLOR_TEXT_ERR;

        userArr.push_back(item);
        _re_g_data();
        break;
    }
    case ID_MENU_DEL:       // ɾ����Ŀ
    {
        int index = m_list.GetIndex();
        if ( index < 0 )
            return;
        if ( MessageBoxW(g_hWndDlg,
                         L"�Ƿ������ǰѡ�и���?\r\n"
                         L"���������ύ����������, ���ݽ����ɻָ�\r\n\r\n"
                         L"�Ƿ�Ҫɾ��?", L"�Ƿ�ɾ��?",
                         MB_ICONQUESTION | MB_YESNO) == IDNO )
        {
            return; // ����, ������ִ��, break�����Ļ�, �����ѵ�ǰ�赥д��������
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
    case ID_MENU_CLEAR:     // �����Ŀ
    {
        if ( MessageBoxW(g_hWndDlg,
                         L"�Ƿ������ǰ�赥���и���?\r\n"
                         L"���������ύ����������, ���ݽ����ɻָ�\r\n\r\n"
                         L"�Ƿ�Ҫ���?", L"�Ƿ����?",
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
    case ID_MENU_ADDMY:     // ����ǰ������ӵ��Լ��ĸ赥��
    {
        USER_ARR_DATA& userDataCurrent = g_user[0];
        std::vector<LIST_DATA_STRUCT>& userArr = userDataCurrent.arr;
        int index = m_list.GetIndex();
        PLIST_DATA_STRUCT& item = g_data[index];
        _set_item_data(userArr, *item);
        _re_g_data();
        break;
    }
    case ID_MENU_ADDMYALL:  // ����ǰ�赥��ӵ��Լ��ĸ赥��
    {
        for ( PLIST_DATA_STRUCT pItem : g_data )
        {
            if ( pItem->isUser() )
                continue;   // �����Լ��ĸ赥
            _set_item_data(userArr, *pItem);
        }
        _re_g_data();
        break;
    }
    default:
        break;
    }
    update_list();
    ListSong_GetListData(true); // ���浽����
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

        // ǰ��û��ƥ��ĺ�����ݸ�������
        return ret(pItem1->pszSongName, pItem2->pszSongName);
    }
    inline bool ret(LPCWSTR name1, LPCWSTR name2)
    {
        int cmp = wcscmp(name1, name2);
        bool isGreater = __query(mode, SORT_UPTOLOW);   // �Ƿ�Ӵ�С����
        if ( isGreater ) return cmp > 0;
        return cmp < 0;
    }
    // ���������־���ض�Ӧ��ֵ, ��ֵ�Ƚ�, ��Ⱦͷ��ظ����Ƚ�
    inline bool cmp(int v1, int v2)
    {
        if ( v1 == v2 )
            return ret(pItem1->pszSongName, pItem2->pszSongName);
        bool isGreater = __query(mode, SORT_UPTOLOW);   // �Ƿ�Ӵ�С����
        if ( isGreater )
            return v1 > v2;
        return v1 < v2;
    }
};

void ListView_SortList(int mode)
{
    if ( g_data.size() < 2 )
        return; // ��һ����Աû��Ҫ������

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
        return; // ��һ����Աû��Ҫ������

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
    case ID_BTN_UPDATE:     // ���°�ť, ��Ҫ���µ���������
        UpdateBtnClick();
        break;
    case ID_BTN_FIND:       // ����
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
        // ����õ�����ѡ�к�ı�ǰ��״̬
        bool isCheck = m_menu_song.GetCheck(id);
        LPCWSTR isPlay;
        if ( isCheck )
        {
            // ԭ����ѡ��, ��������ȡ��ѡ��, ��ô����Ҫֹͣ����
            isPlay = L"0";
            mp3PlayStop(false);
        }
        else
        {
            // ԭ������ѡ��, �ǵ�������ѡ��, ���Ÿ���
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
    case ID_MENU_SORT_LEVEL:    // ���Ǽ�����
    case ID_MENU_SORT_NAME:     // ����������
    case ID_MENU_SORT_ARTIST:   // ����������
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
    case ID_MENU_SORT_COMBO:    // ����, ����������
    case ID_MENU_SORT_TIMER:    // ����, ��ʱ������
    case ID_MENU_SORT_SONG:     // ����, ����������
    case ID_MENU_SORT_SINGER:   // ����, ����������
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
        memcpy(dlg.caption, L"����", 6);

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
                    hFontEdit = CWndBase::CreateFontW(L"΢���ź�", -16);
                
                SendMessageW(hEdit, WM_SETFONT, (WPARAM)hFontEdit, 0);
                const LPCWSTR text =
                    L"���Ͻ��û��赥:\r\n"
                    L"    ����ѡ��鿴ĳ���˷���ĸ赥, ������ӷ�������ȡ�赥, ��ֻ��ʾ�Լ��ı��ظ赥\r\n"
                    L"    ��һ��ѡ����ȫ���û��ĸ赥, ���Բ鿴�����ڷ������ǼǼ�¼���û�����ĸ赥\r\n"
                    L"    �ڶ���ѡ�����Լ��ĸ赥, ֻ�ܱ༭�Լ��ĸ赥����, �Լ��ĸ赥������ɫ��ͬ\r\n"
                    L"    ���һ��ѡ�������ٱ��صĸ赥, �״δ򿪵�ʱ����������Ŀ¼�µ����и���\r\n\r\n"
                    L"���Ͻ���������, ��������ֱ������, ֧����������ģʽ\r\n"
                    L"    ^ģʽ, ƥ��ͷ, �������� \"^re\",  ��ô�ͻ��г�����\"re\"��ͷ�ĸ���/����\r\n"
                    L"    $ģʽ, ƥ��β, �������� \"$re\",  ��ô�ͻ��г�����\"re\"��β�ĸ���/����\r\n"
                    L"    #ģʽ, ����������, �������� #1104 �ͻ��г�������1104�ĸ���\r\n"
                    L"    *ģʽ, �����ȼ�, �������� \"*9.5\" �ͻ��г�����9.5�ǵĸ���\r\n"
                    L"    !ģʽ, ����BPM, �������� \"!180\" �ͻ��г�����BPM����180�ĸ���\r\n"
                    L"    %ģʽ, ����ʱ��, �������� \"*120\" �ͻ��г�����120��ĸ���\r\n"
                    L"    &&��������, ��Ҫ�������ͬʱ����, ��ο����������\r\n"
                    L"    ����/�Ǽ�/ʱ��/BPM ֧�ֱȽϷ���, >, <, >=, <=, �ֱ��Ǵ���, С��, ���ڵ���, С�ڵ���\r\n"
                    L"        ���� \"#>1000\" ���г�������������1000�ĸ���\r\n"
                    L"        ���� \"!<130\" ���г�����BPMС��130�ĸ���\r\n"
                    L"        ���� \"*>=8.5\" ���г�����8.5�ʹ���8.5�ǵĸ���\r\n"
                    L"        ���� \"*>=8.5 && !>200\" ���г�8�����ϲ���BPM����200�ĸ���, ������8��\r\n"
                    L"    ����֧�ֶ���������, && �ǲ��ҵ�����, ����\",\"����\"||\"�ǻ��ߵ�����, ����:\r\n"
                    L"        ���� \"%>=120 && %<=150 && #>999 || !>220\"\r\n"
                    L"        �������ģʽ��� ���ڵ���120��, ����С�ڵ���150��ĸ���\r\n"
                    L"        ���� ��������999, �ֻ��� BPM����220�ĸ���\r\n"
                    L"        ���ģʽ���г�ʱ���� 120-150��֮����������999�ĸ���\r\n"
                    L"        �����г�BPM����220�ĸ���\r\n"
                    L"    ����������ʽ�ȴ���������\r\n\r\n"
                    L"��������м�Ĵ�༭��:\r\n"
                    L"    ��������һЩ��ע, ������ʱ�������������ע������\r\n"
                    L"    ����, \"Oracle\" ���׸�, �����Ǽǲ�ס����, ��ֻ�ǵ�����\"����\"\r\n"
                    L"    ������ \"Oracle\" ���׸�ı�ע���¼ \"����\", �Ժ�������\"����\"�����ѵ���\r\n"
                    L"    �ٱ��� Jetking, RetroPoktan, ��ֻ�ǵý�JK��RP, �ǾͿ��԰�JK��RPд����ע��\r\n"
                    L"    Ҳ�����Լ�дһЩ�����������������Լ��Ĺؼ���\r\n\r\n"
                    L"���µ��б�:\r\n"
                    L"    ��ʾ�����б��õ�, ����б�����Խ�������\r\n"
                    //L"    ��ʾ�赥�ĸ����õ�, ����б�����Խ�������\r\n"
                    //L"    �����Լ��赥�ĸ���, ��ɫ������ɫ������ʾ\r\n"
                    //L"    �Լ��赥�ĸ�����ɫ����, ��ɫ������ʾ\r\n"
                    //L"    �޸Ĺ�û�ύ���������ĸ���, ��ɫ�ı���ʾ, ���������һ��\r\n"
                    //L"    ѡ��ĳ��������, �ڸ���������һ��, ���ԶԸ������б༭\r\n"
                    //L"    �༭��ʱ��ᵯ��һ�������б�, ˫���б�ĸ������Կ��ٱ༭����\r\n\r\n"
                    L"�Ҽ��˵�:\r\n"
                    L"    ��������������˼, ����в�������˵��....\r\n\r\n"
                    L"��ʲô������߳����������\r\n"
                    L"������ϵ����, QQ 121007124\r\n\r\n"
                    L"Ŀǰ�������뷨��ʵ��, ����ʱ�䲻�Ǻܳ���, Ҳ��֪����Щ���ܻ᲻��ʵ��\r\n"
                    L"    1. ���޹���, ��������ĸ���һ�㶼�����ñȽ�����, ֵ�ö���\r\n"
                    L"    2. ��ȹ���, ����޲��, �������˱ܿ�, �ȵö�Ļ����Ƿ���\r\n"
                    L"    3. �ղ�, �ı�ע, �����ղ�, �����ղع���\r\n"
                    L"        ���Ը�������һЩ��ע, ������ʱ��Ϳ���������ע��\r\n\r\n"
                    L"Ŀǰ������������֪����:\r\n"
                    L"    1. �����б���ֱ�ӻ�ȡmusic.txt�µ�����, ���Ի��в��ָ�����Ϸ�ﲢû��\r\n"
                    L"    2. ֻ֧��528�İ汾, pro���ѡ��call��QQ�����¼��, Q������....\r\n"
                    L"    3. ���ָ���������MP3��ͼƬ��Ϣ, pak�ļ�������, û�н��ܹ���\r\n"
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
            title.assign(L"�赥�б� - ���ڴӷ�������ȡ�赥����").append(*pInc, '.');
            SetWindowTextW(g_hWndDlg, title.c_str());
        };

        int inc = 0;
        SetTimer(g_hWndDlg, (UINT_PTR)&inc, 120, pfn_timer);
        _str listData;
        DWORD ret = newThread(ThreadProc_GetSongList, &listData, true);
        KillTimer(g_hWndDlg, (UINT_PTR)&inc);
        switch ( ret )
        {
        case -1: MessageBoxW(g_hWndDlg, L"���������������ʧ��, ���Ժ�����", L"��������ʧ��", 0); break;
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
                // ��ǰ�û�, ʹ����һ���ṹȥ��Ž��
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
                    item2.clrText = COLOR_TEXT_ERR;  // ���ص����ݶ�������û���ύ��, �������������޸Ļ���

                // ��ǰ�û�, ��Ҫ�Ų��ظ���
                int save = -1;
                for ( LIST_DATA_STRUCT& item1 : _user.arr )
                {
                    item1.clrText = COLOR_TEXT_OK;  // ��������Ƿ���������, ֱ�Ӹĳ�һ����ɫ
                    bool isExist = false;
                    PLIST_DATA_STRUCT pItemLocal = 0;
                    for ( LIST_DATA_STRUCT& item2 : userData.arr )
                    {
                        // ѭ���ͱ��صĶԱ�, ������ص������׸���, ��ô pItemLocal ���Ǳ��ظ���������
                        if ( wcscmp(item2.pszSongName, item1.pszSongName) == 0 )
                        {
                            pItemLocal = &item2;
                            break;
                        }
                    }

                    if ( !pItemLocal )
                    {
                        // ��������û�����׸�, ֱ�Ӽ�������
                        userData.arr.push_back(item1);
                        continue;
                    }

                    // ԭ�����б������׸���, �ж�һ�������ǲ�����ȫһ��, �����ȫһ���Ͳ�����
                    // ��һ����һ������Ϣ��ѭ���������������Ǳ���

                    // ���, ����ѭ��, ������
                    if ( item1.isEqual(*pItemLocal) )
                    {
                        pItemLocal->clrText = COLOR_TEXT_OK;
                        continue;
                    }

                    // �ߵ��������������Ŀ�в�ͬ�ĵط�, ��Ϣ��ѯ�ʱ����������Ļ��Ǳ��ص�
                    if ( save == -1 )
                    {
                        save = MessageBoxW(g_hWndDlg,
                                           L"�����������뱾�������в�ͬ, �������ػ��Ƿ�����?\r\n\r\n"
                                           L"�� = ����������������\r\n"
                                           L"�� = �������ص�����\r\n"
                                           , L"������һ������", MB_ICONQUESTION | MB_YESNO);
                    }

                    // �������ص�����, ����Ҫ���뵽������
                    // �����������ż�������, ���صĻ��Ѿ�����������
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
                MessageBoxW(g_hWndDlg, L"��¼�ɹ�", L"��¼�ɹ�", 0);
        }
        break;
    }
    case ID_MENU_ADD:       // �����Ŀ
    case ID_MENU_DEL:       // ɾ����Ŀ
    case ID_MENU_CLEAR:     // �����Ŀ
    case ID_MENU_ADDMY:     // ����ǰ������ӵ��Լ��ĸ赥��
    case ID_MENU_ADDMYALL:  // ����ǰ�赥��ӵ��Լ��ĸ赥��
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
            MessageBoxW(0, L"�Ҳ�����Ϸ����", L"���ȴ���Ϸ, ����Ϸ�����ٺ��ٵ������", MB_ICONMASK);
            break;
        }
        DWORD pid;
        GetWindowThreadProcessId(hWndR2, &pid);
        if ( !hWndR2 )
        {
            tstr_MessageBox(0, L"��ȡ��Ϸ��Ϣʧ��", MB_ICONMASK, L"��ȡ��Ϸ����IDʧ��, ��ǰ��ȡ������Ϸ����=%d", hWndR2);
            break;
        }

        HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, false, pid);
        if ( !hProc )
        {
            tstr_MessageBox(0, L"����Ϸ����ʧ��", MB_ICONMASK, L"����Ϸ����ʧ��, ����ID=%d", pid);
            break;
        }
        CDLLInject dll;
        wstr dllFile(260);
        dllFile.assign(_str::A2W(g_argData->pszR2Path)).append(L"R2Inject.dll");
        HMODULE hModule = dll.InjectDll(hProc, dllFile.c_str(), "", 0, 0, 0);
        if ( !hModule )
        {
            tstr_MessageBox(0, L"����ʧ��", MB_ICONMASK, L"ע�����ʧ��, ����ID=%d", pid);
        }
        CloseHandle(hProc);
        break;
    }
    case ID_MENU_IMPORTFILE:
        MessageBoxW(0, L"��δʵ��", L"�Ⱥ�������", 0);
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

// �����ύ��������������
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
        int n = MessageBoxW(g_hWndDlg, L"��ǰ����Ϊ��, �Ƿ�Ҫ�ύ��������?\r\n"
                            L"�ύ�󽫷����������ݽ��ᱻ���, �޷��ָ�\r\n"
                            L"\r\n�Ƿ�Ҫ���?\r\n"
                            , L"�Ƿ��������������?", MB_ICONQUESTION | MB_YESNO);
        if ( n == IDNO )
            return;
    }
    else if ( !isUpdate )
    {
        MessageBoxW(g_hWndDlg, L"�б�����û�б䶯, ����Ҫ�ύ", L"û��������Ҫ����", MB_ICONINFORMATION);
        return;
    }
    int _index = 0, _index2 = 0;
    wstr _dbgText(260);
    int addCount = 0;

    _str listData;
    if ( !ListSong_GetListData(true, &listData) )
    {
        _dbgText.assign(L"��ȡ��Ŀ����ʧ��, ������ַ�ʽ���͸�����, �ø����޸�");
        MessageBoxW(g_hWndDlg, _dbgText.c_str(), L"��ȡ����ʧ��", 0);
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
    case -1: MessageBoxW(g_hWndDlg, L"�������ݵ�������ʧ��", L"��������ʧ��", 0); break;
    case -2: MessageBoxW(g_hWndDlg, L"�ύʧ��, �����û������������Ƿ���ȷ", L"�ύ����ʧ��", 0); g_token.clear(); break;
    case -3: MessageBoxW(g_hWndDlg, L"�ύʧ��, ��¼״̬��ʧЧ, �����µ�¼", L"��¼״̬��ʧЧ", 0); g_token.clear(); break;
    case -10: MessageBoxW(g_hWndDlg, L"��¼״̬ʧЧ, �����µ�¼", L"�����µ�¼", 0); g_token.clear(); break;
    case -11: MessageBoxW(g_hWndDlg, L"δע����˻�", L"δע����˻�", 0); g_token.clear(); break;
    case -12: MessageBoxW(g_hWndDlg, L"�ύ�˿յ�����", L"�ύ�˿յ�����", 0); break;
    default:
        break;
    }
    if ( ret == 121007124 )
    {
        for ( LIST_DATA_STRUCT& item : arr )
        {
            // ѭ����������Ŀ����ɫ��һ��
            item.clrText = COLOR_TEXT_OK;
        }
        ListSong_GetListData(true);
        m_list.InvalidateRect();
    }
    else
    {
        MessageBoxW(g_hWndDlg, L"����ʧ��, ���Ժ�����", L"����ʧ��", 0);
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
        case L'��': ch = ':'; break;
        case L'��': ch = ','; break;
        case L'��': ch = '>'; break;
        case L'��': ch = '<'; break;
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
                // ѭ����ÿ���ؼ��ֶ�����һ��
                // ������������ʾ�ĸ���, û�о�������ע
                // �ߵ�����ľ��ǲ��ҵ��߼�, ����������������
                // �����ÿ����Ա����һ������, ����ѭ��ƥ��
                // ֻҪ��һ����, �Ǿ�ƥ��ʧ��, ����ִ���������, �Ǿ���ƥ��ɹ�
                // ���ֻ��һ����Ա, ������ֻѭ��һ��.... �����һ��������ͳһ����
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
    case L'^':   // xx��ͷ���ı�
    {
        ++subStr; --subLen;
        return _wcsnicmp(sourceStr, subStr, subLen) == 0;
    }
    case L'$':   // xx��β���ı�
    {
        ++subStr; --subLen;
        LPCWSTR pCmpStart = sourceStr + srcLen - subLen;
        return _wcsicmp(pCmpStart, subStr) == 0;
    }
    case L'*':   // ��������
    {
        ++subStr; --subLen;
        return find_int(subStr, pItem->nLevel, true);
    }
    case L'#':   // ������������
    {
        ++subStr; --subLen;
        return find_int(subStr, pItem->nCombo);
    }
    case L'%':   // ��������ʱ��
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
        // Ĭ������, ���������ı�
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
        ++findStr; --subLen; // ���������ڻ���С�ڵ���ֵ
        const wchar_t& ch2 = *findStr;
        if ( ch2 == '=' )
        {
            // �ߵ��������Ҫ�������ڵ��ڻ���С�ڵ���ĳ����ֵ
            ++findStr; --subLen; // >= ���� <=
            int vl = _float_2_int(findStr, isLevel);
            if ( ch1 == '>' )
                return findInt >= vl;
            return findInt <= vl;
        }

        // �ߵ��������Ҫ�������ڻ���С��ĳ����ֵ
        int vl = _float_2_int(findStr, isLevel);
        if ( ch1 == '>' )
            return findInt > vl;
        return findInt < vl;
    }
    if ( ch1 == '=' )
        findStr++;
    // �������������ֵ
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
    m_edit.SetText(L"");    // ���������, Ȼ�����
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
        tstr_MessageBox(0, L"���ܳ�����", 0, L"Ӧ���ǳ�����, ������ѡ�ĸ����������û�ҵ�, Index = %d", index);
        return; // Ҫ���ߵ��������������
    }

    m_list.SetIndex(i, true);

}

