// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "pch.h"
#include <cstdio>
#include <stdlib.h>
#include <iostream>
#include <algorithm>
#include <ctime>
#include <detours.h>
#include <windows.h>
#include "framework.h"
#include <stdarg.h>
#include <wchar.h>
#include <sstream>
#include <string>
#include <mutex>
#include <vector>
//打开保存文件对话框
#include<Commdlg.h>
//选择文件夹对话框
#include<Shlobj.h>
#include <WinSock2.h>
#pragma comment(lib,"detours.lib")

//#define PrintNoInformation
#define PrintAllInformation
#define PrintMessageBoxW (1)
#define PrintMessageBoxA (1<<1)
#define PrintHeapCreate (1<<2)
#define PrintHeapAlloc (1<<3)
#define PrintHeapDestroy (1<<4)
#define PrintHeapFree (1<<5)
#define PrintCreateFile (1<<6)
#define PrintCloseHandle (1<<7)
#define PrintReadFile (1<<8)
#define PrintWriteFile (1<<9)
#define PrintNowProcessPathInfor (1<<10)
#define PrintAllProcessPathInfor (1<<11)
#define PrintRegCreateKeyEx (1<<12)
#define PrintRegSetValueEx (1<<13)
#define PrintRegCloseKey (1<<14)
#define PrintRegOpenKeyEx (1<<15)
#define PrintRegDeleteValue (1<<16)
#define Printsocket (1<<17)
#define Printbind (1<<18)
#define Printsend (1<<19)
#define Printconnect (1<<20)

#ifdef PrintNoInformation
const long long PrintOption=0;          //通过这个来限制参数输出
#elif defined PrintAllInformation
const long long PrintOption = 0x7fffffffffffffff;
#else 
//const long long PrintOption = PrintMessageBoxW | PrintMessageBoxA | PrintHeapCreate | PrintHeapAlloc | PrintHeapDestroy | PrintHeapFree | PrintCreateFile | PrintCloseHandle | PrintReadFile | PrintWriteFile | PrintHandleInfor | PrintRegCreateKeyEx | PrintRegSetValueEx | PrintRegCloseKey | PrintRegOpenKeyEx | PrintRegDeleteValue | Printsocket | Printbind | Printsend | Printconnect;
const long long PrintOption = PrintHeapAlloc| PrintNowProcessPathInfor;
#endif
/*
#ifdef COMPILING_THE_DLL
#define MY_DLL_EXPORT extern "C" __declspec(dllexport)
#else
#define MY_DLL_EXPORT extern "C" __declspec(dllimport)
#endif
//不知道做什么用的宏定义
*/
#define MY_DLL_EXPORT extern "C" __declspec(dllexport)
#define MY_DLL_IMPORT extern "C" __declspec(dllimport)

#pragma data_seg("MySeg")   //添加共享段
WCHAR seg[1000][256] = {};               //目前不知道干什么用
MY_DLL_EXPORT WCHAR Infor[100000] = {};     //最终需要输出的字符串 加上前缀就可以被引用了
MY_DLL_EXPORT WCHAR ERRORInfor[100000] = {};
MY_DLL_EXPORT int count = 0;
volatile int HeapAllocNum = 0;
WCHAR AvoidProcess[100000][MAX_PATH] = {}, AimProcess[100000][MAX_PATH] = {};
int CntAvoidProcess = 0, CntAimProcess = 0;
std::mutex mtx;      //互斥锁，使得同时只有一个访问共享段，防止出现数据混乱,写入Infor时使用
std::mutex MTXOnProcess;    //互斥锁，使得同时只有一个访问共享段，防止出现数据混乱,写入AvoidProcess/AimProcess时使用
#pragma data_seg()
#pragma comment(linker, "/section:MySeg,RWS")

WCHAR BufferStr[100000];    //中间字符串，利用sprintf格式串来生成
WCHAR ProcessPath[100000];    //存储Path的字符串，专门用来判断当前应用程序的状态（必须，必须不，其他）
std::vector<HANDLE>Created_Heap;    //存储现在还没有释放的堆
std::vector<LPVOID>Heap_Alloc_lp;
std::vector<HANDLE>Created_File;       //存储还没有释放的文件指针
std::vector<LPVOID>Read_File;
int flagg;           //用于函数中判断是否出现异常
FILE* TempOutPath;       //用于输出到文件 默认地址为.\\qwer.txt

MY_DLL_EXPORT void AddToInfor(WCHAR* str){ mtx.lock(); wcscat_s(Infor, str); mtx.unlock();} //注意这里应该用"\n"来换行
MY_DLL_EXPORT void AddToErrorInfor(WCHAR* str) { mtx.lock(); wcscat_s(ERRORInfor, str); mtx.unlock(); }  //注意这里应该用"\r\n"来换行

MY_DLL_EXPORT void AddAvoidProcess(WCHAR* hLocalPath) { MTXOnProcess.lock(); wcscat_s(AvoidProcess[++CntAvoidProcess], hLocalPath);
swprintf(BufferStr, 1000, L"Add New Avoid ProcessPath = > %lS\n", hLocalPath);
AddToInfor(BufferStr); BufferStr[0] = 0;
MTXOnProcess.unlock(); }    //使用一个互斥锁，存储一条新的必须不的应用程序
MY_DLL_EXPORT void AddAimProcess(WCHAR* hLocalPath) { MTXOnProcess.lock(); wcscat_s(AimProcess[++CntAimProcess], hLocalPath);
swprintf(BufferStr, 1000, L"Add New Aim ProcessPath = > %lS\n", hLocalPath);
AddToInfor(BufferStr); BufferStr[0] = 0;
MTXOnProcess.unlock(); }  //使用一个互斥锁，存储一条新的必须的应用程序


MY_DLL_EXPORT DWORD IfDetour(WCHAR* hLocalPath) {
    //Infor[1] = 0; //Infor[0] = (WCHAR )"\n";          //最好Infor的第一个个元素为"\n"（X）

     for (int i = 1; i <= CntAvoidProcess; ++i)            //判断是否为avoid ，注意avoid的优先级高于aim因此如果一个应用程序同时出现在两处，会被判断为Avoid
    {
        if (wcscmp(AvoidProcess[i], hLocalPath) == 0) return 0;      //表示当前进程不能Detour
        if (PrintOption & PrintAllProcessPathInfor)    //如果定义宏就输出全部的Avoid的Path
        {
            swprintf(BufferStr, 1000, L"Avoid Process%d = > %lS\n", i, AvoidProcess[i]);
            AddToInfor(BufferStr); BufferStr[0] = 0;
        }
    }

     for (int i = 1; i <= CntAimProcess; ++i)
    {
        if (wcscmp(AimProcess[i], hLocalPath) == 0) return 1;         //表示当前进程必须要Detour
        if (PrintOption & PrintAllProcessPathInfor)
        {
            swprintf(BufferStr, 1000, L"Aim Process%d = > %lS\n", i, AimProcess[i]);
            AddToInfor(BufferStr); BufferStr[0] = 0;
        }
    }

    if (PrintOption & PrintNowProcessPathInfor) {       //如果定义宏就输出每一次比较时得当前应用程序
        BufferStr[0] = 0;
        swprintf(BufferStr, 10000, L"Now ProcessPath = > %lS\n", hLocalPath);
        AddToInfor(BufferStr); BufferStr[0] = 0;
    }
    return 2;         //表示当前进程可以Detour也可以不Detour
}

