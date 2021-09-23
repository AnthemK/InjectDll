// TestConsole.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//
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

#ifdef DLL_IMPLEMENT  
#define DLL_API __declspec(dllexport)  
#else  
#define DLL_API __declspec(dllimport)  
#endif  
#define CountdownBeforeExit 3

#pragma comment(lib,"detours.lib")
#pragma comment(lib,"InnjectDll.lib")     //为了引用变量，不能删
//extern "C" DLL_API int count;        //在dll里面引用的变量
//extern "C" DLL_API WCHAR Infor[];
//根据fc老师所说，最好不要引用变量而是用函数来解决问题

WCHAR ProcessPath[100000];    //存储Path的字符串，专门用来判断当前应用程序的状态（必须，必须不，其他）
typedef void(*lpPrintInfor)(int WillClear);
lpPrintInfor PrintInf; //函数指针，输出Infor
typedef void(*AddProcess)(WCHAR*);
AddProcess AddAvoidProc, AddAimProc;  //添加一个路径进Avoid或者Aim
HINSTANCE hDll; //DLL句柄
//extern _export "C"{char* Infor[]; }
int main(int argc, char* argv[])
{
	std::cout << "This program will attach DLL to EXE" << std::endl;
	/*
	std::cout << argv[0] << std::endl;printf("%lS\n",GetCommandLineW());
	if (argc > 1) std::cout << argv[1] << std::endl;
	else std::cout << "argc == 1" << std::endl << std::endl << std::endl;
	//*/

	//WCHAR DirPath[MAX_PATH + 1]=L"C:\\Users\\lenovo\\Desktop\\Working\\SoftwareSecurityExperiment\\InjectDll\\Debug";
	//WCHAR DirPath[MAX_PATH + 1] = L"..\\Debug";
	WCHAR DirPath[MAX_PATH + 1]; GetCurrentDirectoryW(MAX_PATH, DirPath);;
	int LenthDirPath = wcslen(DirPath);
	for (int i = LenthDirPath - 1; i; i--)
		if (DirPath[i] == '\\' || i == 0) { DirPath[i] = 0; break; }
	wcscat_s(DirPath, L"\\InjectDll\\Debug");     //获取WCHAR型的文件夹路径
	printf("DirPath = >%lS\n", DirPath);              


	//char DLLPath[MAX_PATH + 1] = "..\\Debug\\InnjectDll.dll";//Dll的地址   待修改为相对地址      待分析
	//char DLLPath[MAX_PATH + 1] = "C:\\Users\\lenovo\\Desktop\\Working\\SoftwareSecurityExperiment\\InjectDll\\Debug\\InnjectDll.dll";
	char DLLPath[MAX_PATH + 1]; GetCurrentDirectoryA(MAX_PATH, DLLPath);
	int LenthDLLPath = strlen(DLLPath);
	for (int i = LenthDLLPath - 1; i; i--)
		if (DLLPath[i] == '\\' || i == 0) { DLLPath[i] = 0; break; }
	strcat_s(DLLPath, "\\InjectDll\\Debug\\InnjectDll.dll");        //获取char型的dll文件路径
	printf("DLLPath = >%s\n", DLLPath);

	WCHAR EXEPath[MAX_PATH + 1] = { 0 };
	if(argv[0][0]==0||argc<=0) wcscpy_s(EXEPath, MAX_PATH, L"..\\Debug\\TestApp.exe");//需要注入程序的地址  已经为相对地址
	else MultiByteToWideChar(CP_ACP, 0, argv[0], strlen(argv[0]), EXEPath, sizeof(EXEPath));
	//wcscpy_s(EXEPath, MAX_PATH, L"C:\\Users\\lenovo\\Desktop\\Working\\SoftwareSecurityExperiment\\Virus\\Sample.exe");
	printf("EXEPath = >%lS\n", EXEPath);

	hDll = LoadLibraryA(DLLPath);  //为了引入函数所加载的动态链接库，不能删
	if (hDll == 0)
	{
		printf("Error Load Library\n");Sleep(1000);
		return 0;
	}
	PrintInf = (lpPrintInfor)GetProcAddress(hDll, "PrintInfor"); ProcessPath[0] = 0;
	AddAvoidProc = (AddProcess)GetProcAddress(hDll, "AddAvoidProcess"); AddAimProc = (AddProcess)GetProcAddress(hDll, "AddAimProcess"); //函数指针
	GetModuleFileNameW(NULL, ProcessPath, MAX_PATH);
	AddAvoidProc(ProcessPath); AddAimProc(EXEPath);   //当前文件不应当被Detour，目标文件应当被Detour
	if (wcscmp(ProcessPath, EXEPath) == 0 || EXEPath[0] == 0)
	{
		printf("Aim Program is NULL or this program itself\n"); Sleep(1000);
		return 0;
	}
	ProcessPath[0] = 0;

	STARTUPINFO si;          //开始信息（？）
	PROCESS_INFORMATION pi;   //进程信息
	ZeroMemory(&si, sizeof(STARTUPINFO));
	ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));
	si.cb = sizeof(STARTUPINFO);
	int idxcnt=0;

	if (DetourCreateProcessWithDllEx(EXEPath, NULL, NULL, NULL, TRUE,
		CREATE_DEFAULT_ERROR_MODE | CREATE_SUSPENDED, NULL, DirPath, &si, &pi,DLLPath, NULL))
	{
		//MessageBoxA(NULL, "INJECTED", NULL, NULL);
		ResumeThread(pi.hThread);
		//printf("No\n");
		WaitForSingleObject(pi.hProcess, INFINITE);
		//printf("Yes\n");
		if (hDll != NULL)
		{
			if (PrintInf != NULL)
			{
				std::cout << std::endl;
				PrintInf(0);            //参数表示是否输出后清空
				std::cout << std::endl;
			}
			FreeLibrary(hDll);
		}
		else {
			std::cout << "Can't output Information" << std::endl;
		}
	} else {
		char error[100];
		sprintf_s(error, "%d", GetLastError());
		printf("%s", error);
		//MessageBoxA(NULL, error, NULL, NULL);
	}
	#ifdef CountdownBeforeExit
	std::cout << std::endl;
	for (int i = CountdownBeforeExit; i; i--) printf("%d ", i), Sleep(1000);
	std::cout << std::endl<<"Syringer exit"<<std::endl;
	Sleep(1000);
	#endif

	return 0;
}

// 运行程序: Ctrl + F5 或调试 >“开始执行(不调试)”菜单
// 调试程序: F5 或调试 >“开始调试”菜单

// 入门使用技巧: 
//   1. 使用解决方案资源管理器窗口添加/管理文件
//   2. 使用团队资源管理器窗口连接到源代码管理
//   3. 使用输出窗口查看生成输出和其他消息
//   4. 使用错误列表窗口查看错误
//   5. 转到“项目”>“添加新项”以创建新的代码文件，或转到“项目”>“添加现有项”以将现有代码文件添加到项目
//   6. 将来，若要再次打开此项目，请转到“文件”>“打开”>“项目”并选择 .sln 文件










/*HMODULE hMod = LoadLibrary(L"kermel32.dll");
SysRtlMoveMemory = (RelMemoryOption)GetProcAddress(hMod, "RtlMoveMemory");
DetourDetach(&(PVOID&)SysRtlMoveMemory, NewRtlMoveMemory);

SysRtlCopyMemory = (RelMemoryOption)GetProcAddress(hMod, "RtlCopyMemory");
DetourDetach(&(PVOID&)SysRtlCopyMemory, NewRtlCopyMemory);
//*/
//*/