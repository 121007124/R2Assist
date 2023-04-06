#pragma once
#include "../tstr.h"
#include <windowsx.h>
#include <commctrl.h>
#include "WndBaseState.h"
#include "WndClassName.h"
#include "WndStyleDef.h"
#include "CControlDraw.h"



#define PROP_WNDPROC L"{1DE85EBD-ECDD-4091-93D6-D609A63D9DC6}"
class CWndBase;


//typedef tagSUBCLASSSTRUCT_YEMPLATE<OBJSTRUCT>* LPOBJSUBCLASS;
typedef LRESULT(CALLBACK* WND_SUBPROC)( WNDPROC, HWND, UINT, WPARAM, LPARAM );



typedef struct PROPSTRUCT
{
    PROPSTRUCT() :name(0), value(0) { }
    ~PROPSTRUCT()
    {
        if ( name )
            wstr::free(name);
        name = 0;
        value = 0;
    }

    PROPSTRUCT(const PROPSTRUCT& obj) :name(0), value(0)
    {
        operator=(obj);
    }

    inline PROPSTRUCT& operator=(const PROPSTRUCT& obj)
    {
        if ( !obj.name ) return *this;
        operator=(obj.name);
        value = obj.value;
        return *this;
    }

    inline PROPSTRUCT& operator=(LPCWSTR text)
    {
        if ( !text ) return *this;
        if ( name )
            wstr::free(name);
        name = wstr::copy(text);
        return *this;
    }

    inline PROPSTRUCT& move(PROPSTRUCT& obj)
    {
        if ( !obj.name ) return *this;
        if ( name )
            wstr::free(name);
        name        = obj.name;
        value       = obj.value;
        obj.name    = 0;
        obj.value   = 0;
        return *this;
    }

#if _MSC_VER > 1200
    PROPSTRUCT(PROPSTRUCT&& obj) noexcept :name(0), value(0)
    {
        operator=(std::move(obj));
    }

    inline PROPSTRUCT& operator=(PROPSTRUCT&& obj) noexcept
    {
        return move(obj);
    }
#endif

    LPWSTR name;
    HANDLE value;
}* PPROPSTRUCT, * LPPROPSTRUCT;



typedef struct tagPROP_SUBWNDDATA
{
    CWndBase* pThis;        // �����ڵĻ���ָ��
    LPVOID param;           // �������ڸ��Ӳ���
    LONG_PTR id;            // ����ID
    WNDPROC oldProc;        // �ɹ��̵�ַ
    WND_SUBPROC subFun;     // �û���������໯�ص�����
    bool isSubClass;        // �Ƿ����໯
}PROP_SUBWNDDATA, * LPPROP_SUBWNDDATA;


// �����ڴ�Ի���, ���ضԻ���ص������õķ���ֵ, ʧ�ܷ���0
inline INT_PTR DialogBoxPop(DWORD dwExStyle, LPCWSTR lpszName, LONG style, 
    int x, int y, int nWidth, int nHeight, HWND hWndParent, HINSTANCE hInstance, DLGPROC dlgProc, LPVOID lpParam = 0)
{
    struct __DLGTEMPLATE : DLGTEMPLATE
    {
        WORD menuid;
        WORD cls;
        wchar_t caption[1024];
    };
    __DLGTEMPLATE dlg;
    memset(&dlg, 0, sizeof(dlg));
    dlg.style = style;
    dlg.dwExtendedStyle = dwExStyle;
    dlg.cdit = 0;
    dlg.x = x;
    dlg.y = y;
    dlg.cx = nWidth;
    dlg.cy = nHeight;
    if (!lpszName)lpszName = L"";
    size_t len = wcslen(lpszName) + 1;
    if (len > 1024) len = 1024;
    memcpy(dlg.caption, lpszName, len * sizeof(wchar_t));

    INT_PTR b = DialogBoxIndirectParamW(hInstance, &dlg, hWndParent, dlgProc, (LPARAM)lpParam);
    return b;
}

//template<typename TData, typename TStringA, typename TStringW>
class CWndBase
{
private:
    static inline void __init_data(HFONT* hFont = 0, HINSTANCE* hInst = 0)
    {
        static HFONT s_hFont;
        static HINSTANCE s_hInst;
        if (!s_hInst)
        {
            s_hFont = CWndBase::CreateFontW();
            s_hInst = GetModuleHandleW(0);
        }
        if (hFont)*hFont = s_hFont;
        if (hInst)*hInst = s_hInst;
        return;
    }

public:
    static HINSTANCE GetInst() {
        static HINSTANCE hInst;
        if (!hInst)
            __init_data(0, &hInst);
        return hInst;
    }
    static HFONT GetStaticFont() { return DefaultFont(); }
    static HFONT GetDefaultFont() { return DefaultFont(); }
    static HFONT DefaultFont() {
        static HFONT hFont;
        if (!hFont)
            __init_data(&hFont, 0);
        return hFont;
    }


    CWndBase() :m_hWnd(0), m_data(0)
    {
    }
    ~CWndBase()
    {
        LPPROP_SUBWNDDATA param = (LPPROP_SUBWNDDATA)GetProp(PROP_WNDPROC);
        if (param)
            param->pThis = 0;
        if (m_data)delete m_data;
        m_data = 0;
        //clear();
    }
    HWND m_hWnd;
    LPPROP_SUBWNDDATA m_data;   // ���໯�����õ�����

