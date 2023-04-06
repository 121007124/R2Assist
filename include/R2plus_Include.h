#pragma once
#include <windows.h>
#include <stdio.h>
#include "include/zlib/Czlib.h"
#include <vector>

#define __query(_s, _n) (  ((_s) & (_n)) == (_n)  )
#define FILE_FLAG_SUNITEM_COUNT 8       // д���ļ��Ľṹ��Ա��
#define FILE_HEAD_FLAGS MAKELONG(MAKEWORD('K', 'D'), MAKEWORD('F', 'P'))
#define COLOR_TEXT_ERR      RGB(252, 12, 12)
#define COLOR_TEXT_OK       RGB(12, 12, 222)
#define COLOR_TEXT_BACK_ERR RGB(180, 240, 255)
#define COLOR_TEXT_BACK     RGB(220, 250, 255)


#define SERVER_TIME2STR         1       // ʱ��ת�ı�
#define SERVER_STR2TIME         2       // �ı�תʱ��
#define SERVER_FREE             3       // �ͷ��ڴ�
#define SERVER_SONGTOBIN        4       // �赥����ת�ֽڼ�
#define SERVER_TEXT_EN          5       // �ı�����
#define SERVER_TEXT_DE          6       // �ı�����
#define SERVER_FREE_STR         7       // �ͷ��ַ�������, _str
#define SERVER_MAKERETDATA      8       // ���ɷ��ظ��ͻ��˵�����
#define SERVER_PARSELOGIN       9       // �����ͻ��˷��͹���������
#define SERVER_PARSECODE        10      // �����ͻ��˷��͹�������Ϣ
#define SERVER_COMPRESS         11      // ѹ������
#define SERVER_DATABASE2BIN     12      // ���ݿ�����ת�ֽڼ�
#define SERVER_MAKE_RETSONG     13      // ���ɷ��ظ赥�б�

#define SERVER_R2_READINT       30      // ������, ��ƫ��ָ��
#define SERVER_R2_READINT64     31      // ��������, ��ƫ��ָ��
#define SERVER_R2_READSTR       32      // ���ı�, ��ƫ��ָ��
#define SERVER_R2_READDATA      33      // ������, ��ƫ��ָ��

#define SERVER_R2_WRITEINT      40      // д����, ��ƫ��ָ��
#define SERVER_R2_WRITEINT64    41      // д������, ��ƫ��ָ��
#define SERVER_R2_WRITESTR      42      // д�ı�, ��ƫ��ָ��
#define SERVER_R2_WRITEDATA     43      // д����, ��ƫ��ָ��


#define SERVER_LOGIN            65536   // ��¼
#define SERVER_REGIS            65537   // ע��
#define SERVER_BINTOSONG        65538   // �ֽڼ�ת�赥����
#define SERVER_SONGLIST         65539   // ��ȡ�赥�б�
#define SERVER_MAKECODE         65540   // ����������
#define SERVER_UPDATELIST       65541   // ���¸赥�б�
#define SERVER_OFFLINE          65542   // ����
#define SERVER_LOADLIST         65543   // ���ظ赥�б�

#pragma pack (1)

// ��DLLͨѶ�Ľṹ
typedef struct SERVER_ARG
{
    UINT    code;
    LPCSTR  buffer;
    int     bufSize;
    LPVOID  pRet;
    ULONG64 llRet;
}*PSERVER_ARG;

// �赥���ݽṹ
typedef struct R2PLUS_SONG_STRUCT
{
    ULONG64 id;         // id, �����ݿ����ID
    ULONG64 userid;     // ���û������ID
    int     index;      // �����Ա�������������
    int     combo;      // ������
    int     timer;      // ��λ��
    int     level;      // �����Ǽ�
    LPCSTR  songname;   // ������
    LPCSTR  flags;      // Ӧ����100���ֽڵ�һ���ṹ
    LPCSTR  remark;     // ��ע�ı�
    LPCSTR  userName;   // �赥����

}*PR2PLUS_SONG_STRUCT;

// �û����ݽṹ
typedef struct R2PLUS_USER_STRUCT
{
    ULONG64 id;
    LPCSTR  username;
    LPCSTR  pwd;
    LPCSTR  parent;
    int     makecount;
    LPCSTR  table;
    ULONG64 lasttimer;
    int     state;

}*PR2PLUS_USER_STRUCT;

