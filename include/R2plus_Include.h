#pragma once
#include <windows.h>
#include <stdio.h>
#include "include/zlib/Czlib.h"
#include <vector>

#define __query(_s, _n) (  ((_s) & (_n)) == (_n)  )
#define FILE_FLAG_SUNITEM_COUNT 8       // 写出文件的结构成员数
#define FILE_HEAD_FLAGS MAKELONG(MAKEWORD('K', 'D'), MAKEWORD('F', 'P'))
#define COLOR_TEXT_ERR      RGB(252, 12, 12)
#define COLOR_TEXT_OK       RGB(12, 12, 222)
#define COLOR_TEXT_BACK_ERR RGB(180, 240, 255)
#define COLOR_TEXT_BACK     RGB(220, 250, 255)


#define SERVER_TIME2STR         1       // 时间转文本
#define SERVER_STR2TIME         2       // 文本转时间
#define SERVER_FREE             3       // 释放内存
#define SERVER_SONGTOBIN        4       // 歌单数组转字节集
#define SERVER_TEXT_EN          5       // 文本编码
#define SERVER_TEXT_DE          6       // 文本解码
#define SERVER_FREE_STR         7       // 释放字符串对象, _str
#define SERVER_MAKERETDATA      8       // 生成返回给客户端的数据
#define SERVER_PARSELOGIN       9       // 解析客户端发送过来的数据
#define SERVER_PARSECODE        10      // 解析客户端发送过来的消息
#define SERVER_COMPRESS         11      // 压缩数据
#define SERVER_DATABASE2BIN     12      // 数据库数据转字节集
#define SERVER_MAKE_RETSONG     13      // 生成返回歌单列表

#define SERVER_R2_READINT       30      // 读整数, 并偏移指针
#define SERVER_R2_READINT64     31      // 读长整数, 并偏移指针
#define SERVER_R2_READSTR       32      // 读文本, 并偏移指针
#define SERVER_R2_READDATA      33      // 读数据, 并偏移指针

#define SERVER_R2_WRITEINT      40      // 写整数, 并偏移指针
#define SERVER_R2_WRITEINT64    41      // 写长整数, 并偏移指针
#define SERVER_R2_WRITESTR      42      // 写文本, 并偏移指针
#define SERVER_R2_WRITEDATA     43      // 写数据, 并偏移指针


#define SERVER_LOGIN            65536   // 登录
#define SERVER_REGIS            65537   // 注册
#define SERVER_BINTOSONG        65538   // 字节集转歌单数组
#define SERVER_SONGLIST         65539   // 获取歌单列表
#define SERVER_MAKECODE         65540   // 生成邀请码
#define SERVER_UPDATELIST       65541   // 更新歌单列表
#define SERVER_OFFLINE          65542   // 离线
#define SERVER_LOADLIST         65543   // 加载歌单列表

#pragma pack (1)

// 与DLL通讯的结构
typedef struct SERVER_ARG
{
    UINT    code;
    LPCSTR  buffer;
    int     bufSize;
    LPVOID  pRet;
    ULONG64 llRet;
}*PSERVER_ARG;

// 歌单数据结构
typedef struct R2PLUS_SONG_STRUCT
{
    ULONG64 id;         // id, 在数据库里的ID
    ULONG64 userid;     // 在用户表里的ID
    int     index;      // 这个成员在数组里的索引
    int     combo;      // 连击数
    int     timer;      // 单位秒
    int     level;      // 歌曲星级
    LPCSTR  songname;   // 歌曲名
    LPCSTR  flags;      // 应该是100个字节的一个结构
    LPCSTR  remark;     // 备注文本
    LPCSTR  userName;   // 歌单作者

}*PR2PLUS_SONG_STRUCT;

// 用户数据结构
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


// 序列化, 读取数据, 读取后指针指向数据的后面
inline void r2_read_data(LPBYTE& buf, LPVOID pData, int nSize)
{
    if ( nSize == 0 )
        return;

    memcpy(pData, buf, nSize);
    buf += nSize;
}
// 序列化, 读取4字节整数, 读取后指针指向数值的后面
inline UINT r2_read_int(LPBYTE& buf)
{
    UINT vl;
    r2_read_data(buf, &vl, sizeof(vl));
    return vl;
}
// 序列化, 读取8字节整数, 读取后指针指向数值的后面
inline ULONG64 r2_read_int64(LPBYTE& buf)
{
    ULONG64 vl;
    r2_read_data(buf, &vl, sizeof(vl));
    return vl;
}
// 序列化, 读取字符串, 读取后指针指向字符串的后面
inline LPCSTR r2_read_str(LPBYTE& buf, int* pLen = 0)
{
    LPCSTR pStr = (LPCSTR)buf;
    int len = (int)strlen(pStr);
    buf += len + 1;
    if ( pLen )*pLen = len;
    return pStr;
}
// 序列化, 读取字符串, 读取后指针指向字符串的后面
inline LPCWSTR r2_read_strW(LPBYTE& buf, int* pLen = 0)
{
    LPCWSTR pStr = (LPCWSTR)buf;
    int len = (int)wcslen(pStr) + 1;
    len *= sizeof(wchar_t);
    buf += len;
    if ( pLen )*pLen = len;
    return pStr;
}




// 序列化, 写入数据, 写入后指针指向数据的后面
inline void r2_write_data(_str& buf, LPCVOID pData, int nSize)
{
    if ( nSize == 0 )
        return;
    buf.append((LPCSTR)pData, nSize);
}
// 序列化, 写入4字节整数, 写入后指针指向数值的后面
inline void r2_write_int(_str& buf, UINT vl)
{
    r2_write_data(buf, &vl, sizeof(vl));
}
// 序列化, 写入8字节整数, 写入后指针指向数值的后面
inline void r2_write_int64(_str& buf, ULONG64 vl)
{
    r2_write_data(buf, &vl, sizeof(vl));
}
// 序列化, 写入字符串, 写入后指针指向字符串的后面
inline void r2_write_str(_str& buf, LPCSTR pStr, int len = 0)
{
    if ( !pStr )pStr = "";
    if ( len == 0 )
        len = (int)strlen(pStr);
    r2_write_data(buf, pStr, len);
    buf.push_back(0);   // 写入结束标志, 1个字节0
}

// 序列化, 写入字符串, 写入后指针指向字符串的后面
// len = 字符数, 不是字节数
inline void r2_write_strW(_str& buf, LPCWSTR pStr, int len = 0)
{
    if ( !pStr )pStr = L"";
    if ( len == 0 )
        len = (int)wcslen(pStr);
    len *= 2;
    r2_write_data(buf, pStr, len);
    buf.push_back(0);   // 写入结束标志, 2个字节0
    buf.push_back(0);
}