    static WNDPROC CWndBase_GetSubWndData(HWND hWnd)
    {
        WNDPROC oldProc = 0;

        // �ߵ����һ���ǳ��໯, ���໯�Ļ��������� PROP_WNDPROC ���ֵ�����໯
        int size = (int)GetClassLongPtrW(hWnd, GCL_CBCLSEXTRA);
        if (size < 8)    // ���໯�Ķ�����ߴ�һ����С��8�ֽ�
            return 0;
        const int GCLP_OLDPROC = size - sizeof(WNDPROC);
        oldProc = (WNDPROC)GetClassLongPtrW(hWnd, GCLP_OLDPROC);    // �����ľɹ��̵�ַ, �����Ϊ0, ������Ǿɹ��̵�ַ, ����ֱ��ʹ��, �������ͬ����ͨ�õ�

        if (!oldProc)       // �ɻص�����Ϊ0���ʾ����໹û�аѾɻص���ַд���������Ϣ��, ��Ҫд��
        {
            LPBYTE pMem = (LPBYTE)GetClassLongPtrW(hWnd, GCLP_MENUNAME);
            if (!pMem) return 0;

            const size_t len = wcslen((LPCWSTR)pMem) * 2;
            for (size_t i = 0; i < len; i++)
                pMem[i] ^= 121;

            memcpy(&oldProc, pMem, sizeof(oldProc));
            pMem += sizeof(oldProc);
            if (pMem[0] == 0) pMem = 0;    // ǰ���žɹ���, ������ԭ�˵�, ���ԭ�˵�Ϊ0, ��˵�����

            SetClassLongPtrW(hWnd, GCLP_MENUNAME, (LONG_PTR)pMem);      // ע�����ʱ��������ʱ��žɹ���
            SetClassLongPtrW(hWnd, GCLP_OLDPROC, (LONG_PTR)oldProc);    // �������ľɹ��̵�ַ��ŵ������ߴ�����λ��
        }

        return oldProc;
    }
    

protected:

    static LRESULT CALLBACK CWndBase_DefWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        LPPROP_SUBWNDDATA data = (LPPROP_SUBWNDDATA)::GetPropW(hWnd, PROP_WNDPROC);
        WNDPROC oldProc = 0;
        if ( !data )
            oldProc = CWndBase_GetSubWndData(hWnd);
        
        switch ( message )
        {
        case WM_NCCREATE:   // �ǿͻ������ڴ���,��Ϣѭ���ĵ�һ����Ϣ
        {
            //! ���ߵ������Ϣ���һ���ǳ��໯, ������ע�����, �϶��ǵ��� CWndBase::create() ��������
            // ��Ҫ�ڴ��ڴ����ĵ�һ����Ϣ��ĵ����ֵ
            LPCREATESTRUCT ct = (LPCREATESTRUCT)lParam;
            LPPROP_SUBWNDDATA ptr = (LPPROP_SUBWNDDATA)ct->lpCreateParams;
            if ( ptr )
            {
                ct->lpCreateParams = ptr->param;    // �û����ݵ�lParam, �Ļ��û����ݵ�ֵ
                ptr->pThis->m_hWnd = hWnd;
                ::SetPropW(hWnd, PROP_WNDPROC, ptr);
                data = ptr;
                data->oldProc = oldProc;
            }
            break;
        }
        case WM_NCDESTROY:
        {
            ::RemovePropW(hWnd, PROP_WNDPROC);
            if ( oldProc )
                return CallWindowProcW(oldProc, hWnd, message, wParam, lParam);
            return DefWindowProcW(hWnd, message, wParam, lParam);
        }
        case WM_DESTROY:
        {
            if ( data)
            {
                CWndBase* pThis = data->pThis;
                if ( data->isSubClass )
                {
                    oldProc = data->oldProc;
                    SetWindowLongPtrW(hWnd, GWLP_WNDPROC, (LONG_PTR)oldProc);
                }
                
                pThis->m_data = 0;
                pThis->m_hWnd = 0;
                delete data;
                data = 0;
            }
            if ( oldProc )
                return CallWindowProcW(oldProc, hWnd, message, wParam, lParam);
            return DefWindowProcW(hWnd, message, wParam, lParam);
        }
        default:
            break;
        }

        if ( !data )
        {
            if ( !oldProc )
                return DefWindowProcW(hWnd, message, wParam, lParam);
            return CallWindowProcW(oldProc, hWnd, message, wParam, lParam);
        }

        CWndBase* pThis = data->pThis;
        oldProc = data->oldProc;

        if (pThis) // ���ⲿ��������ķ���,���в��������������
        {
            if (data->subFun)
                return data->subFun(oldProc, hWnd, message, wParam, lParam);
            return pThis->WndProc(oldProc, hWnd, message, wParam, lParam);
        }