MY_DLL_EXPORT BOOL GetNowProcessPath(WCHAR* lpModuleName)    //一个直接获取当前进程Path的函数
{
    if (!GetModuleFileNameW(NULL, lpModuleName, MAX_PATH))
    {
        MessageBoxW(NULL, L"进程名称获取失败", L"TIP", 0);     //注意到此处的messagebox要求本进程必须不能被Detour(注射器程序应该不能被Detour)
        wprintf(L"进程名称获取失败\n");
        return FALSE;
    }
    return TRUE;
}

MY_DLL_EXPORT int PathChange(WCHAR* InputPath,WCHAR* CurrentPath) {           //通过当前路径（当前文件）以及相对路径获得绝对路径，放到CurrentPath里面
    WCHAR tempPath[MAX_PATH] = {0};
    int templenth=0;
    for (int i = wcslen(CurrentPath) - 1; i; i--)          //去掉文件名，获得当前文件夹
        if (CurrentPath[i] == '\\') { CurrentPath[i] = 0; break; }
    for (int i = 0; i <= wcslen(InputPath); ++i)
    {
        if (InputPath[i] == '\\'|| InputPath[i]==0)
        {
            if (templenth == 2 && tempPath[0] == '.' && tempPath[1] == '.')
            {
                for (int i = wcslen(CurrentPath) - 1; i; i--)
                    if (CurrentPath[i] == '\\') { CurrentPath[i] = 0; break; }
            }
            else if (templenth == 1 && tempPath[0] == '.')
            {
                ;
            }
            else {
                swprintf(CurrentPath,1000, L"\\%lS", tempPath);
            }
            templenth = 0; tempPath[0] = 0;
        } else {
            tempPath[templenth++] = InputPath[i]; tempPath[templenth] = 0;
        }
    }
   
    return TRUE;
}

MY_DLL_EXPORT int GetFileName(WCHAR* lpPathName,WCHAR* lpFileName)       //把lpPathName的末尾（最后一个'\\'之后）放到lpFileName
{
    lpFileName[0] = 0;
    for (int i = wcslen(lpPathName) - 1; i; i--)
        if (lpPathName[i] == '\\') { wcscpy_s(lpFileName, MAX_PATH, &lpPathName[i + 1]); break; }
    return wcslen(lpFileName);
}


MY_DLL_EXPORT void PrintInfor(int WillClear)   //供Console程序调用，完成字符串输出,WillClear决定是否清空
{
    if (Infor[0] == 0)return;
    std::wcout << Infor; count++;            //wcout来输出wchar_t型字符串
    if(WillClear) Infor[0] = 0;       
    return;
}

