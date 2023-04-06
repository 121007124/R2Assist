#include "pch.h"
//#include "assist.h"


// post到服务器, 返回是否成功, retData接收服务器返回的数据
bool SendServerMessage(LPCVOID pPostData, int nPostSize, _str& retData);

#define _ISDEBUG_URL 0

bool SendServerMessage(LPCVOID pPostData, int nPostSize, _str& retData)
{
    // 这里自己处理吧, 把数据发送到自己服务器上, 自己拼接数据, 这里就不联网了
    return false;
#if _ISDEBUG_URL
    const LPCWSTR url = L"http://127.0.0.1:12101/R2Plus_QQ_121007124";
#else
    const LPCWSTR url = L"http://4cd.cc:12101/R2Plus_QQ_121007124";
#endif

    Czlib zlib; // 需要把post数据压缩后发送
    uLongf compressSize = 0;
    Byte* pData = zlib.compress(pPostData, nPostSize, &compressSize);
    if ( !pData ) return false;

    zlib.free(&pData);
    return false;
}


DWORD CALLBACK ThreadProc_Login(LPVOID _p)
{
    PSERVER_ARG pArg = (PSERVER_ARG)_p;
    _str retHtml;
    Czlib zlib;
    g_token.clear();

    {
        _str buf(260);
        LPCSTR username = (LPCSTR)pArg->buffer;
        LPCSTR password = (LPCSTR)pArg->bufSize;

        static _str tmpName;
        static _str tmpPwd;

        tmpName = username;
        tmpPwd = password;
        g_userName = tmpName.c_str();
        g_password = tmpPwd.c_str();

        r2_write_int(buf, pArg->code);
        r2_write_str(buf, username);
        r2_write_str(buf, password);

        if ( !SendServerMessage(buf.c_str(), (int)buf.size(), retHtml) )
        {
            pArg->code = -1;
            return 0;
        }

        if ( retHtml.empty() )
        {
            pArg->code = 1;
            return 0;
        }
    }

    {
        uLongf bufSize = 0;
        zlib.uncompressEx(retHtml.c_str(), 0, 0, &bufSize);
        if ( bufSize == 0 )
        {
            pArg->code = -3;
            return 0;
        }
        g_token.resize(bufSize);
        Bytef* ptr = (Bytef*)g_token.data();

        if ( !zlib.uncompressEx(retHtml.c_str(), ptr, bufSize, &bufSize) )
        {
            pArg->code = -4;
            return 0;
        }
        g_token.resize(bufSize);
    }

    pArg->code = 0;
    // 解析返回的数据, 登录成功是返回歌单列表, 登录失败是返回空数据, 在前面已经返回了
    // buffer = 服务器返回的数据, 已经解压, 并且解压成功了

    return 0;
}

// 将秒数转换成 xx:xx 这种形式
LPCWSTR timer2str(int t)
{
    int m = t / 60;
    int s = t % 60;
    return g_buf.AddFormat(L"%02d:%02d", m, s);
}
// 将 xx:xx 这种形式转换成秒数
int str2time(LPCWSTR str)
{
    int m = 0, s = 0;
    swscanf_s(str, L"%02d:%02d", &m, &s);
    return ( ( m * 60 ) + s );
}



DWORD CALLBACK ThreadProc_Regis(LPVOID _p)
{
    PSERVER_ARG pArg = (PSERVER_ARG)_p;
    LPCSTR name = (LPCSTR)pArg->buffer;
    LPCSTR pwd = (LPCSTR)pArg->bufSize;
    LPCSTR code = (LPCSTR)pArg->pRet;

    _str buf(260), retHtml;
    r2_write_int(buf, pArg->code);
    r2_write_str(buf, code);
    r2_write_str(buf, name);
    r2_write_str(buf, pwd);

    if ( !SendServerMessage(buf.c_str(), (int)buf.size(), retHtml) )
    {
        pArg->code = -1;
        return 0;
    }
    pArg->code = retHtml.empty() ? -1 : 0;
    pArg->pRet = 0;
    return 0;
}

DWORD CALLBACK ThreadProc_MakeCode(LPVOID _p)
{
    PSERVER_ARG pArg = (PSERVER_ARG)_p;
    LPCSTR name = (LPCSTR)pArg->buffer;

    _str buf(260), retHtml;
    r2_write_int(buf, pArg->code);
    r2_write_str(buf, g_token.c_str());
    r2_write_str(buf, name);

    pArg->pRet = 0;
    if ( !SendServerMessage(buf.c_str(), (int)buf.size(), retHtml) )
    {
        pArg->code = -1;
        return 0;
    }

    if ( retHtml.empty() && retHtml[0] == 128 )
    {
        pArg->code = 128;
        g_token.clear();
        return 0;
    }

    Czlib zlib;
    uLongf uncompressSize = 0;
    pArg->pRet = zlib.uncompress(retHtml.c_str(), &uncompressSize);
    pArg->bufSize = uncompressSize;
    pArg->code = 0;
    return 0;
}

DWORD CALLBACK ThreadProc_UpdateList(LPVOID _p)
{
    _str* pPostData = (_str*)_p;
    _str retHtml;
    if ( !SendServerMessage(pPostData->c_str(), (int)pPostData->size(), retHtml) )
    {
        return -1;
    }
    if ( retHtml.empty() )
        return -2;

    if ( retHtml[0] == 128 )
    {
        g_token.clear();
        return -3;
    }
    PDWORD pRet = (PDWORD)retHtml.c_str();
    return *pRet;
}

DWORD CALLBACK ThreadProc_OffLine(LPVOID _p)
{
    g_token.clear();
    _str retHtml;

    _str buf(260);
    r2_write_int(buf, SERVER_OFFLINE);
    r2_write_str(buf, g_token.c_str());
    if ( !SendServerMessage(buf.c_str(), (int)buf.size(), retHtml) )
    {
        return -1;
    }
    return 0;
}
DWORD CALLBACK ThreadProc_GetSongList(LPVOID _p)
{
    _str* retHtml = (_str*)_p;

    _str buf(260);
    r2_write_int(buf, SERVER_SONGLIST);
    r2_write_str(buf, g_token.c_str());
    if ( !SendServerMessage(buf.c_str(), (int)buf.size(), *retHtml) )
    {
        return -1;
    }

    if ( !retHtml->empty() && retHtml->front() == 128 )
    {
        g_token.clear();
        return 0;
    }

    return 0;
}