        if (!oldProc)
            return DefWindowProcW(hWnd, message, wParam, lParam);
        return CallWindowProcW(oldProc, hWnd, message, wParam, lParam);
    }
    // ���ڻ����ע������, �����Ҫ���໯, ��̳� SuperClassName() ������Ҫ���໯������
    // ִ������
    // 1. ���className()�Ѿ�ע��, �򷵻�className();
    // 2. ȡ��SuperClassName()��Ĭ�ϻص�����, ���SuperClassName()Ϊ����ʹ�� DefWindowProcW;
    // 3. ע��className(), �ص������Ǵ������ڵ�Ĭ�ϴ���ص�����
    LPCWSTR WndBaseRegisterClass(WNDPROC proc = 0, UINT style = 0, HBRUSH hbrBackground = 0, HCURSOR hCursor = 0, HICON hIcon = 0, HICON hIconSm = 0, int cbClsExtra = 0, int cbWndExtra = 0, WNDPROC* pOldProc = 0)
    {
        LPCWSTR newClassName = className();
        if (!newClassName)return newClassName;
        WNDCLASSEXW wcex = { 0 };
        wcex.cbSize = sizeof(WNDCLASSEXW);
        if (GetClassInfoExW(GetInst(), newClassName, &wcex))
            return newClassName;

        LPCWSTR oldClassName = SuperClassName();
        if (oldClassName)    // ��������Ϊ�ղŻ�ȡ����Ϣ
        {
            if (GetClassInfoExW(GetInst(), oldClassName, &wcex))
            {
                // ��ȡ����Ϣ�ɹ���ʾ�Ѿ�ע��, �����¾��������, ֱ�ӷ���������
                if (wcsicmp_api(oldClassName, newClassName) == 0)
                    return newClassName;
            }
            else
            {
                // ����Ϣ��ȡʧ��, ��������������һ���ͷ���, һ���Ļ�������ע��
                if (wcsicmp_api(oldClassName, newClassName) != 0)
                    return newClassName;
            }
        }

        WNDPROC oldProc = wcex.lpfnWndProc;
        if (!oldProc)
            oldProc = DefWindowProcW;    // �����ֱ��ע��, ��ԭ�ص���ַ����Ĭ�ϻص�

        if (!proc)proc = CWndBase::CWndBase_DefWndProc;
        if (!style)style = CS_VREDRAW | CS_HREDRAW;
        if (!hCursor)hCursor = LoadCursor(0, IDC_ARROW);
        if (!hbrBackground)hbrBackground = wcex.hbrBackground;
        if (wcex.style) style |= wcex.style;
        if (pOldProc)*pOldProc = oldProc;

        wcex.lpfnWndProc = proc;
        wcex.lpszClassName = newClassName;
        wcex.cbWndExtra += (8 + cbWndExtra);
        wcex.cbClsExtra += (8 + cbClsExtra);
        wcex.style = style;
        wcex.hCursor = hCursor;
        wcex.hbrBackground = hbrBackground;

        size_t len = 0;
        const size_t cbSize = sizeof(oldProc);
        if (wcex.lpszMenuName)
            len = wcslen(wcex.lpszMenuName);

        len = len * 2 + 2;
        BYTE pMem[280] = { 0 };
        memcpy(pMem, &oldProc, cbSize);
        if (wcex.lpszMenuName && len > 2)
            memcpy(pMem + cbSize, wcex.lpszMenuName, len);
        len += cbSize;    // ���ֽڳ��� = ԭ�ַ������� + �ɹ��̵�ַ����

        for (size_t i = 0; i < len; i++)
            pMem[i] ^= 121;

        wcex.lpszMenuName = (LPCWSTR)pMem;
        ATOM atom = RegisterClassExW(&wcex);
        assert(atom != 0);
        atom = GlobalAddAtomW(wcex.lpszClassName);
        assert(atom != 0);
        return newClassName;
    }

    // ���໯������, ������Ҫ���໯, static, ��̳������������static, className() �������ǳ��໯���������
    // ����0�򲻽��г��໯
    virtual inline LPCWSTR SuperClassName()
    {
        return 0;
    }