#pragma pack ()



inline LPVOID r2_text_en(PSERVER_ARG pArg)
{
    UCHAR* str = (UCHAR*)pArg->buffer;
    int size = pArg->bufSize;
    LPSTR buf = (LPSTR)pArg->pRet;
    int offset = 0;
    int bufSize = (int)(ULONG)pArg->llRet;
    for ( int i = 0; i < size; i++ )
    {
        UCHAR ch = *str++;
        offset = offset + sprintf_s(buf + offset, 3, "%02X", ch);
    }

    return buf;
}
inline LPVOID r2_text_de(PSERVER_ARG pArg)
{
    LPCSTR str = pArg->buffer;
    int size = pArg->bufSize;
    LPSTR buf = (LPSTR)pArg->pRet;
    int bufSize = (int)(ULONG)pArg->llRet;
    for ( int i = 0; i < size; i += 2 )
    {
        if ( sscanf_s(str + i, "%02X", (int*)buf) )
            buf++;
    }

    return buf;
}


// ���л�, ��ȡ����, ��ȡ��ָ��ָ�����ݵĺ���
inline void r2_read_data(LPBYTE& buf, LPVOID pData, int nSize)
{
    if ( nSize == 0 )
        return;

    memcpy(pData, buf, nSize);
    buf += nSize;
}
// ���л�, ��ȡ4�ֽ�����, ��ȡ��ָ��ָ����ֵ�ĺ���
inline UINT r2_read_int(LPBYTE& buf)
{
    UINT vl;
    r2_read_data(buf, &vl, sizeof(vl));
    return vl;
}
// ���л�, ��ȡ8�ֽ�����, ��ȡ��ָ��ָ����ֵ�ĺ���
inline ULONG64 r2_read_int64(LPBYTE& buf)
{
    ULONG64 vl;
    r2_read_data(buf, &vl, sizeof(vl));
    return vl;
}
// ���л�, ��ȡ�ַ���, ��ȡ��ָ��ָ���ַ����ĺ���
inline LPCSTR r2_read_str(LPBYTE& buf, int* pLen = 0)
{
    LPCSTR pStr = (LPCSTR)buf;
    int len = (int)strlen(pStr);
    buf += len + 1;
    if ( pLen )*pLen = len;
    return pStr;
}
// ���л�, ��ȡ�ַ���, ��ȡ��ָ��ָ���ַ����ĺ���
inline LPCWSTR r2_read_strW(LPBYTE& buf, int* pLen = 0)
{
    LPCWSTR pStr = (LPCWSTR)buf;
    int len = (int)wcslen(pStr) + 1;
    len *= sizeof(wchar_t);
    buf += len;
    if ( pLen )*pLen = len;
    return pStr;
}




// ���л�, д������, д���ָ��ָ�����ݵĺ���
inline void r2_write_data(_str& buf, LPCVOID pData, int nSize)
{
    if ( nSize == 0 )
        return;
    buf.append((LPCSTR)pData, nSize);
}
// ���л�, д��4�ֽ�����, д���ָ��ָ����ֵ�ĺ���
inline void r2_write_int(_str& buf, UINT vl)
{
    r2_write_data(buf, &vl, sizeof(vl));
}
// ���л�, д��8�ֽ�����, д���ָ��ָ����ֵ�ĺ���
inline void r2_write_int64(_str& buf, ULONG64 vl)
{
    r2_write_data(buf, &vl, sizeof(vl));
}
// ���л�, д���ַ���, д���ָ��ָ���ַ����ĺ���
inline void r2_write_str(_str& buf, LPCSTR pStr, int len = 0)
{
    if ( !pStr )pStr = "";
    if ( len == 0 )
        len = (int)strlen(pStr);
    r2_write_data(buf, pStr, len);
    buf.push_back(0);   // д�������־, 1���ֽ�0
}

// ���л�, д���ַ���, д���ָ��ָ���ַ����ĺ���
// len = �ַ���, �����ֽ���
inline void r2_write_strW(_str& buf, LPCWSTR pStr, int len = 0)
{
    if ( !pStr )pStr = L"";
    if ( len == 0 )
        len = (int)wcslen(pStr);
    len *= 2;
    r2_write_data(buf, pStr, len);
    buf.push_back(0);   // д�������־, 2���ֽ�0
    buf.push_back(0);
}

