// ConsoleApplication1.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include <windows.h>
#include <gdiplus.h>
#include <comdef.h>

int main()
{
    typedef LPVOID(WINAPI* PFN_R2Puls_Dll_Interface)( LPVOID pArg );
    HMODULE hModule = LoadLibraryW(L"R2Plus_Dll.dll");
    PFN_R2Puls_Dll_Interface pfn = (PFN_R2Puls_Dll_Interface)GetProcAddress(hModule, "R2Puls_Dll_Interface");
    pfn(0);
    std::cout << "Hello World!\n";
    int a = 0;
    std::cin >> a;
}