public:
    // Ĭ��Ϊ΢���ź�, �����С -12, û����ʽ
    // lfWeight = ����,  lfItalic = б��,  lfUnderline = �»���, lfStrikeOut = ɾ����
    static HFONT CreateFontW(const wchar_t* name = L"΢���ź�", int size = -12, bool lfWeight = false, bool lfItalic = false, bool lfUnderline = false, bool lfStrikeOut = false)
    {
        LOGFONTW lf = { 0 };
        SystemParametersInfoW(SPI_GETICONTITLELOGFONT, sizeof(lf), &lf, 0);
        lf.lfHeight = size;
        if (lfWeight)
            lf.lfWeight = FW_BOLD;
        lf.lfItalic = lfItalic;
        lf.lfUnderline = lfUnderline;
        lf.lfStrikeOut = lfStrikeOut;
        lf.lfCharSet = GB2312_CHARSET;
        if (!name) name = L"΢���ź�";
        size_t len = wcslen(name) + 1;
        memcpy(lf.lfFaceName, name, len * sizeof(wchar_t));
        return CreateFontIndirectW(&lf);
    }
    virtual LRESULT WndProc(WNDPROC oldProc, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        return CallWindowProcW(oldProc, hWnd, message, wParam, lParam);
    }
    virtual inline LPCWSTR className()
    {
        return 0;
    }

    // ֪ͨ��Ϣ
    virtual LRESULT OnNotify(WNDPROC oldProc, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
        return CallWindowProcW(oldProc, hWnd, message, wParam, lParam);
    }

    virtual HWND create(DWORD dwExStyle, LPCWSTR lpszTitle, DWORD dwStyle,
        int x, int y, int nWidth, int nHeight, HWND hWndParent, LONG_PTR id = 0, LPVOID lpParam = 0)
    {
        if (IsWindow()) return GetWindow();
        if (!m_data)
        {
            m_data = new PROP_SUBWNDDATA;
            memset(m_data, 0, sizeof(PROP_SUBWNDDATA));
        }
        m_data->id      = (LONG_PTR)id;
        m_data->pThis   = this;
        m_data->param   = lpParam;
        m_data->oldProc = 0;

        m_hWnd = CreateWindowExW(dwExStyle, WndBaseRegisterClass(), lpszTitle, dwStyle, x, y, nWidth, nHeight, hWndParent, (HMENU)(LONG_PTR)id, GetInst(), m_data);
        if (!m_hWnd)
        {
            delete m_data;
            m_data = 0;
            return 0;
        }
        SetProp(PROP_WNDPROC, m_data);

        if (m_data->oldProc == 0 && m_data->isSubClass)    // ����ɴ��ڹ���û��ֵ, ������Ҫ���໯
        {
            m_data->oldProc = (WNDPROC)SetWindowLongPtrW(m_hWnd, GWLP_WNDPROC, (LONG_PTR)CWndBase::CWndBase_DefWndProc);
        }
        SetFont(DefaultFont());
        return m_hWnd;
    }

    inline static bool WINAPI GetClientRect(HWND hWnd, RECT* prc) {
        return ::GetClientRect(hWnd, prc);
    }
    inline static bool WINAPI GetWindowRect(HWND hWnd, RECT* prc) {
        return ::GetWindowRect(hWnd, prc);
    }
    // ȡ���ڿͻ�������ߴ�, ��߶����ǿͻ�������Ļ�е�λ��
    inline static bool WINAPI GetClientSize(HWND hWnd, RECT* prc)
    {
        if (!hWnd || !prc)return false;

        bool ret = ::GetClientRect(hWnd, prc);
        if(!ret)return false;
        const int cxClient = prc->right - prc->left;  // ��¼���ڿͻ����ߴ�
        const int cyClient = prc->bottom - prc->top;
        ClientToScreen(hWnd, (LPPOINT)prc);
        prc->right = prc->left + cxClient;
        prc->bottom = prc->top + cyClient;
        return true;
    }

    // ȡ���ھ���, �Ӹ��������ȡ, ���ص��Ƿǿͻ����ڸ������е�λ��, ���ҵ�
    inline static bool WINAPI GetRectFromParent(HWND hWnd, RECT* prc) {
        if (!hWnd || !prc)return false;
        GetRect(hWnd, 0, 0, 0, prc);
        const int width = prc->right - prc->left;
        const int height = prc->bottom - prc->top;
        HWND hParent = GetParent(hWnd);
        if (hParent)
            ScreenToClient(hParent, (LPPOINT)prc);
        
        prc->right = prc->left + width;
        prc->bottom = prc->top + height;
        return true;
    }

    // ȡ���ڷǿͻ������ҵױ߿���
    // prcBorder = ����4���߿�Ŀ��, Ϊ0�򲻻�ȡ, 4���߶�Ϊ0��ʾû�б߿�
    // prcWindow = ���ոô�������Ļ�е�λ��, GetWindowRect() ���ص�ֵ
    // prcClient = ���ոô��ڵĿͻ���λ��, GetClientRect() ���ص�ֵ
    // prcClientScreen = ���ոô��ڿͻ�������Ļ�е�λ��, ȥ���˱߿�
    inline static bool WINAPI GetRect(HWND hWnd, RECT* prcBorder = 0, RECT* prcWindow = 0, RECT* prcClient = 0, RECT* prcClientScreen = 0) {
        if (!hWnd)return false;
        if (!prcBorder && !prcWindow && !prcClientScreen)return false;
        RECT rc = { 0 }, rcClient = { 0 }, rcBorder = { 0 };
        ::GetWindowRect(hWnd, &rc);
        ::GetClientRect(hWnd, &rcClient);
        if (prcWindow)memcpy(prcWindow, &rc, sizeof(RECT));
        if (prcClient)memcpy(prcClient, &rcClient, sizeof(RECT));

        int cxClient = rcClient.right - rcClient.left;  // ��¼���ڿͻ����ߴ�
        int cyClient = rcClient.bottom - rcClient.top;

        ::ClientToScreen(hWnd, (LPPOINT)&rcClient);
        rcClient.right = rcClient.left + cxClient;
        rcClient.bottom = rcClient.top + cyClient;

        rcBorder.left = rcClient.left - rc.left;
        rcBorder.top = rcClient.top - rc.top;
        rcBorder.right = rc.right - rcClient.right;
        rcBorder.bottom = rc.bottom - rcClient.bottom;

        if (prcBorder)
            memcpy(prcBorder, &rcBorder, sizeof(RECT));
        if (!prcClientScreen)return true;

        rcClient.left = rcBorder.left + rc.left;
        rcClient.top = rcBorder.top + rc.top;
        rcClient.right = rcClient.left + cxClient;
        rcClient.bottom = rcClient.top + cyClient;
        memcpy(prcClientScreen, &rcClient, sizeof(RECT));
        return true;
    }
    

