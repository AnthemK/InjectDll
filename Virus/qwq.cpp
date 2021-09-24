#define _CRT_SECURE_NO_WARNINGS
#include <cstdio>
#include <iostream>
#include <windows.h>
#pragma comment(lib, "ws2_32.lib")  //加载 ws2_32.dll
using namespace std;

void opt3() {
	HANDLE hHeap = HeapCreate(HEAP_NO_SERIALIZE, 4096 * 10, 4096 * 100);
	int* pArr = (int*)HeapAlloc(hHeap, 0, sizeof(int) * 30);
	HeapFree(hHeap, 0, pArr);
	HeapDestroy(hHeap);
}
void opt4() {
	int fileSize = 0;
	char writeString[100];
	bool flag;
	HANDLE hOpenFile = (HANDLE)CreateFileW(L"a.txt", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, NULL, NULL);
	if (hOpenFile == INVALID_HANDLE_VALUE) {
		hOpenFile = NULL;
		printf("Can not open the file\n");
		return;
	}
	printf("successfully open a file\n");
	printf("input a string:");
	scanf("%s", writeString);
	flag = WriteFile(hOpenFile, writeString, strlen(writeString), NULL, NULL);
	if (flag) {
		printf("successful writed!\n");
	}
	FlushFileBuffers(hOpenFile);
	CloseHandle(hOpenFile);
}
void opt5() {
	CHAR* pBuffer;
	int fileSize = 0;
	bool flag;
	HANDLE hOpenFile = (HANDLE)CreateFileW(L"a.txt", GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_ALWAYS, NULL, NULL);
	if (hOpenFile == INVALID_HANDLE_VALUE)
	{
		hOpenFile = NULL;
		printf("Can not open the file\n");
		return;
	}
	printf("successfully open a file\n");
	fileSize = GetFileSize(hOpenFile, NULL);
	pBuffer = (char*)malloc((fileSize + 1) * sizeof(char));
	flag = ReadFile(hOpenFile, pBuffer, fileSize, NULL, NULL);
	pBuffer[fileSize] = 0;
	if (flag) {
		printf("successfully read a string:%s!\n", pBuffer);
	}
	free(pBuffer);
	CloseHandle(hOpenFile);
}
void opt6() {
	HKEY hKey = NULL;
	TCHAR Data[254];
	memset(Data, 0, sizeof(Data));
	wcsncpy_s(Data, TEXT("My name id awson. "), 254);

	size_t lRet = RegCreateKeyEx(HKEY_CURRENT_USER, (LPWSTR)L"c_new_key", 0, NULL, REG_OPTION_NON_VOLATILE,
		KEY_ALL_ACCESS, NULL, &hKey, NULL);	// 修改注册表键值，没有则创建它
	size_t iLen = wcslen(Data);
	// 设置键值
	lRet = RegSetValueEx(hKey, L"awson's_new_key", 0, REG_SZ, (CONST BYTE*)Data, sizeof(TCHAR) * iLen);
	RegCloseKey(hKey);

	hKey = NULL;
	lRet = RegOpenKeyEx(HKEY_CURRENT_USER, (LPWSTR)L"c_new_key", 0, KEY_ALL_ACCESS, &hKey);
	lRet = RegDeleteKeyEx(hKey, L"c_new_key", KEY_WOW64_32KEY, 0);
	RegCloseKey(hKey);
}
void opt7() {
	char chDesBuffer[1024];
	char chSouBuffer[] = "abcd";

	typedef VOID(__stdcall* myrtlcpy)(PVOID, CONST VOID*, ULONG);
	myrtlcpy pRtlCopy = NULL;

	HINSTANCE hDLL;
	hDLL = LoadLibrary(L"kernel32.dll");
	if (hDLL == NULL)
	{
		cout << "kao" << endl;
	}
	pRtlCopy = (myrtlcpy)GetProcAddress(hDLL, "RtlMoveMemory");
	if (pRtlCopy == NULL)
	{
		cout << "fuck" << endl;
	}
	pRtlCopy(chDesBuffer, chSouBuffer, 4);
	chDesBuffer[4] = 0;
	cout << chDesBuffer << endl;
}
string tras(LPCWSTR pwszSrc) {
	int nLen = WideCharToMultiByte(CP_ACP, 0, pwszSrc, -1, NULL, 0, NULL, NULL);
	char* pszDst = new char[nLen];
	WideCharToMultiByte(CP_ACP, 0, pwszSrc, -1, pszDst, nLen, NULL, NULL);
	pszDst[nLen - 1] = 0;
	string strTemp(pszDst);
	delete[] pszDst;
	return strTemp;
}
void headRepeatedRelease() {

	HANDLE hHeap = HeapCreate(HEAP_NO_SERIALIZE, 4096 * 10, 4096 * 100);

	int* pArr = (int*)HeapAlloc(hHeap, 0, sizeof(int) * 30);

	HeapFree(hHeap, 0, pArr);
	HeapFree(hHeap, 0, pArr);

	HeapDestroy(hHeap);
}
void modifyExProgram() {
	HANDLE hOpenFile = (HANDLE)CreateFile(L"a.exe", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, NULL, NULL);
	CloseHandle(hOpenFile);
}
void selfReplication() {
	//testCode.exe
	HANDLE hOpenFile = (HANDLE)CreateFile(L"qwq.exe", GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_ALWAYS, NULL, NULL);
	CloseHandle(hOpenFile);
}
void modifyStartupRegistry() {
	HKEY hKey = NULL;
	size_t lRet = RegOpenKeyEx(HKEY_CURRENT_USER, (LPWSTR)L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_READ, &hKey);
	RegCloseKey(hKey);
}
void openAnotherFolder() {
	HANDLE hOpenFile = (HANDLE)CreateFile(L".\\testFolder\\a.txt", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, NULL, NULL);
	CloseHandle(hOpenFile);
}
int main() {
	int op = 0;
	while (1) {
		scanf_s("%d", &op);
		switch (op) {
		case 0: break;
		case 1: MessageBoxA(NULL, "testtest!@#$%", "This is a MessageBoxA", MB_OK); break;
		case 2: MessageBoxW(NULL, L"测试测试，。、‘", L"这是一个 MessageBoxW", MB_OK); break;
		case 3: // heap creat alloc free destroy
			opt3(); break;
		case 4: // createfile & writefile @input content
			opt4(); break;
		case 5: // readfile @output content
			opt5(); break;
		case 6: // creat set value & open delete value
			opt6(); break;
		case 7:
			opt7(); break;
		case 10:
			headRepeatedRelease(); break;
		case 11:
			modifyExProgram(); break;
		case 12:
			selfReplication(); break;
		case 13:
			modifyStartupRegistry(); break;
		case 14:
			openAnotherFolder(); break;
		}
		if (op == 0) {
			break;
		}
	}
	return 0;

}