/*
MY_DLL_EXPORT void AddToInfor(WCHAR* str)
{
    mtx.lock();wcscat_s(Infor, str);mtx.unlock();
}
*/
SYSTEMTIME st;
MY_DLL_EXPORT void DLLLogOutput()   //输出日志，主要是当前时间
{
    GetLocalTime(&st); BufferStr[0] = 0;
    swprintf(BufferStr,1000,L"DLL Log Output: %d-%d-%d %02d:%02d:%02d:%03d\n", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
    AddToInfor(BufferStr); BufferStr[0] = 0;
    swprintf(BufferStr, 1000, L"**************************************************\n");
    AddToInfor(BufferStr); BufferStr[0] = 0;
    return;
}


/**************************************************************************************************************
MessageBox相关操作
***************************************************************************************************************/

static int (WINAPI* SysMessageBoxA)(_In_opt_ HWND hWnd, _In_opt_ LPCSTR lpText, _In_opt_ LPCSTR lpCaption, _In_ UINT uType) = MessageBoxA;
//引入需要Hook的函数
MY_DLL_EXPORT int WINAPI NewMessageBoxA(_In_opt_ HWND hWnd, _In_opt_ LPCSTR lpText, _In_opt_ LPCSTR lpCaption, _In_ UINT uType)
{
    /*ProcessPath[0] = 0; GetNowProcessPath(ProcessPath);       //获取当前进程Path并且进行判断的模板
    if (IfDetour(ProcessPath) != 1) { ProcessPath[0] = 0; return SysMessageBoxA(hWnd, lpText, lpCaption, uType);}
    ProcessPath[0] = 0;
    //*/
    if (PrintOption & PrintMessageBoxA)
    {
        swprintf(BufferStr, 1000, L"\n**************************************************\n");
        AddToInfor(BufferStr); BufferStr[0] = 0;
        swprintf(BufferStr, 1000, L"MessageBoxA Hooked\n");
        AddToInfor(BufferStr); BufferStr[0] = 0;
        swprintf(BufferStr, 1000, L"\nParameters:\nhWnd = > %p, \nlpText = > %S, \nlpCaption = > %S, \nuType = > %u\n", hWnd, lpText, lpCaption, uType);
        AddToInfor(BufferStr); BufferStr[0] = 0;
        DLLLogOutput();
    }
    return SysMessageBoxA(NULL, "new MessageBoxA", "Hooked", MB_OK);  //测试用，完成时应该如同正常工作
    //return SysMessageBoxA(hWnd, lpText, lpCaption, uType);  //工作时用
}

static int (WINAPI* SysMessageBoxW)(_In_opt_ HWND hWnd, _In_opt_ LPCWSTR lpText, _In_opt_ LPCWSTR lpCaption, _In_ UINT uType) = MessageBoxW;
//一半会被调用的是本函数（MessageBoxW）
MY_DLL_EXPORT int WINAPI NewMessageBoxW(_In_opt_ HWND hWnd, _In_opt_ LPCWSTR lpText, _In_opt_ LPCWSTR lpCaption, _In_ UINT uType)
{
    /*ProcessPath[0] = 0; GetNowProcessPath(ProcessPath);
    if (IfDetour(ProcessPath) != 1) { ProcessPath[0] = 0; return SysMessageBoxW(hWnd, lpText, lpCaption, uType);}
    ProcessPath[0] = 0;
    //*/
    if (PrintOption & PrintMessageBoxW)
    {
        swprintf(BufferStr, 1000, L"\n**************************************************\n");
        AddToInfor(BufferStr); BufferStr[0] = 0;
        swprintf(BufferStr, 1000, L"MessageBoxW Hooked\n");
        AddToInfor(BufferStr); BufferStr[0] = 0;
        swprintf(BufferStr, 1000, L"\nParameters:\nhWnd = > %p ,\nlpText = > %lS,\nlpCaption = > %lS,\nuType = > %u\n", hWnd, lpText, lpCaption, uType);
        AddToInfor(BufferStr); BufferStr[0] = 0;
        DLLLogOutput();
    }
    return SysMessageBoxW(NULL, L"new MessageBoxW", L"Hooked", MB_OK);//测试用，完成时应该如同正常工作
    //return SysMessageBoxW(hWnd, lpText, lpCaption, uType);  //工作时用
}


/**************************************************************************************************************
堆相关操作
***************************************************************************************************************/

static HANDLE(WINAPI* SysHeapCreate)(DWORD fIOoptions, SIZE_T dwInitialSize, SIZE_T dwMaximumSize) = HeapCreate;
MY_DLL_EXPORT HANDLE WINAPI NewHeapCreate(DWORD fIOoptions, SIZE_T dwInitialSize, SIZE_T dwMaximumSize) 
{ 
    HANDLE ReturnHeapHANDLE = SysHeapCreate(fIOoptions, dwInitialSize, dwMaximumSize);;
    /*ProcessPath[0] = 0; GetNowProcessPath(ProcessPath);
    if (ProcessPath[0] == 0 || IfDetour(ProcessPath) != 1) { ProcessPath[0] = 0;return ReturnHeapHANDLE;}
    ProcessPath[0] = 0;
    //*/
    if (PrintOption & PrintHeapCreate)
    {
        swprintf(BufferStr, 1000, L"\n**************************************************\n");
        AddToInfor(BufferStr); BufferStr[0] = 0;
        swprintf(BufferStr, 1000, L"HeapCreate Hooked\n");
        AddToInfor(BufferStr); BufferStr[0] = 0;
        swprintf(BufferStr, 1000, L"\nParameters:\nfIOoptions = > %u ,\ndwInitialSize = > %u,\ndwMaximumSize = > %u\n", fIOoptions, dwInitialSize, dwMaximumSize);
        AddToInfor(BufferStr); BufferStr[0] = 0;
        swprintf(BufferStr, 1000, L"\nHeap HANDLE = > %p\n", ReturnHeapHANDLE);
        AddToInfor(BufferStr); BufferStr[0] = 0;
        DLLLogOutput();
    }
    Created_Heap.push_back(ReturnHeapHANDLE);
    return ReturnHeapHANDLE;
}

static LPVOID(WINAPI* SysHeapAlloc)(_In_ HANDLE hHeap, _In_ DWORD dwFlags, _In_ SIZE_T dwBytes) = HeapAlloc;
//问题一大堆
MY_DLL_EXPORT LPVOID WINAPI NewHeapAlloc(_In_ HANDLE hHeap,_In_ DWORD dwFlags,_In_ SIZE_T dwBytes)  //未测试
{

    /*ProcessPath[0] = 0; GetNowProcessPath(ProcessPath);
    if (ProcessPath[0] == 0 ||IfDetour(ProcessPath) != 1) { ProcessPath[0] = 0; return SysHeapAlloc(hHeap, dwFlags, dwBytes);}
    ProcessPath[0] = 0; 
    //*/
    flagg = 0;
    for (std::vector<HANDLE>::iterator iter = Created_Heap.begin(); iter != Created_Heap.end(); iter++)
        if (*iter == hHeap) {  flagg = 1; break; }
    if (flagg == 0)
    {
        swprintf(BufferStr, 1000, L"ERROR:\r\n 正在一个没有create过的堆（句柄为%p）中分配空间\r\n", hHeap);
        //***************************************************************************************************************************************
        //此处需要处理一类异常
        AddToErrorInfor(BufferStr); BufferStr[0] = 0;
        return SysHeapAlloc(hHeap, dwFlags, dwBytes);           //可能需要调整
    }

    //为了避免一些进程调用这个函数，只会在指定的进程里进行参数输出和分析

    if (PrintOption & PrintHeapAlloc)
    {
        swprintf(BufferStr, 1000, L"\n**************************************************\n");
        AddToInfor(BufferStr); BufferStr[0] = 0;
        swprintf(BufferStr, 1000, L"HeapAlloc Hooked\n");
        AddToInfor(BufferStr); BufferStr[0] = 0;
        swprintf(BufferStr, 1000, L"\nParameters:\nhHeap = > %p ,\ndwFlags = > %u,\ndwBytes = > %u\n", hHeap, dwFlags, dwBytes);
        AddToInfor(BufferStr); BufferStr[0] = 0;
        DLLLogOutput();
    }
    LPVOID ReturnHeapLPVOID = SysHeapAlloc(hHeap, dwFlags, dwBytes);
    Heap_Alloc_lp.push_back(ReturnHeapLPVOID);
    return ReturnHeapLPVOID;
}


static BOOL(WINAPI* SysHeapDestroy)(HANDLE) = HeapDestroy;
MY_DLL_EXPORT BOOL WINAPI NewHeapDestroy(HANDLE hHeap)
{
    /*
    ProcessPath[0] = 0; GetNowProcessPath(ProcessPath);
    //if (ProcessPath[0] == 0) system("pause");
    if (ProcessPath[0] == 0 || IfDetour(ProcessPath) != 1) { ProcessPath[0] = 0; return SysHeapDestroy(hHeap);}
    ProcessPath[0] = 0; */
    for (std::vector<HANDLE>::iterator iter = Created_Heap.begin(); iter != Created_Heap.end(); iter++)
        if (*iter == hHeap) { Created_Heap.erase(iter); flagg = 1; break; }
    if (flagg == 0)
    {
        swprintf(BufferStr, 1000, L"ERROR:\r\n 正在destroy一个没有create过的堆，句柄为%p\r\n", hHeap);
        AddToErrorInfor(BufferStr); BufferStr[0] = 0;
        //***************************************************************************************************************************************
        //此处需要处理一类异常
    }flagg = 0;
    if (PrintOption & PrintHeapDestroy)
    {
        swprintf(BufferStr, 1000, L"\n**************************************************\n");
        AddToInfor(BufferStr); BufferStr[0] = 0;
        swprintf(BufferStr, 1000, L"HeapDestroy Hooked\n");
        AddToInfor(BufferStr); BufferStr[0] = 0;
        swprintf(BufferStr, 1000, L"\nParameters:\nhHeap = > %p\n", hHeap);
        AddToInfor(BufferStr); BufferStr[0] = 0;
        DLLLogOutput();
    }

    return SysHeapDestroy(hHeap);
}

static BOOL(WINAPI* SysHeapFree)(HANDLE hHeap, DWORD dwFlags, _Frees_ptr_opt_ LPVOID lpMem) = HeapFree;
MY_DLL_EXPORT BOOL WINAPI NewHeapFree(HANDLE hHeap, DWORD dwFlags, _Frees_ptr_opt_ LPVOID lpMem) {
    /*ProcessPath[0] = 0; GetNowProcessPath(ProcessPath);
    if (ProcessPath[0] == 0 || IfDetour(ProcessPath) != 1) { ProcessPath[0] = 0;  return SysHeapFree(hHeap, dwFlags, lpMem);}
    ProcessPath[0] = 0; */
    flagg = 0;
    for (std::vector<HANDLE>::iterator iter = Created_Heap.begin(); iter != Created_Heap.end(); iter++)
        if (*iter == hHeap) { Created_Heap.erase(iter); flagg = 1; break; }
    if (flagg == 0)
    {
        swprintf(BufferStr, 1000, L"ERROR:\r\n 正在一个没有create过的堆（句柄为%p）中释放空间\r\n", hHeap);
        AddToErrorInfor(BufferStr); BufferStr[0] = 0;
        //***************************************************************************************************************************************
        //此处需要处理一类异常，或许需要存储分配的内存的所有指针
       // return SysHeapFree(hHeap, dwFlags, lpMem);          //可能需要调整
    }
    flagg = 0;
    for (std::vector<LPVOID>::iterator iter = Heap_Alloc_lp.begin(); iter != Heap_Alloc_lp.end(); iter++)
        if (*iter == hHeap) { Heap_Alloc_lp.erase(iter); flagg = 1; break; }
    if (flagg == 0)
    {
        swprintf(BufferStr, 1000, L"ERROR:\r\n 正在一个没有alloc过的堆（句柄为%p）中释放空间\r\n", hHeap);
        AddToErrorInfor(BufferStr); BufferStr[0] = 0;
        //***************************************************************************************************************************************
        //此处需要处理一类异常，或许需要存储分配的内存的所有指针
        //return SysHeapFree(hHeap, dwFlags, lpMem);          //可能需要调整
    }
    
    if (PrintOption & PrintHeapFree)
    {
        swprintf(BufferStr, 1000, L"\n**************************************************\n");
        AddToInfor(BufferStr); BufferStr[0] = 0;
        swprintf(BufferStr, 1000, L"HeapFree Hooked\n");
        AddToInfor(BufferStr); BufferStr[0] = 0;
        swprintf(BufferStr, 1000, L"\nParameters:\nhHeap = > %p ,\ndwFlags = > %u,\nlpMem = > %p\n", hHeap, dwFlags, lpMem);
        AddToInfor(BufferStr); BufferStr[0] = 0;
        DLLLogOutput();
    }
    return SysHeapFree(hHeap, dwFlags, lpMem);
}


/**************************************************************************************************************
文件相关操作
***************************************************************************************************************/

MY_DLL_EXPORT int NumberofSubfolders(LPCWSTR lpFileName)         //绝对或者相对,判断当前目录下有多少子文件夹
{
    
    WIN32_FIND_DATA fd;
    HANDLE hFirst;
    int countfolder=0;
    WCHAR tempPath[10000];
    wcscpy_s(tempPath, lpFileName);
    for(int i=wcslen(tempPath);i;i--)
        if (tempPath[i] == L'\\') { tempPath[i] = 0; break; }

    if (lpFileName[0] == '.')       //转绝对路径
    {
        ProcessPath[0] = 0; GetNowProcessPath(ProcessPath);
        PathChange(tempPath, ProcessPath);
        AddToInfor(ProcessPath); //输出绝对路径 
      
        wcscpy_s(tempPath, ProcessPath);
       // wcscat_s(ProcessPath, lpFileName);wcscpy_s((WCHAR*)lpFileName,(rsize_t)1000, ProcessPath); ProcessPath[0] = 0;
    } 
    wcscat_s(tempPath, L"\\*.*");   //表示搜索所有类型文件
    for (hFirst = FindFirstFile(tempPath, &fd); hFirst != INVALID_HANDLE_VALUE && FindNextFile(hFirst, &fd); )
    {
        if (fd.dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY && fd.cFileName[0] != '.')
            countfolder++;
    }
    ProcessPath[0] = 0;
    return countfolder;
}

static HANDLE(WINAPI* SysCreateFile)(
    _In_ LPCWSTR lpFileName,             //文件名
    _In_ DWORD dwDesiredAccess,            //访问模式
    _In_ DWORD dwShareMode,            //共享模式
    _In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes,    //安全属性（也即销毁方式）
    _In_ DWORD dwCreationDisposition,         //how to create
    _In_ DWORD dwFlagsAndAttributes,       //文件属性
    _In_opt_ HANDLE hTemplateFile       //模板文件句柄
    ) = CreateFile;

MY_DLL_EXPORT HANDLE WINAPI NewCreateFile(        //还没有测试
    _In_ LPCWSTR lpFileName,
    _In_ DWORD dwDesiredAccess,
    _In_ DWORD dwShareMode,
    _In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    _In_ DWORD dwCreationDisposition,
    _In_ DWORD dwFlagsAndAttributes,
    _In_opt_ HANDLE hTemplateFile
)
{
    /*ProcessPath[0] = 0; GetNowProcessPath(ProcessPath);
    if (ProcessPath[0] == 0 || IfDetour(ProcessPath) != 1) { ProcessPath[0] = 0;  return SysCreateFile(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);}
    ProcessPath[0] = 0;*/
    HANDLE ReturnFileHANDLE = SysCreateFile(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
    flagg = 0;
    if ((flagg = NumberofSubfolders(lpFileName)) >= 2)
    {
        swprintf(BufferStr, 1000, L"ERROR:\r\n 当前目录下存在%d个子文件夹\r\n", flagg);
        AddToErrorInfor(BufferStr); BufferStr[0] = 0;
        //***************************************************************************************************************************************
        //此处需要处理一类异常，
    }flagg = 0;
    WCHAR FileName[MAX_PATH];
    //getFileName((WCHAR*)lpFileName, FileName);      //似乎不需要
    if ((wcsstr(lpFileName, L".exe") || wcsstr(lpFileName, L".dll") || wcsstr(lpFileName, L".ocx")) && (dwDesiredAccess & GENERIC_WRITE))
    {
        swprintf(BufferStr, 1000, L"ERROR:\r\n 可能正在试图修改可执行程序\r\n");
        AddToErrorInfor(BufferStr); BufferStr[0] = 0;
        //***************************************************************************************************************************************
        //此处需要处理一类异常，
    }
    ProcessPath[0] = 0; GetNowProcessPath(ProcessPath); GetFileName(ProcessPath, FileName); ProcessPath[0] = 0;
    WCHAR* FileNameAddress;
    FileNameAddress = (WCHAR*)wcsstr(lpFileName, FileName);
    if (dwDesiredAccess & GENERIC_READ && FileNameAddress != NULL && (FileNameAddress == lpFileName || *(FileNameAddress - 1) == L'\\'))
    {
        swprintf(BufferStr, 1000, L"ERROR:\r\n 可能正在试图复制自身\r\n");
        AddToErrorInfor(BufferStr); BufferStr[0] = 0;
        //***************************************************************************************************************************************
        //此处需要处理一类异常，
    }
    if (PrintOption & PrintCreateFile)
    {
        swprintf(BufferStr, 1000, L"\n**************************************************\n");
        AddToInfor(BufferStr); BufferStr[0] = 0;
        swprintf(BufferStr, 1000, L"CreateFile Hooked\n");
        AddToInfor(BufferStr); BufferStr[0] = 0;
        swprintf(BufferStr, 1000, L"\nParameters:\nlpFileName = > %lS ,\ndwDesiredAccess = > %u,\ndwShareMode = > %u,\n\dwCreationDisposition = > %u,\ndwFlagsAndAttributes = > %u,\nhTemplateFile = > %p\n", \
            lpFileName, dwDesiredAccess, dwShareMode, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
        AddToInfor(BufferStr); BufferStr[0] = 0;
        if (lpSecurityAttributes != NULL)
        {
            swprintf(BufferStr, 1000, L"lpSecurityAttributes->bInheritHandle = > %u, lpSecurityAttributes->lpSecurityDescriptor = > %p, lpSecurityAttributes->nLength = > %u\n", lpSecurityAttributes->bInheritHandle, lpSecurityAttributes->lpSecurityDescriptor, lpSecurityAttributes->nLength);
            AddToInfor(BufferStr); BufferStr[0] = 0;
        }
        else swprintf(BufferStr, 1000, L"lpSecurityAttributes = > NULL\n");
        AddToInfor(BufferStr); BufferStr[0] = 0;
        DLLLogOutput();
    }
    Created_File.push_back(ReturnFileHANDLE);
    return ReturnFileHANDLE;
}
static BOOL(WINAPI* SysCloseHandle)(HANDLE hObject) = CloseHandle;
MY_DLL_EXPORT BOOL WINAPI NewCloseHandle(HANDLE hObject)
{
    /*ProcessPath[0] = 0; GetNowProcessPath(ProcessPath);
    if (ProcessPath[0] == 0 || IfDetour(ProcessPath) != 1) { ProcessPath[0] = 0;  return SysCloseHandle(hObject);}
    ProcessPath[0] = 0; 
    */
    flagg = 0;
    for (std::vector<HANDLE>::iterator iter = Created_File.begin(); iter != Created_File.end(); iter++)
        if (*iter == hObject) { flagg = 1; break; }
    if (flagg == 0)
    {
        swprintf(BufferStr, 1000, L"ERROR:\r\n 正在关闭一个没有create过的文件句柄（句柄为%p）\r\n", hObject);
        //***************************************************************************************************************************************
        //此处需要处理一类异常
        AddToErrorInfor(BufferStr); BufferStr[0] = 0;
        //return SysCloseHandle(hObject);           //可能需要调整
    }
    flagg = 0;
    if (PrintOption & PrintCloseHandle)
    {
        swprintf(BufferStr, 1000, L"\n**************************************************\n");
        AddToInfor(BufferStr); BufferStr[0] = 0;
        swprintf(BufferStr, 1000, L"CloseHandle Hooked\n");
        AddToInfor(BufferStr); BufferStr[0] = 0;
        swprintf(BufferStr, 1000, L"\nParameters:\nhObject = > %p\n", hObject);
        AddToInfor(BufferStr); BufferStr[0] = 0;
        DLLLogOutput();
    }
    return SysCloseHandle(hObject);
}

static BOOL(WINAPI* SysReadFile)(
    _In_ HANDLE hFile,
    _Out_writes_bytes_to_opt_(nNumberOfBytesToRead, *lpNumberOfBytesRead) __out_data_source(FILE) LPVOID lpBuffer,
    _In_ DWORD nNumberOfBytesToRead,
    _Out_opt_ LPDWORD lpNumberOfBytesRead,
    _Inout_opt_ LPOVERLAPPED lpOverlapped
    ) = ReadFile;
MY_DLL_EXPORT BOOL WINAPI NewReadFile(
    _In_ HANDLE hFile,
    _Out_writes_bytes_to_opt_(nNumberOfBytesToRead, *lpNumberOfBytesRead) __out_data_source(FILE) LPVOID lpBuffer,
    _In_ DWORD nNumberOfBytesToRead,
    _Out_opt_ LPDWORD lpNumberOfBytesRead,
    _Inout_opt_ LPOVERLAPPED lpOverlapped
)
{
    /*ProcessPath[0] = 0; GetNowProcessPath(ProcessPath);
    if (ProcessPath[0] == 0 || IfDetour(ProcessPath) != 1) { ProcessPath[0] = 0;  return SysReadFile(hFile, lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead, lpOverlapped); }    ProcessPath[0] = 0; flagg = 0;
    ProcessPath[0] = 0; */
    flagg = 0;
    for (std::vector<HANDLE>::iterator iter = Created_File.begin(); iter != Created_File.end(); iter++)
        if (*iter == hFile) { flagg = 1; break; }
    if (flagg == 0)
    {
        swprintf(BufferStr, 1000, L"ERROR:\r\n 正在使用一个没有create过的文件句柄（句柄为%p）进行读操作\r\n", hFile);
        //***************************************************************************************************************************************
        //此处需要处理一类异常
        AddToErrorInfor(BufferStr); BufferStr[0] = 0;
        //return SysReadFile(hFile, lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead, lpOverlapped);
    }
    flagg = 0;
    if (PrintOption & PrintReadFile)
    {
        swprintf(BufferStr, 1000, L"\n**************************************************\n");
        AddToInfor(BufferStr); BufferStr[0] = 0;
        swprintf(BufferStr, 1000, L"ReadFile Hooked\n");
        AddToInfor(BufferStr); BufferStr[0] = 0;
        swprintf(BufferStr, 1000, L"\nParameters:\nhFile = > %p\nlpBuffer = > %p\nnNumberOfBytesToRead = > %u\n lpNumberOfBytesRead = > %p, *lpNumberOfBytesRead = > %u\nlpOverlapped = > %p\n", hFile, lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead, *(DWORD *)lpNumberOfBytesRead, lpOverlapped);
        AddToInfor(BufferStr); BufferStr[0] = 0;
        DLLLogOutput();
    }
    Read_File.push_back(lpBuffer);
    return SysReadFile(hFile, lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead, lpOverlapped);
}

static BOOL(WINAPI* SysWriteFile)(
    _In_ HANDLE hFile,
    _In_reads_bytes_opt_(nNumberOfBytesToWrite) LPCVOID lpBuffer,
    _In_ DWORD nNumberOfBytesToWrite,
    _Out_opt_ LPDWORD lpNumberOfBytesWritten,
    _Inout_opt_ LPOVERLAPPED lpOverlapped
    ) = WriteFile;
MY_DLL_EXPORT BOOL WINAPI NewWriteFile(
    _In_ HANDLE hFile,
    _In_reads_bytes_opt_(nNumberOfBytesToWrite) LPCVOID lpBuffer,
    _In_ DWORD nNumberOfBytesToWrite,
    _Out_opt_ LPDWORD lpNumberOfBytesWritten,
    _Inout_opt_ LPOVERLAPPED lpOverlapped
)
{

    /*ProcessPath[0] = 0; GetNowProcessPath(ProcessPath);
    if (ProcessPath[0] == 0 || IfDetour(ProcessPath) != 1) { ProcessPath[0] = 0;  return SysWriteFile(hFile, lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWritten, lpOverlapped);}
    ProcessPath[0] = 0;*/ 
    flagg = 0;
    for (std::vector<HANDLE>::iterator iter = Created_File.begin(); iter != Created_File.end(); iter++)
        if (*iter == hFile) { flagg = 1; break; }
    if (flagg == 0)
    {
        swprintf(BufferStr, 1000, L"ERROR:\r\n 正在使用一个没有create过的文件句柄（句柄为%p）进行写操作\r\n", hFile);
        //***************************************************************************************************************************************
        //此处需要处理一类异常
        AddToErrorInfor(BufferStr); BufferStr[0] = 0;
        //return SysWriteFile(hFile, lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWritten, lpOverlapped);
    }
    flagg = 0;

    if (PrintOption & PrintWriteFile)
    {
        swprintf(BufferStr, 1000, L"\n**************************************************\n");
        AddToInfor(BufferStr); BufferStr[0] = 0;
        swprintf(BufferStr, 1000, L"WriteFile Hooked\n");
        AddToInfor(BufferStr); BufferStr[0] = 0;
        swprintf(BufferStr, 1000, L"\nParameters:\nhFile = > %p\nlpBuffer = > %p\nnNumberOfBytesToWrite = > %u\nlpNumberOfBytesWritten = > %p, *lpNumberOfBytesWritten = > %u\nlpOverlapped = > %p\n", hFile, lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWritten, *(DWORD *)lpNumberOfBytesWritten, lpOverlapped);
        AddToInfor(BufferStr); BufferStr[0] = 0;
        DLLLogOutput();
    }
    return SysWriteFile(hFile, lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWritten, lpOverlapped);
}


/**************************************************************************************************************
注册表相关操作
***************************************************************************************************************/


static LSTATUS(WINAPI* SysRegCreateKeyEx)(
    HKEY                        hKey,
    LPCWSTR                     lpSubKey,
    DWORD                       Reserved,
    LPWSTR                      lpClass,
    DWORD                       dwOptions,
    REGSAM                      samDesired,
    const LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    PHKEY                       phkResult,
    LPDWORD                     lpdwDisposition) = RegCreateKeyEx;
MY_DLL_EXPORT LSTATUS WINAPI NewRegCreateKeyEx(
    HKEY                        hKey,
    LPCWSTR                     lpSubKey,
    DWORD                       Reserved,
    LPWSTR                      lpClass,
    DWORD                       dwOptions,
    REGSAM                      samDesired,
    const LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    PHKEY                       phkResult,
    LPDWORD                     lpdwDisposition
) {

    
    if (PrintOption & PrintRegCreateKeyEx)
    {
        swprintf(BufferStr, 1000, L"\n**************************************************\n");
        AddToInfor(BufferStr); BufferStr[0] = 0;
        swprintf(BufferStr, 1000, L"RegCreateKeyEx Hooked\n");
        AddToInfor(BufferStr); BufferStr[0] = 0;
        swprintf(BufferStr, 1000, L"\nParameters:\nhKey = > %p\nlpSubKey = > %lS\nReserved = > %u\nlpClass = > %lS\ndwOptions = > %d\nsamDesired = > %u\nphkResult = >%p\nlpdwDisposition = > %p,*lpdwDisposition = > %u\n", hKey, lpSubKey, Reserved, lpClass, dwOptions, samDesired, phkResult, lpdwDisposition, *lpdwDisposition);
        AddToInfor(BufferStr); BufferStr[0] = 0;
        if (lpSecurityAttributes != NULL)
        {
            swprintf(BufferStr, 1000, L"lpSecurityAttributes->bInheritHandle = > %u, lpSecurityAttributes->lpSecurityDescriptor = > %p, lpSecurityAttributes->nLength = > %u\n", lpSecurityAttributes->bInheritHandle, lpSecurityAttributes->lpSecurityDescriptor, lpSecurityAttributes->nLength);
            AddToInfor(BufferStr); BufferStr[0] = 0;
        }
        DLLLogOutput();
    }

    LSTATUS ReturnLSTATU=SysRegCreateKeyEx(hKey, lpSubKey, Reserved, lpClass, dwOptions, samDesired, lpSecurityAttributes, phkResult, lpdwDisposition);
    //if (*lpdwDisposition == REG_CREATED_NEW_KEY && ERROR_SUCCESS== ReturnLSTATU)
    if(1)
    {
        swprintf(BufferStr, 1000, L"ERROR:\r\n 正在创建一个新的注册表项\r\n");
        //***************************************************************************************************************************************
        //此处需要处理一类异常
        AddToErrorInfor(BufferStr); BufferStr[0] = 0;
        if (wcsstr(lpSubKey, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run"))
        {
            swprintf(BufferStr, 1000, L"ERROR:\r\n 正在创建一个自启动注册表项\r\n");
            //***************************************************************************************************************************************
            //此处需要处理一类异常
            AddToErrorInfor(BufferStr); BufferStr[0] = 0;
        }
    }

    return ReturnLSTATU;
}

static LSTATUS(WINAPI* SysRegSetValueEx)(
    HKEY       hKey,
    LPCWSTR    lpValueName,
    DWORD      Reserved,
    DWORD      dwType,
    const BYTE* lpData,
    DWORD      cbData
    ) = RegSetValueEx;
MY_DLL_EXPORT LSTATUS WINAPI NewRegSetValueEx(
    HKEY       hKey,
    LPCWSTR    lpValueName,
    DWORD      Reserved,
    DWORD      dwType,
    const BYTE * lpData,
    DWORD      cbData)
{
    LSTATUS ReturnLSTATU = SysRegSetValueEx(hKey, lpValueName, Reserved, dwType, lpData, cbData);
    if ( ERROR_SUCCESS == ReturnLSTATU)
    {
        swprintf(BufferStr, 1000, L"ERROR:\r\n 正在修改一个注册表项\r\n");
        //***************************************************************************************************************************************
        //此处需要处理一类异常
        AddToErrorInfor(BufferStr); BufferStr[0] = 0;
    }

    if (PrintOption & PrintRegSetValueEx)
    {
        swprintf(BufferStr, 1000, L"\n**************************************************\n");
        AddToInfor(BufferStr); BufferStr[0] = 0;
        swprintf(BufferStr, 1000, L"RegSetValueEx Hooked\n");
        AddToInfor(BufferStr); BufferStr[0] = 0;
        swprintf(BufferStr, 1000, L"\nParameters:\nhKey = > %p\nlpValueName = > %lS\nReserved = > %u\ndwType = > %u\nlpData = > %p,*lpData = > %c\ncbData = > %u\n", hKey, lpValueName, Reserved, dwType, lpData, *lpData, cbData);
        AddToInfor(BufferStr); BufferStr[0] = 0;
        DLLLogOutput();
    }
    return ReturnLSTATU;
}

static LSTATUS(WINAPI* SysRegCloseKey)(HKEY hKey) = RegCloseKey;
MY_DLL_EXPORT LSTATUS WINAPI NewRegCloseKey(HKEY hKey)
{
    if (PrintOption & PrintRegCloseKey)
    {
        swprintf(BufferStr, 1000, L"\n**************************************************\n");
        AddToInfor(BufferStr); BufferStr[0] = 0;
        swprintf(BufferStr, 1000, L"RegCloseKey Hooked\n");
        AddToInfor(BufferStr); BufferStr[0] = 0;
        swprintf(BufferStr, 1000, L"\nParameters:\nhKey = > %p\n", hKey);
        AddToInfor(BufferStr); BufferStr[0] = 0;
        DLLLogOutput();
    }
    return SysRegCloseKey(hKey);
}

static LSTATUS(WINAPI* SysRegOpenKeyEx)(           //注意当打开不成功的时候不会新建新的注册表项
    HKEY    hKey,
    LPCWSTR lpSubKey,
    DWORD   ulOptions,
    REGSAM  samDesired,
    PHKEY   phkResult
    ) = RegOpenKeyEx;
MY_DLL_EXPORT LSTATUS WINAPI NewRegOpenKeyEx( 
    HKEY    hKey,
    LPCWSTR lpSubKey,
    DWORD   ulOptions,
    REGSAM  samDesired,
    PHKEY   phkResult)
{
    if (PrintOption & PrintRegOpenKeyEx)
    {
        swprintf(BufferStr, 1000, L"\n**************************************************\n");
        AddToInfor(BufferStr); BufferStr[0] = 0;
        swprintf(BufferStr, 1000, L"RegOpenKeyEx Hooked\n");
        AddToInfor(BufferStr); BufferStr[0] = 0;
        swprintf(BufferStr, 1000, L"\nParameters:\nhKey = > %p\nlpSubKey = > %lS\nulOptions = > %u\nsamDesired = > %u\nphkResult = >%p\n", hKey, lpSubKey, ulOptions,samDesired, phkResult);
        AddToInfor(BufferStr); BufferStr[0] = 0;
        DLLLogOutput();
    }
    return SysRegOpenKeyEx(hKey, lpSubKey, ulOptions, samDesired, phkResult);
}

static LSTATUS(WINAPI* SysRegDeleteValue)(
    HKEY    hKey,
    LPCWSTR lpValueName
    ) = RegDeleteValue;

MY_DLL_EXPORT LSTATUS WINAPI NewRegDeleteValue(
    HKEY    hKey,
    LPCWSTR lpValueName)
{
    LSTATUS ReturnLSTATU = SysRegDeleteValue(hKey, lpValueName);
    if (ERROR_SUCCESS == ReturnLSTATU)
    {
        swprintf(BufferStr, 1000, L"ERROR:\r\n 正在删除一个注册表项\r\n");
        //***************************************************************************************************************************************
        //此处需要处理一类异常
        AddToErrorInfor(BufferStr); BufferStr[0] = 0;
    }
    if (PrintOption & PrintRegDeleteValue)
    {
        swprintf(BufferStr, 1000, L"\n**************************************************\n");
        AddToInfor(BufferStr); BufferStr[0] = 0;
        swprintf(BufferStr, 1000, L"RegDeleteValue Hooked\n");
        AddToInfor(BufferStr); BufferStr[0] = 0;
        swprintf(BufferStr, 1000, L"\nParameters:\nhKey = > %p\nlpValueName = > %lS\n", hKey, lpValueName);
        AddToInfor(BufferStr); BufferStr[0] = 0;
        DLLLogOutput();
    }
    return ReturnLSTATU;
}



/**************************************************************************************************************
网络通信相关操作
***************************************************************************************************************/

static SOCKET(WINAPI* Syssocket)(
    int af,
    int type,
    int protocol
    ) = socket;
MY_DLL_EXPORT SOCKET WINAPI Newsocket(int af, int type, int protocol) {

    if (protocol == IPPROTO_TCP)
    {
        swprintf(BufferStr, 1000, L"SOCKET:\r\n 正在使用TCP传输协议\r\n");
        //***************************************************************************************************************************************
        //此处需要处理一类异常
        AddToErrorInfor(BufferStr); BufferStr[0] = 0;
    } else  if (protocol == IPPROTO_UDP)
    {
        swprintf(BufferStr, 1000, L"SOCKET:\r\n 正在使用UDP传输协议\r\n");
        //***************************************************************************************************************************************
        //此处需要处理一类异常
        AddToErrorInfor(BufferStr); BufferStr[0] = 0;
    }
    else  if (protocol == IPPROTO_SCTP)
    {
        swprintf(BufferStr, 1000, L"SOCKET:\r\n 正在使用STCP传输协议\r\n");
        //***************************************************************************************************************************************
        //此处需要处理一类异常
        AddToErrorInfor(BufferStr); BufferStr[0] = 0;
    }

    if (PrintOption & Printsocket)
    {
        swprintf(BufferStr, 1000, L"\n**************************************************\n");
        AddToInfor(BufferStr); BufferStr[0] = 0;
        swprintf(BufferStr, 1000, L"socket Hooked\n");
        AddToInfor(BufferStr); BufferStr[0] = 0;
        swprintf(BufferStr, 1000, L"\nParameters:\naf = > %d\naf = > %d\nprotocol = > %d\n",af,type,protocol);
        AddToInfor(BufferStr); BufferStr[0] = 0;
        DLLLogOutput();
    }
    return Syssocket(af, type, protocol);
}

static int (WINAPI* Sysbind)(
    SOCKET         s,
    const sockaddr* name,
    int            namelen
    ) = bind;
MY_DLL_EXPORT int WINAPI Newbind(SOCKET s, const sockaddr * name, int namelen) {

    if (sizeof(*name) == sizeof(sockaddr_in))
    {
        swprintf(BufferStr, 1000, L"SOCKET:\r\n 端口为%d,Addr 为%d\r\n", ((sockaddr_in* )name)->sin_port, ((sockaddr_in*)name)->sin_addr.s_addr);
        //***************************************************************************************************************************************
        //此处需要处理一类异常
        AddToErrorInfor(BufferStr); BufferStr[0] = 0;
    }
    else  
    {
        //swprintf(BufferStr, 1000, L"SOCKET:\r\n 端口为%d,Addr 为%d\r\n", ((sockaddr_in6*)name)->sin_port, ((sockaddr_in6*)name)->sin_addr.s_addr);
        swprintf(BufferStr, 1000, L"SOCKET:not use ipv4");
        //***************************************************************************************************************************************
        //此处需要处理一类异常
        AddToErrorInfor(BufferStr); BufferStr[0] = 0;
    }
    //name->sa_data
    if (PrintOption & Printbind)
    {
        swprintf(BufferStr, 1000, L"\n**************************************************\n");
        AddToInfor(BufferStr); BufferStr[0] = 0;
        swprintf(BufferStr, 1000, L"bind Hooked\n");
        AddToInfor(BufferStr); BufferStr[0] = 0;
        swprintf(BufferStr, 1000, L"\nParameters:\ns = > %d\nname = > %p\nnamelen = > %d\n", s, name, namelen);
        AddToInfor(BufferStr); BufferStr[0] = 0;
        DLLLogOutput();
    }
    return Sysbind(s, name, namelen);
}

static int (WINAPI* Syssend)(
    SOCKET     s,
    const char* buf,
    int        len,
    int        flags
    ) = send;
MY_DLL_EXPORT int WINAPI Newsend(SOCKET s, const char* buf, int len, int flags) {
    flagg = 0;
    for (std::vector<LPVOID>::iterator iter = Read_File.begin(); iter != Read_File.end(); iter++)
        if (buf == *iter) { flagg = 1; break; }
    if (flagg == 0)
    {
        swprintf(BufferStr, 1000, L"ERROR:\r\n 正在向网络发送从文件中读取的信息\r\n");
        //***************************************************************************************************************************************
        //此处需要处理一类异常
        AddToErrorInfor(BufferStr); BufferStr[0] = 0;
        //return SysReadFile(hFile, lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead, lpOverlapped);
    }
    if (PrintOption & Printsend)
    {
        swprintf(BufferStr, 1000, L"\n**************************************************\n");
        AddToInfor(BufferStr); BufferStr[0] = 0;
        swprintf(BufferStr, 1000, L"send Hooked\n");
        AddToInfor(BufferStr); BufferStr[0] = 0;
        swprintf(BufferStr, 1000, L"\nParameters:\ns = > %d\nbuf = > %s\nlen = > %d\nflags = > %d\n", s, buf, len, flags);
        AddToInfor(BufferStr); BufferStr[0] = 0;
        DLLLogOutput();
    }

    return Syssend(s, buf, len, flags);
}

static int (WINAPI* Sysconnect)(
    SOCKET         s,
    const sockaddr* name,
    int            namelen
    ) = connect;
MY_DLL_EXPORT int WINAPI Newconnect(SOCKET s, const sockaddr * name, int namelen) {
    if (PrintOption & Printsend)
    {
        swprintf(BufferStr, 1000, L"\n**************************************************\n");
        AddToInfor(BufferStr); BufferStr[0] = 0;
        swprintf(BufferStr, 1000, L"connect Hooked\n");
        AddToInfor(BufferStr); BufferStr[0] = 0;
        swprintf(BufferStr, 1000, L"\nParameters:\ns = > %d\nname = > %p\nnamelen = > %d\n", s, name, namelen);
        AddToInfor(BufferStr); BufferStr[0] = 0;
        DLLLogOutput();
    }
    return Sysconnect(s, name, namelen);
}

/**************************************************************************************************************
内存拷贝监测与关联分析
***************************************************************************************************************/
//暂时不能用   去掉stdcall？
/*
typedef  VOID(* __stdcall  RelMemoryOption)(VOID UNALIGNED* Destination, const VOID UNALIGNED* Source, SIZE_T Length);
static VOID (*WINAPI SysRtlMoveMemory)(
    VOID UNALIGNED* Destination,
    const VOID UNALIGNED* Source,
    SIZE_T Length
);
MY_DLL_EXPORT VOID NewRtlMoveMemory(
    VOID UNALIGNED* Destination,
    const VOID UNALIGNED* Source,
    SIZE_T Length
) {
    if (PrintOption & Printsocket)
    {
        swprintf(BufferStr, 1000, L"\n**************************************************\n");
        AddToInfor(BufferStr); BufferStr[0] = 0;
        swprintf(BufferStr, 1000, L"MoveMemory Hooked\n");
        AddToInfor(BufferStr); BufferStr[0] = 0;
        swprintf(BufferStr, 1000, L"\nParameters:\nDestination = > %p\nSource = > %p\nLength = > %d\n", Destination, Source, Length);
        AddToInfor(BufferStr); BufferStr[0] = 0;
        DLLLogOutput();
    }
    return SysRtlMoveMemory(Destination, Source, Length);
}

static VOID(*WINAPI SysRtlCopyMemory)(
    VOID UNALIGNED* Destination,
    const VOID UNALIGNED* Source,
    SIZE_T Length
    );
MY_DLL_EXPORT VOID NewRtlCopyMemory(
    VOID UNALIGNED* Destination,
    const VOID UNALIGNED* Source,
    SIZE_T Length
) {
    if (PrintOption & Printsocket)
    {
        swprintf(BufferStr, 1000, L"\n**************************************************\n");
        AddToInfor(BufferStr); BufferStr[0] = 0;
        swprintf(BufferStr, 1000, L"CopyMemory Hooked\n");
        AddToInfor(BufferStr); BufferStr[0] = 0;
        swprintf(BufferStr, 1000, L"\nParameters:\nDestination = > %p\nSource = > %p\nLength = > %d\n", Destination, Source, Length);
        AddToInfor(BufferStr); BufferStr[0] = 0;
        DLLLogOutput();
    }
    return SysRtlCopyMemory(Destination, Source, Length);
}
//*/




WCHAR TEMP[1000000];
void Test()
{

}


MY_DLL_EXPORT BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
   // GetNowProcessPath(ProcessPath);       //调试用，全部DLL感染文件全部avoid
   // AddAvoidProcess(ProcessPath);ProcessPath[0] = 0;
    //测试
    fopen_s(&TempOutPath, ".\\qwer.txt", "w");
    if (DetourIsHelperProcess())
    {
        return TRUE;
    }

    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    {
        ProcessPath[0] = 0; GetNowProcessPath(ProcessPath);
        if (IfDetour(ProcessPath) == 0 || wcsstr(ProcessPath, L"\\TestConsole.exe") || wcsstr(ProcessPath, L"\\TestApp.exe")) break;
        if (wcsstr(ProcessPath, L"\\rundll32.exe"))
        {
            BufferStr[0] = 0;
            swprintf(BufferStr, 1000, L"Not a 32-bit program\n");
            AddToInfor(BufferStr); BufferStr[0] = 0;
            break;
        }
        //上面部分在dll加载时就会判断是否需要Detour，代替了函数中的判断内容 ||wcsstr(ProcessPath,L"\\TestApp.exe")||wcsstr(ProcessPath,L"\\rundll32.exe")
        ProcessPath[0] = 0; GetNowProcessPath(ProcessPath);
        swprintf(BufferStr, 1000, L"DLL Inject Success in %lS\n", ProcessPath);
        AddToInfor(BufferStr); BufferStr[0] = 0;
        Test();
        DisableThreadLibraryCalls(hModule);
        DetourTransactionBegin();
        DetourAttach(&(PVOID&)SysMessageBoxW, NewMessageBoxW);
        DetourAttach(&(PVOID&)SysMessageBoxA, NewMessageBoxA);

        DetourAttach(&(PVOID&)SysHeapCreate, NewHeapCreate);
        DetourAttach(&(PVOID&)SysHeapAlloc, NewHeapAlloc);
        DetourAttach(&(PVOID&)SysHeapDestroy, NewHeapDestroy);
        DetourAttach(&(PVOID&)SysHeapFree, NewHeapFree);

        DetourAttach(&(PVOID&)SysCreateFile, NewCreateFile);
        DetourAttach(&(PVOID&)SysCloseHandle, NewCloseHandle);
        DetourAttach(&(PVOID&)SysReadFile, NewReadFile);
        DetourAttach(&(PVOID&)SysWriteFile, NewWriteFile);
        
        DetourAttach(&(PVOID&)SysRegCreateKeyEx, NewRegCreateKeyEx);
        DetourAttach(&(PVOID&)SysRegSetValueEx, NewRegSetValueEx);
        DetourAttach(&(PVOID&)SysRegDeleteValue, NewRegDeleteValue);
        DetourAttach(&(PVOID&)SysRegCloseKey, NewRegCloseKey);
        DetourAttach(&(PVOID&)SysRegOpenKeyEx, NewRegOpenKeyEx);
        
        DetourAttach(&(PVOID&)Syssocket, Newsocket);
        DetourAttach(&(PVOID&)Sysbind, Newbind);
        DetourAttach(&(PVOID&)Syssend, Newsend);
        DetourAttach(&(PVOID&)Sysconnect, Newconnect);

       /* HINSTANCE hMod = LoadLibrary(L"kermel32.dll");
        SysRtlMoveMemory =(RelMemoryOption)GetProcAddress(hMod, "RtlMoveMemory");
        DetourAttach(&(PVOID&)SysRtlMoveMemory, NewRtlMoveMemory);

        
        SysRtlCopyMemory = (RelMemoryOption)GetProcAddress(hMod, "RtlCopyMemory");
        DetourAttach(&(PVOID&)SysRtlCopyMemory, NewRtlCopyMemory);
        //*/
        //*/
        DetourTransactionCommit();
        break;
    } 
    case DLL_THREAD_ATTACH:                 //疑似应该这样，原本在case DLL_PROCESS_ATTACH: 后面
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
    {
        ProcessPath[0] = 0; GetNowProcessPath(ProcessPath);
        if (IfDetour(ProcessPath) == 0 || wcsstr(ProcessPath, L"\\TestConsole.exe") || wcsstr(ProcessPath, L"\\TestApp.exe")) break;
        if (wcsstr(ProcessPath, L"\\rundll32.exe"))
        {
            BufferStr[0] = 0;
            swprintf(BufferStr, 1000, L"Not a 32-bit program\n");
            AddToInfor(BufferStr); BufferStr[0] = 0;
            break;
        }
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourDetach(&(PVOID&)SysMessageBoxW, NewMessageBoxW);
        DetourDetach(&(PVOID&)SysMessageBoxA, NewMessageBoxA);

        DetourDetach(&(PVOID&)SysHeapCreate, NewHeapCreate);
        DetourDetach(&(PVOID&)SysHeapAlloc, NewHeapAlloc);
        DetourDetach(&(PVOID&)SysHeapDestroy, NewHeapDestroy);
        DetourDetach(&(PVOID&)SysHeapFree, NewHeapFree);

        DetourDetach(&(PVOID&)SysCreateFile, NewCreateFile);
        DetourDetach(&(PVOID&)SysCloseHandle, NewCloseHandle);
        DetourDetach(&(PVOID&)SysReadFile, NewReadFile);
        DetourDetach(&(PVOID&)SysWriteFile, NewWriteFile);

        DetourDetach(&(PVOID&)SysRegCreateKeyEx, NewRegCreateKeyEx);
        DetourDetach(&(PVOID&)SysRegSetValueEx, NewRegSetValueEx);
        DetourDetach(&(PVOID&)SysRegDeleteValue, NewRegDeleteValue);
        DetourDetach(&(PVOID&)SysRegCloseKey, NewRegCloseKey);
        DetourDetach(&(PVOID&)SysRegOpenKeyEx, NewRegOpenKeyEx);
        //*/
        
        DetourDetach(&(PVOID&)Syssocket, Newsocket);
        DetourDetach(&(PVOID&)Sysbind, Newbind);
        DetourDetach(&(PVOID&)Syssend, Newsend);
        DetourDetach(&(PVOID&)Sysconnect, Newconnect);

 /*       HMODULE hMod = LoadLibrary(L"kermel32.dll");
        SysRtlMoveMemory = (RelMemoryOption)GetProcAddress(hMod, "RtlMoveMemory");
        DetourDetach(&(PVOID&)SysRtlMoveMemory, NewRtlMoveMemory);

        SysRtlCopyMemory = (RelMemoryOption)GetProcAddress(hMod, "RtlCopyMemory");
        DetourDetach(&(PVOID&)SysRtlCopyMemory, NewRtlCopyMemory);
        //*/
        //*/
        DetourTransactionCommit();
        break;
    }
    }
    return TRUE;
 }