public:
    inline operator HWND()      const   { return m_hWnd; }
    inline bool     Update()    const   { return ::UpdateWindow(m_hWnd); }
    inline bool     IsWindow()  const   { return ::IsWindow(m_hWnd); }
    inline bool     Destroy()   { bool ret = ::DestroyWindow(m_hWnd); m_hWnd = 0; return ret; }
    inline HWND     GetWindow() const   { return m_hWnd; }
    inline bool     SetFocus()  const   { return ::SetFocus(m_hWnd) != NULL; }
    inline bool     IsFocus()   const   { return ::GetFocus() == m_hWnd; }
    inline int      GetID()     const   { return GetDlgCtrlID(m_hWnd); }
    // ȡ���ڵ�Ĭ����Ϣ�ص�����, �ú��������ؿ�
    inline WNDPROC  GetDefProc() const  {
        if (m_data && m_data->oldProc)
            return m_data->oldProc;
        return DefWindowProcW;
    }

    // �������ڵ�Ĭ�ϴ�����
    inline bool     CallProc(WNDPROC oldProc, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
        if (!m_hWnd)return false;
        return WndProc(oldProc, hWnd, message, wParam, lParam);
    }

    // ȡ���ڿͻ�λ��
    inline bool GetClientRect(RECT* prc) const {
        if (!m_hWnd)return false;
        return ::GetClientRect(m_hWnd, prc);
    }

    // ȡ��������Ļ�е�λ��
    inline bool GetWindowRect(RECT* prc) const {
        if (!m_hWnd)return false;
        return ::GetWindowRect(m_hWnd, prc);
    }

    // ȡ���ڷǿͻ������ҵױ߿���
    // prcBorder = ����4���߿�Ŀ��, Ϊ0�򲻻�ȡ
    // prcWindow = ���ոô�������Ļ�е�λ��
    // prcClient = ���ոô��ڵĿͻ���λ��, GetClientRect() ���ص�ֵ
    // prcClientScreen = ���ոô��ڿͻ�������Ļ�е�λ��, ȥ���˱߿�
    inline bool GetRect(RECT* prcBorder = 0, RECT* prcWindow = 0, RECT* prcClient = 0, RECT* prcClientScreen = 0) const {
        return GetRect(m_hWnd, prcBorder, prcWindow, prcClient, prcClientScreen);
    }

    // ȡ���ھ���, �Ӹ��������ȡ, ���ص��Ǵ����ڸ������е�λ��, ���ҵ�
    inline bool GetRectFromParent(RECT* prc) const {
        return GetRectFromParent(m_hWnd, prc);
    }

    // ȡ���ڿ��, �����˷ǿͻ���
    inline int GetScreenWidth() const {
        if (!m_hWnd)return 0;
        RECT rc = { 0 };
        ::GetClientRect(m_hWnd, &rc);
        return rc.right - rc.left;
    }

    // ȡ���ڸ߶�, �����˷ǿͻ���
    inline int GetScreenHeight() const {
        if (!m_hWnd)return 0;
        RECT rc = { 0 };
        ::GetClientRect(m_hWnd, &rc);
        return rc.bottom - rc.top;
    }

    // ȡ�����ڸ������е�����
    inline bool GetClientPos(LPPOINT ppt) const {
        if (!m_hWnd || !ppt)return false;
        RECT rc;
        if (!GetRectFromParent(m_hWnd, &rc)) return false;
        ppt->x = rc.left;
        ppt->y = rc.top;
        return true;
    }

    // ���ô����ڸ������е�����
    inline bool SetClientPos(const LPPOINT ppt) const {
        if (!ppt) return false;
        return SetWindowPos(m_hWnd, 0, ppt->x, ppt->y, 0, 0, SWP_NOOWNERZORDER | SWP_NOSIZE);
    }


    // ȡ���ڿ��, �����ǿͻ���
    inline bool GetSize(int* width, int* height) const {
        if (!m_hWnd)return false;
        if (!width && !height)return false;
        RECT rc = { 0 };
        bool ret = ::GetWindowRect(m_hWnd, &rc);
        if (!ret)return false;

        if (width)  *width  = rc.right - rc.left;
        if (height) *height = rc.bottom - rc.top;
        return true;
    }

    // ���ô��ڿ��
    inline bool SetSize(int width, int height) const {
        if (!m_hWnd)return false;
        return SetWindowPos(m_hWnd, 0, 0, 0, width, height, SWP_NOZORDER | SWP_NOMOVE);
    }

    // ���ô������
    inline bool SetLeft(int left) const {
        if (!m_hWnd)return false;
        RECT rc = { 0 };
        ::GetWindowRect(m_hWnd, &rc);
        return SetWindowPos(m_hWnd, 0, left, rc.top, 0, 0, SWP_NOZORDER | SWP_NOSIZE);

    }

    // ���ô��ڶ���
    inline bool SetTop(int top) const {
        if (!m_hWnd)return false;
        RECT rc = { 0 };
        ::GetWindowRect(m_hWnd, &rc);
        return SetWindowPos(m_hWnd, 0, rc.left, top, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
    }

    // ���ô��ڿ��
    inline bool SetWidth(int width) const {
        if (!m_hWnd)return false;
        RECT rc = { 0 };
        ::GetWindowRect(m_hWnd, &rc);
        return SetWindowPos(m_hWnd, 0, 0, 0, width, rc.bottom - rc.top, SWP_NOZORDER | SWP_NOMOVE);

    }

    // ���ô��ڸ߶�
    inline bool SetHeight(int height) const {
        if (!m_hWnd)return false;
        RECT rc = { 0 };
        ::GetWindowRect(m_hWnd, &rc);
        return SetWindowPos(m_hWnd, 0, 0, 0, rc.right - rc.left, height, SWP_NOZORDER | SWP_NOMOVE);
    }

    // ȡ�������
    inline int GetLeft() const {
        if (!m_hWnd)return 0;
        RECT rc;
        GetRectFromParent(m_hWnd, &rc);
        return rc.left;
    }

    // ȡ���ڶ���
    inline int GetTop() const {
        if (!m_hWnd)return 0;
        RECT rc;
        GetRectFromParent(m_hWnd, &rc);
        return rc.top;
    }
    // ȡ���ڸ߶�, ���ص��ǿͻ����Ŀ��
    inline int GetWidth() const {
        if (!m_hWnd)return 0;
        RECT rc = { 0 };
        ::GetClientRect(m_hWnd, &rc);
        return rc.right - rc.left;
    }

    // ȡ���ڸ߶�, ���ص��ǿͻ����ĸ߶�
    inline int GetHeight() const {
        if (!m_hWnd)return 0;
        RECT rc = { 0 };
        ::GetClientRect(m_hWnd, &rc);
        return rc.bottom - rc.top;
    }

    // ��ȡ��������, ���û�и��������򷵻ؿվ���, �������궼Ϊ0
    // prc = ���ո��������RECT�ṹָ��
    // bErase = ָ���Ƿ�Ҫɾ�����������еı���������˲���ΪTRUE���Ҹ�������Ϊ�գ���GetUpdateRect��WM_ERASEBKGND��Ϣ���͵�ָ���Ĵ����Բ���������
    inline bool GetUpdateRect(RECT* prc = 0, bool bErase = false) const {
        if (!m_hWnd)return false;
        return ::GetUpdateRect(m_hWnd, prc, bErase);
    }
    inline bool InvalidateRect(const RECT* prc = 0, bool bErase = false) const {
        if (!m_hWnd)return false;
        return ::InvalidateRect(m_hWnd, prc, bErase);
    }

    inline bool InvalidateRgn(HRGN hRgn, bool bErase = false) const {
        if (!m_hWnd)return false;
        return ::InvalidateRgn(m_hWnd, hRgn, bErase);
    }

    inline bool Move(int x, int y, int cx, int cy, bool bRepaint = true) const {
        if (!m_hWnd)return false;
        return MoveWindow(m_hWnd, x, y, cx, cy, bRepaint);
    }

    inline bool Move(const RECT& rc, bool bRepaint = true) const {
        if (!m_hWnd)return false;
        return Move(rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, bRepaint);
    }

    inline bool Move(const RECT* rc, bool bRepaint = true) const {
        if (!m_hWnd)return false;
        return Move(rc->left, rc->top, rc->right - rc->left, rc->bottom - rc->top, bRepaint);
    }

    

    // ֪ͨ�߿�ı�, ���޸Ĵ���λ��, ǿ�ƴ��� WM_NCCALCSIZE���¼���߿�
    inline bool ChangedBorder() const {
        if (!m_hWnd)return false;
        return SetWindowPos(m_hWnd, 0, 0, 0, 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_NOMOVE | SWP_FRAMECHANGED);
    }

    // �������
    inline bool After(HWND after, UINT uFlags = 0) const {
        if (!m_hWnd)return false;
        return SetWindowPos(m_hWnd, after, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | uFlags);
    }
    
    // ����W����Ϣ
    inline INT_PTR Send(UINT message, WPARAM wParam, LPARAM lParam) const {
        if (!m_hWnd)return 0;
        return (INT_PTR)SendMessageW(m_hWnd, message, wParam, lParam);
    }

    // ����A����Ϣ
    inline INT_PTR SendA(UINT message, WPARAM wParam, LPARAM lParam) const {
        if (!m_hWnd)return 0;
        return (INT_PTR)SendMessageA(m_hWnd, message, wParam, lParam);
    }
#ifndef SMTO_ERRORONEXIT
#define SMTO_ERRORONEXIT 0x20
#endif
    // ����W����Ϣ
    inline INT_PTR SendTimeOut(UINT message, WPARAM wParam, LPARAM lParam) const {
        if (!m_hWnd)return 0;
        DWORD_PTR ret = 0;
        SendMessageTimeoutW(m_hWnd, message, wParam, lParam,
            SMTO_ABORTIFHUNG | SMTO_NOTIMEOUTIFNOTHUNG | SMTO_ERRORONEXIT,
            1000, &ret);
        return (INT_PTR)ret;
    }

    // ����A����Ϣ
    inline INT_PTR SendTimeOutA(UINT message, WPARAM wParam, LPARAM lParam) const {
        if (!m_hWnd)return 0;
        DWORD_PTR ret = 0;
        SendMessageTimeoutA(m_hWnd, message, wParam, lParam,
            SMTO_ABORTIFHUNG | SMTO_NOTIMEOUTIFNOTHUNG | SMTO_ERRORONEXIT,
            1000, &ret);
        return (INT_PTR)ret;
    }

    // Ͷ��W����Ϣ
    inline bool Post(UINT message, WPARAM wParam, LPARAM lParam) const {
        if (!m_hWnd)return 0;
        return ::PostMessageW(m_hWnd, message, wParam, lParam);
    }

    // Ͷ��A����Ϣ
    inline bool PostA(UINT message, WPARAM wParam, LPARAM lParam) const {
        if (!m_hWnd)return 0;
        return ::PostMessageA(m_hWnd, message, wParam, lParam);
    }

    // ��ʾ����
    inline bool show() const {
        if (!m_hWnd)return false;
        return SetVisible(true);
    }

    // ���ش���
    inline bool hide() const {
        if (!m_hWnd)return false;
        return SetVisible(false);
    }

    // �ô��ڿ���״̬
    inline bool SetVisible(bool isVisible) const {
        if (!m_hWnd)return false;
        return ::ShowWindow(m_hWnd, isVisible ? SW_SHOW : SW_HIDE);
    }

    // ȡ���ڿ���״̬
    inline bool GetVisible() const {
        if (!m_hWnd)return false;
        return ::IsWindowVisible(m_hWnd);
    }

    // ȡ���ڿ���״̬
    inline bool IsVisible() const {
        if (!m_hWnd)return false;
        return ::IsWindowVisible(m_hWnd);
    }

    // �ô�������״̬, true = ����, false = ��ֹ
    inline bool SetEnabled(bool isEnable) const {
        if (!m_hWnd)return false;
        return ::EnableWindow(m_hWnd, isEnable);
    }

    // ȡ��������״̬, true = ����, false = ��ֹ
    inline bool GetEnabled() const {
        if (!m_hWnd)return false;
        return ::IsWindowEnabled(m_hWnd);
    }

    // ���ô�����ʽ
    inline DWORD SetStyle(DWORD style, bool isEx) const {
        if (!m_hWnd)return 0;
        return (DWORD)SetWindowLongPtrW(m_hWnd, isEx ? GWL_EXSTYLE : GWL_STYLE, style);
    }

    // ȡ������ʽ
    inline DWORD GetStyle(bool isEx) const {
        if (!m_hWnd)return 0;
        return (DWORD)::GetWindowLongPtrW(m_hWnd, isEx ? GWL_EXSTYLE : GWL_STYLE);
    }

    // ��Ӵ�����ʽ, ��ԭ�е���ʽ������һ����ʽ
    inline DWORD InsterStyle(DWORD style, bool isEx) const {
        if (!m_hWnd)return 0;
        return (DWORD)::SetWindowLongPtrW(m_hWnd, isEx ? GWL_EXSTYLE : GWL_STYLE, GetStyle(isEx) | style);
    }

    // ȥ��������ʽ, ��ԭ�е���ʽ��ȥ��һ����ʽ
    inline DWORD RemoveStyle(DWORD style, bool isEx) const {
        if (!m_hWnd)return 0;
        return (DWORD)::SetWindowLongPtrW(m_hWnd, isEx ? GWL_EXSTYLE : GWL_STYLE, (GetStyle(isEx) & ~style));
    }

    // �����Ƿ����ָ����ʽ
    inline bool IsStyle(DWORD style, bool isEx) const {
        if (!m_hWnd)return false;
        return (GetStyle(isEx) & style) == style;
    }

//#pragma push_macro("SetProp")
//#pragma push_macro("GetProp")
//#pragma push_macro("RemoveProp")
//#undef SetProp
//#undef GetProp
//#undef RemoveProp
    // ���ø�������
    inline bool SetProp(LPCWSTR name, void* pData) const {
        if (!m_hWnd)return false;
        return ::SetPropW(m_hWnd, name, (HANDLE)pData);
    }

    // ��ȡ��������
    inline void* GetProp(LPCWSTR name) const {
        if (!m_hWnd)return 0;
        return ::GetPropW(m_hWnd, name);
    }

    // �Ƴ���������
    inline void* RemoveProp(LPCWSTR name) const {
        if (!m_hWnd)return 0;
        return ::RemovePropW(m_hWnd, name);
    }
//#pragma pop_macro("SetProp")
//#pragma pop_macro("GetProp")
//#pragma pop_macro("RemoveProp")
    
    // ȡ�����б�, ���������Ա��, ��������ָ���������, ��ʹ��ʱ��Ҫ���� FreeProps() �ͷŲο���ȥ�Ĳ���
    inline int GetProps(PPROPSTRUCT* propAry) const {
        return GetProps(m_hWnd, propAry);
    }
    inline static void FreeProps(PPROPSTRUCT propAry) {
        if (propAry)
            delete[] propAry;
    }
    // ȡ�����б�, ���������Ա��, ��������ָ���������, ��ʹ��ʱ��Ҫ���� FreeProps() �ͷŲο���ȥ�Ĳ���
    inline static int GetProps(HWND hWnd, PPROPSTRUCT* propAry) {
        if (!hWnd)return 0;
        ENUMPROPS_PROPDATA data = { 0 };
        if (propAry)
        {
            *propAry = 0;
            data.ary = new PROPSTRUCT[50];
            data.length = 0;
            data.bufLen = 50;
        }
        if (EnumPropsExW(hWnd, EnumProp_Proc, (LPARAM)&data) == -1)
        {
            if (data.ary) delete[] data.ary;
            return 0;
        }
        if (propAry)    // �����յ�ʱ�򲻻���������ڴ�
            *propAry = data.ary;
        return data.length;
    }
private:

    struct ENUMPROPS_PROPDATA
    {
        LPPROPSTRUCT ary;
        DWORD length;
        DWORD bufLen;
    };
    
    // ����TRUE�Լ��������б�ö��, ����FALSE��ֹͣ�����б�ö��
    static BOOL CALLBACK EnumProp_Proc(HWND hWnd, LPWSTR name, HANDLE value, ULONG_PTR param)
    {
        ENUMPROPS_PROPDATA* ptr = (ENUMPROPS_PROPDATA*)param;
        if ( !ptr )return FALSE;
        if ( !ptr->ary )
        {
            ptr->length++;
            return TRUE;
        }
        const DWORD index = ptr->length;
        LPPROPSTRUCT pItem = 0;
        if ( index >= ptr->bufLen )
        {
            const DWORD newSize = index * 2;
            LPPROPSTRUCT newData = new PROPSTRUCT[newSize];
            for ( DWORD i = 0; i < ptr->length; i++ )
                newData[i].move(ptr->ary[i]);
            delete[] ptr->ary;
            ptr->ary = newData;
            ptr->bufLen = newSize;
        }

        ptr->length++;
        PROPSTRUCT& item = ptr->ary[index];
        if ( (LONG_PTR)name <= 0xffff )
        {
            // ��ԭ��, �����ı�, ��Ҫת���ı�
            wstr text = wstr::GetFormat(L" ATOM:%d", (int)(LONG_PTR)name);
            item = text.c_str();
        }
        else
        {
            item = name;
        }
        item.value = value;
        return TRUE;
    }
public:

    // �������ָ��
    inline LONG_PTR SetCursor(HCURSOR hCursor) const {
        if (!m_hWnd)return 0;
        return ::SetClassLongPtrW(m_hWnd, GCLP_HCURSOR, (LONG_PTR)hCursor);
    }

    // ȡ���ָ��
    inline HCURSOR GetCursor() const {
        if (!m_hWnd)return 0;
        return (HCURSOR)::GetClassLongPtrW(m_hWnd, GCLP_HCURSOR);
    }

    // �������, ����֮ǰ�Ĵ��ھ��
    inline HWND SetCapture() const {
        if (!m_hWnd)return 0;
        return ::SetCapture(m_hWnd);
    }

    // �ͷŲ������
    inline bool ReleaseCapture() const {
        return ::ReleaseCapture();
    }

    // ���ø�����
    inline bool SetParent(HWND hWndParent) const {
        if (!m_hWnd)return 0;
        return ::SetParent(m_hWnd, hWndParent);
    }

    // ȡW�洰�ڱ���
    inline wstr GetWindowTextW() const {
        if (!m_hWnd)
            return wstr();
        int len = GetWindowTextLengthW(m_hWnd) + 1;
        wstr ret(len);
        ::GetWindowTextW(m_hWnd, ret.data(), len + 1);
        ret.resize(wcslen(ret.c_str()));
        return ret;
    }

    // ȡA�洰�ڱ���
    inline _str GetWindowTextA() const {
        if (!m_hWnd)return _str();
        int len = GetWindowTextLengthA(m_hWnd) + 1;
        _str ret(len);
        ::GetWindowTextA(m_hWnd, ret.data(), len + 1);
        ret.resize(strlen(ret.c_str()));
        return ret;
    }

    // ��W�洰�ڱ���
    inline bool SetWindowTextW(LPCWSTR lpString) const {
        if (!m_hWnd)return false;
        return ::SetWindowTextW(m_hWnd, lpString);
    }

    // ��A�洰�ڱ���
    inline bool SetWindowTextA(LPCSTR lpString) const {
        if (!m_hWnd)return false;
        return ::SetWindowTextA(m_hWnd, lpString);
    }

    // ��������
    inline void SetFont(HFONT hFont, bool isUpdate = false) const {
        if (!m_hWnd)return;
        ::SendMessageW(m_hWnd, WM_SETFONT, (WPARAM)hFont, isUpdate);
    }

    virtual void SetFont(const wchar_t* name = L"΢���ź�", int size = -12, bool isUpdate = false) const {
        if (!m_hWnd)return;
        Send(WM_SETFONT, (WPARAM)CreateFontW(name, size), isUpdate);
    }

    // ��ȡ����
    inline HFONT GetFont() const {
        if (!m_hWnd)return 0;
        return (HFONT)::SendMessageW(m_hWnd, WM_GETFONT, 0, 0);
    }

    //// �ļ��Ϸ�
    //inline void drag(bool isDrag) {
    //    DragAcceptFiles(m_hWnd, isDrag);
    //}

    // ֧���ڴ�������ǰ����, ���໯����, ������Ϣ, Ϊ0�����, ��Ҫ�������ڷ���������������ڷ��� WndProc
    inline void SubClass(WND_SUBPROC proc = 0) {
        if (!m_data)
        {
            m_data = new PROP_SUBWNDDATA;
            memset(m_data, 0, sizeof(PROP_SUBWNDDATA));
        }

        m_data->subFun = proc;
        m_data->isSubClass = proc != 0;
        if ( !m_data->oldProc && IsWindow() )    // �����Ѵ���, ����û�����໯, �ڲ�����һ�����໯
        {
            // ���໯ȫ���������ڲ�ת����ȥ, �����Ϣ�������ڲ�, �в��ֹ��ܲ���ʵ��
            SetProp(PROP_WNDPROC, m_data);
            m_data->oldProc = (WNDPROC)SetWindowLongPtrW(m_hWnd, GWLP_WNDPROC, (LONG_PTR)CWndBase::CWndBase_DefWndProc);
        }
    }

};

// �ؼ�����, create() ��������һ���Ӵ�����ʽ
class CControlBase : public CWndBase
{
public:
    CControlBase() :CWndBase() { ; }
    virtual ~CControlBase() { ; }
public:
    virtual HWND create(DWORD dwExStyle, LPCWSTR lpszTitle, DWORD dwStyle,
        int x, int y, int nWidth, int nHeight, HWND hWndParent, LONG_PTR id = 0, LPVOID lpParam = 0) {
        return CWndBase::create(dwExStyle, lpszTitle, dwStyle | WS_CHILD, x, y, nWidth, nHeight, hWndParent, id, lpParam);
    }
};

