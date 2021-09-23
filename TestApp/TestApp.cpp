// TestApp.cpp : 定义应用程序的入口点。
//讲解详见  https://blog.csdn.net/tcjiaan/article/details/8497535?spm=1001.2014.3001.5501

#include "framework.h"
#include "TestApp.h"
#include <iostream>
//打开保存文件对话框
#include<Commdlg.h>
//选择文件夹对话框
#include<Shlobj.h>
#include <vector>

#define MAX_LOADSTRING 1000
#ifdef DLL_IMPLEMENT  
#define DLL_API __declspec(dllexport)  
#else  
#define DLL_API __declspec(dllimport)  
#endif  

#pragma comment(lib,"InnjectDll.lib")     //为了引用变量，不能删
extern "C" DLL_API WCHAR Infor[];
extern "C" DLL_API WCHAR ERRORInfor[];
WCHAR* InformationBegin = &Infor[0];
typedef void(*AddProcess)(WCHAR*);
AddProcess AddAvoidProc,AddAimProc; 
typedef void(*lpPrintInfor)(int WillClear);
lpPrintInfor PrintInf; //函数指针

// 全局变量:
HINSTANCE hInst;                                // 当前实例，在一个回调函数被调用的时候，此时的句柄即为实例句柄
int nCmd;
WCHAR szTitle[MAX_LOADSTRING];                  // 标题栏文本
WCHAR szWindowClass[MAX_LOADSTRING];            // 主窗口类名
WCHAR szDemoFunctionInterfaceClass[MAX_LOADSTRING];         // 演示窗口窗口类名
WCHAR szDemoFunctionInterfaceClassTitle[MAX_LOADSTRING];       //演示窗口文本
WCHAR BufferStr[100000];
WCHAR ProcessPath[100000];    //存储Path的字符串，专门用来判断当前应用程序的状态（必须，必须不，其他）
OPENFILENAME ofn = { 0 };   //文件选择的信息
WCHAR StrFilename[MAX_PATH] = { 0 };//用于接收文件名
BOOL bSuccess=0;     //用来判断注射器启动是否成功
STARTUPINFOW startupInfo = { 0 };       //一个用于决定新进程的主窗体如何显示的STARTUPINFO结构体。
PROCESS_INFORMATION  processInformation = { 0 };     //一个用来接收新进程的识别信息的PROCESS_INFORMATION结构体。


// 此代码模块中包含的函数的前向声明:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK    FunctionDemoProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
void GetNewInformationBegin(int ,int);
int GetNumofLine(int );

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,                //引用程序的实例句柄
                     _In_opt_ HINSTANCE hPrevInstance,      //前一个实例
                     _In_ LPWSTR    lpCmdLine,          //命令行参数
                     _In_ int       nCmdShow)      //主窗口的显示方式，用宏来定义
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: 在此处放置代码。

    // 初始化全局字符串
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);      //从与指定模块关联的可执行文件加载字符串资源，并将字符串复制到带有终止空字符的缓冲区中
    LoadStringW(hInstance, IDC_TESTAPP, szWindowClass, MAX_LOADSTRING);   
    wcscpy_s(szDemoFunctionInterfaceClass, L"DemoFunctionInterfaceClass");
    wcscpy_s(szDemoFunctionInterfaceClassTitle,L"Choose a Function");
    MyRegisterClass(hInstance);



    // 执行应用程序初始化:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_TESTAPP));      //？？

    MSG msg;   //MSG是一个消息类，其具体成员参见msdn

    // 主消息循环:
    while (GetMessage(&msg, nullptr, 0, 0))     //第二个参数是选择是否要来自于某个句柄的，后两个则是过滤消息用
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))      //？？
        {
            TranslateMessage(&msg);     //转换按键信息
            DispatchMessage(&msg);   //DispatchMessage函数负责把消息传到WindowProc让我们的代码来处理（实际上是传给系统，系统在给我们指定的回调函数，然后会点函数对于这个消息执行对应的程序）
        }
    }

    return (int) msg.wParam;
}



//
//  函数: MyRegisterClass()
//
//  目标: 注册窗口类。
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;        //扩展版WNDCLASS
    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;           //窗口的类样式
    wcex.lpfnWndProc    = WndProc;              //设置用哪个WindowProc来处理消息，对于这个窗口的消息的回调函数
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;       //当前应用程序的实例句柄
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_TESTAPP));          //图标
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);          //光标
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);       //窗口的背景色
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_TESTAPP);         //菜单的ID
    wcex.lpszClassName  = szWindowClass;           //要向系统注册的类名，不能与系统已有的重复
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));


    WNDCLASSEXW DemoFunctionInterfaceClass;        //扩展版WNDCLASS
    DemoFunctionInterfaceClass.cbSize = sizeof(WNDCLASSEX);
    DemoFunctionInterfaceClass.style = CS_HREDRAW | CS_VREDRAW;           //窗口的类样式
    DemoFunctionInterfaceClass.lpfnWndProc = FunctionDemoProc;              //设置用哪个WindowProc来处理消息，对于这个窗口的消息的回调函数
    DemoFunctionInterfaceClass.cbClsExtra = 0;
    DemoFunctionInterfaceClass.cbWndExtra = 0;
    DemoFunctionInterfaceClass.hInstance = hInstance;       //当前应用程序的实例句柄
    DemoFunctionInterfaceClass.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_TESTAPP));          //图标
    DemoFunctionInterfaceClass.hCursor = LoadCursor(nullptr, IDC_ARROW);          //光标
    DemoFunctionInterfaceClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);       //窗口的背景色
    DemoFunctionInterfaceClass.lpszMenuName = MAKEINTRESOURCEW(IDI_TESTAPP);         //菜单的ID
    DemoFunctionInterfaceClass.lpszClassName = szDemoFunctionInterfaceClass;           //要向系统注册的类名，不能与系统已有的重复
    DemoFunctionInterfaceClass.hIconSm = LoadIcon(DemoFunctionInterfaceClass.hInstance, MAKEINTRESOURCE(IDI_SMALL));
    RegisterClassExW(&DemoFunctionInterfaceClass);

    StrFilename[0] = 0;
    ofn.lStructSize = sizeof(OPENFILENAME);//结构体大小
    ofn.hwndOwner = NULL;//拥有着窗口句柄，为NULL表示对话框是非模态的，实际应用中一般都要有这个句柄
    ofn.lpstrFilter = TEXT("所有文件\0*.*\0C/C++ Flie\0*.cpp;*.c;*.h\0可执行文件\0*.exe*\0\0");//设置过滤
    ofn.nFilterIndex = 1;//过滤器索引
    ofn.lpstrFile = StrFilename;//接收返回的文件名，注意第一个字符需要为NULL
    ofn.nMaxFile = sizeof(StrFilename);//缓冲区长度
    ofn.lpstrInitialDir = L".\\";//初始目录为默认
    ofn.lpstrTitle = TEXT("请选择一个文件");//使用系统默认标题留空即可
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;//文件、目录必须存在，隐藏只读选项

    return RegisterClassExW(&wcex);   //向系统注册一个窗口类，函数返回一个HWND类型的窗口的句柄
    //注意注册一个窗口类的时候，必须要给所有成员都赋上初值
}


HWND DemoFunctionInterface;  //演示窗口
HWND StaticTextWidget;  //静态输出框窗口
HWND MainInterface;   //主窗口
HWND Demoedit;   //演示界面的文本框
HWND ErrorOuput;  //输出错误信息的窗口、
HWND Errordit;   //输出错误信息的文本框
//
//   函数: InitInstance(HINSTANCE, int)
//
//   目标: 保存实例句柄并创建主窗口
//
//   注释:
//
//        在此函数中，我们在全局变量中保存实例句柄并
//        创建和显示主程序窗口。
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{ 
   HMODULE hMod = LoadLibrary(L"C:\\Users\\lenovo\\Desktop\\Working\\SoftwareSecurityExperiment\\InjectDll\\Debug\\InnjectDll.dll");       //最好改成相对地址
   if (hMod == 0) MessageBox(NULL, L"Can't Load Library", L"C:\\Users\\lenovo\\Desktop\\Working\\SoftwareSecurityExperiment\\InjectDll\\Debug\\InnjectDll.dll", 0);
   AddAvoidProc = (AddProcess)GetProcAddress(hMod, "AddAvoidProcess"); AddAimProc = (AddProcess)GetProcAddress(hMod, "AddAimProcess"); //函数指针
   PrintInf = (lpPrintInfor)GetProcAddress(hMod, "PrintInfor");
   GetModuleFileNameW(NULL, ProcessPath, MAX_PATH);
   AddAvoidProc(ProcessPath); ProcessPath[0] = 0;
   /*BufferStr[0] = 0; GetModuleFileNameW(NULL, BufferStr, MAX_PATH);
   AddAimProc(BufferStr);BufferStr[0] = 0;      //当前进程加到Aim中
   //*/

   //HANDLE qwer=HeapCreate(HEAP_CREATE_ENABLE_EXECUTE, 1000, 10000);
   //HeapAlloc(qwer, HEAP_ZERO_MEMORY, 100);
   //测试用

   hInst = hInstance; // 将实例句柄存储在全局变量中
   nCmd = nCmdShow;
   MainInterface = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW|WS_VSCROLL,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);   //使用szWindowClass窗口类创建一个窗口
   StaticTextWidget = CreateWindow(L"static", L"Hello?", WS_CHILD /*子窗口*/ | WS_VISIBLE /*创建时显示*/ | SS_LEFTNOWORDWRAP /*文本居左，不自动换行（有 '\n' 才会换行），超出控件范围的文本将被隐藏。*/ | WS_BORDER /*带边框*/,//| WS_VSCROLL/*带垂直滚动条*/,   //带边框方便调大小，最后记得去掉
       15, 15, 800, 500, MainInterface,/*父窗口句柄*/nullptr,/*为控件指定一个唯一标识符 */ hInst,/*当前程序实例句柄*/NULL);

   HWND hButtonup = CreateWindow(L"Button", L"UP", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
       900, 50, 160, 80, MainInterface, (HMENU)STATIC_BUTTON_UP, hInst, NULL);
   HWND hButtondown = CreateWindow(L"Button", L"DOWN", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
       900, 400, 160, 80, MainInterface, (HMENU)STATIC_BUTTON_DOWN, hInst, NULL);
   HWND hButtonclear = CreateWindow(L"Button", L"CLEAR", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
       900, 225, 160, 80, MainInterface, (HMENU)STATIC_BUTTON_CLEAR, hInst, NULL);
   //主界面上三个按钮

   DemoFunctionInterface = CreateWindow(szDemoFunctionInterfaceClass, szDemoFunctionInterfaceClassTitle, WS_OVERLAPPEDWINDOW,
       375, 350, 600, 200, MainInterface, nullptr, hInst, nullptr);
   //演示界面

   ErrorOuput = CreateWindow(szDemoFunctionInterfaceClass, szDemoFunctionInterfaceClassTitle, WS_OVERLAPPEDWINDOW,
       375, 350, 800, 494, DemoFunctionInterface, nullptr, hInst, nullptr);
   Errordit = CreateWindow(L"edit", L"错误信息输出", WS_CHILD | WS_VISIBLE | WS_BORDER /*边框*/ | ES_AUTOVSCROLL /*垂直滚动*/| ES_MULTILINE | ES_WANTRETURN| ES_MULTILINE | ES_WANTRETURN,
       0 /*x坐标*/, 0 /*y坐标*/, 800 /*宽度*/, 494 /*高度*/, ErrorOuput, nullptr, hInst, NULL);
   //error输出界面            ，输出的换行和进度条问题，以及重复使用问题


   if (!DemoFunctionInterface) return FALSE;
   if (!MainInterface)  return FALSE;

   ShowWindow(MainInterface, nCmd);   //展示窗口，第一个参数为窗口句柄，第二个参数为控制窗口如何显示，此处采用WinMain中传过来的参数
   UpdateWindow(MainInterface);        //似乎可有可无？

   //*/
   return TRUE;
}

void GetNewInformationBegin(int typ,int num)            //typ==0相对模式(nunm<0向上，num>0向下)，typ==1绝对模式
{
    WCHAR* NowPls, * Lastt;
    if (typ == 0&&num<0)
    {
        num *= -1;
        while (num--)
        {
            NowPls = &Infor[0]; Lastt = &Infor[0];
            for (; *NowPls != 0; NowPls++)
            {
                if ((*NowPls == 0 || *NowPls == L'\n') && NowPls < InformationBegin - 1) Lastt = NowPls;
                else if (NowPls >= InformationBegin) break;
            }
            if (*Lastt == (WCHAR)"\n")InformationBegin = Lastt + 1;
            else InformationBegin = Lastt;
        }

    }
    else if (typ == 0&&num>0)
    {
        while (num--)
        {
            NowPls = &Infor[0];
            for (; *NowPls != 0; NowPls++)
            {
                if (*NowPls == 0 && NowPls > InformationBegin) {
                    InformationBegin = NowPls;
                    break;
                }
                else if (*NowPls == L'\n' && NowPls > InformationBegin) {
                    InformationBegin = NowPls + 1;
                    break;
                }
            }
        }
    }
    else if (typ == 1)
    {
            NowPls = &Infor[0]; Lastt = &Infor[0];
            for (; *NowPls != 0 && num>0; NowPls++)
            {
                if ((*NowPls == 0 || *NowPls == L'\n') ) Lastt = NowPls,num--;
            }
            if (*Lastt == (WCHAR)"\n")InformationBegin = Lastt + 1;
            else InformationBegin = Lastt;
    }
    return;
}
int GetNumofLine(int typ)        //typ==0统计总行数，typ==1统计当前行数
{
    int numofline = 1;
    for (WCHAR* NowPls = &Infor[0]; *NowPls != 0; NowPls++)
    {
        if (*NowPls == L'\n') numofline++;
        if (typ == 1 && NowPls >= InformationBegin) break;
    }
    return numofline;
}

//
//  函数: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  目标: 处理主窗口的消息。
//
//  WM_COMMAND  - 处理应用程序菜单
//  WM_PAINT    - 绘制主窗口
//  WM_DESTROY  - 发送退出消息并返回
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)   //调用DispatchMessage函数之后由系统来调用
{
    WCHAR* NowPls , *Lastt;
    static int iVScrollBarPos=1;
    static WCHAR Test_Msg[] = L"Hello, World!\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!";
    switch (message)
    {
    case WM_CREATE:
        {
            SetScrollRange(hWnd, SB_VERT, 1, GetNumofLine(0), FALSE);
        }
        break;
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // 分析菜单选择:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case ID_TEST:
                if (DemoFunctionInterface)
                {
                    DemoFunctionInterface = CreateWindow(szDemoFunctionInterfaceClass, szDemoFunctionInterfaceClassTitle, WS_OVERLAPPEDWINDOW,
                        375, 350, 600, 100, MainInterface, nullptr, hInst, nullptr);
                    HWND hstaticedit = CreateWindow(L"static", L"选择文件", WS_CHILD | WS_VISIBLE | SS_CENTERIMAGE /*垂直居中*/ | SS_RIGHT /*水平居右*/,
                        0 /*x坐标*/, 20 /*y坐标*/, 70 /*宽度*/, 26 /*高度*/, DemoFunctionInterface, (HMENU)DEMO_STATIC_EDIT, hInst, NULL);
                    Demoedit = CreateWindow(L"edit", L"", WS_CHILD | WS_VISIBLE | WS_BORDER /*边框*/ | ES_AUTOHSCROLL /*水平滚动*/,
                        80, 20, 200, 26, DemoFunctionInterface, (HMENU)DEMO_EDIT, hInst, NULL);
                    HWND hButton = CreateWindow(L"Button", L"选择文件", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                        330, 20, 200, 26, DemoFunctionInterface, (HMENU)DEMO_BUTTON, hInst, NULL);
                    ShowWindow(DemoFunctionInterface, SW_SHOW);
                    UpdateWindow(DemoFunctionInterface);
                }else  MessageBox(0,L"TestOwn\r\nTTest",L"Test****",0);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);      //会在消息序列中添加一条WM_DESTROY消息
                break;
            case STATIC_BUTTON_UP:
                GetNewInformationBegin(0,-1);       //向上翻一行
                InvalidateRect(MainInterface, NULL, FALSE);
               // Redrawwindow(hWnd,NULL,NULL, RDW_UPDATENOW);
                break;
            case STATIC_BUTTON_DOWN:
                GetNewInformationBegin(0,1);             //向下翻一行
                InvalidateRect(MainInterface, NULL,FALSE );
                break;
            case STATIC_BUTTON_CLEAR:
                PrintInf(1);
                InvalidateRect(MainInterface, NULL, FALSE);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_VSCROLL:
    {
        //垂直滚动条消息，F12查看消息可以从MSDN获取消息详细参数
      //  MessageBox(hWnd, L"VSCROLL", L"提示", MB_OK | MB_ICONINFORMATION);
        switch (LOWORD(wParam))
        {
        case SB_LINEUP:
            GetNewInformationBegin(0, -1);       //向上翻一行
            break;
        case SB_LINEDOWN:
            GetNewInformationBegin(0, 1);             //向下翻一行
            break;
        case SB_PAGEUP:
            GetNewInformationBegin(0, -10);       //向上翻一行
            break;
        case SB_PAGEDOWN:
            GetNewInformationBegin(0, 10);             //向下翻一行
            break;
        case SB_THUMBTRACK:
            iVScrollBarPos = HIWORD(wParam);
            GetNewInformationBegin(1, iVScrollBarPos);
            break;
        default:
            break;
        }
        //判断滚动后的滚动条是否超过最大值或最小值，如果超过最大值或者最小值，则取最大值或0，否则等于当前值
        iVScrollBarPos = GetNumofLine(1);
        SetScrollRange(hWnd, SB_VERT, 1, GetNumofLine(0), FALSE);
        //如果滚动条位置发生变化，则设置滚动条位置和刷新屏幕
        if (iVScrollBarPos != GetScrollPos(hWnd, SB_VERT))
        {
            SetScrollPos(hWnd, SB_VERT, iVScrollBarPos, FALSE);
            //最后参数设置为FALSE可以大幅度减少屏幕闪烁，可以尝试一下。

        }
        InvalidateRect(MainInterface, NULL, FALSE);
        break;
    }
    case WM_PAINT:
        {
          //  MessageBox(0, L"TestOwn\r\nTTest", L"Test****", 0);
            PAINTSTRUCT ps;
            InvalidateRect(StaticTextWidget, NULL, NULL);
            SetWindowText(StaticTextWidget, InformationBegin);
            ShowWindow(StaticTextWidget, SW_HIDE);
            ShowWindow(StaticTextWidget, SW_SHOW);
            HDC hWndhdc = BeginPaint(hWnd, &ps);
           /*WCHAR* LastMsgPointer = &Infor[0], * NowMsgPointer = &Infor[0];
            int NowYAxisVal = 15, NowXAxisVal=20,LenthOfMsg=_tcslen(Infor);
            for (; *NowMsgPointer != 0; ++NowMsgPointer)
            {
                if (*NowMsgPointer == L'\n')
                {
                    *NowMsgPointer = 0;
                    TextOut(hWndhdc, NowXAxisVal, NowYAxisVal, LastMsgPointer, _tcslen(LastMsgPointer));
                    NowYAxisVal += Infor_Output_axis_step; LastMsgPointer = NowMsgPointer + 1;
                    *NowMsgPointer = L'\n';
                }
            }
            if(NowMsgPointer!=LastMsgPointer) TextOut(hWndhdc, NowXAxisVal, NowYAxisVal, LastMsgPointer, _tcslen(LastMsgPointer));
            */ 
            EndPaint(hWnd, &ps);                            // 8
            // TODO: 在此处添加使用 hdc 的任何绘图代码...
        }
        break;
    case WM_CTLCOLORSTATIC:
        {
            HDC hdcStatic = (HDC)wParam;
            //SetTextColor(hdcStatic, RGB(0x41, 0x96, 0x4F));  //翠绿色
            SetBkMode(hdcStatic, TRANSPARENT);  //透明背景
            return (INT_PTR)GetStockObject(NULL_BRUSH);  //无颜色画刷

        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);       //向消息序列提交一条WM_QUIT消息，它会使得GetMessage函数返回0。因此对出循环
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);   //让系统继续处理
    }
    return 0;
}


//
//  函数: FunctionDemoProc(HWND, UINT, WPARAM, LPARAM)
//
//  目标: 处理函数演示窗口的消息。
//
//  WM_COMMAND  - 处理应用程序菜单
//  WM_PAINT    - 绘制主窗口
//  WM_DESTROY  - 发送退出消息并返回
//
//
LRESULT CALLBACK FunctionDemoProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)   //调用DispatchMessage函数之后由系统来调用
{
    switch (message)
    {
    case WM_CREATE:
    {
        break;
    }
    case WM_COMMAND:
    {
        switch (LOWORD(wParam))
        {
        case DEMO_BUTTON:
            //MessageBox(hWnd, L"您点击了第二个按钮。", L"提示", MB_OK | MB_ICONINFORMATION);
            while (1)
            {
                if (GetOpenFileName(&ofn))
                {
                    MessageBox(NULL, StrFilename, TEXT("选择的文件"), 0);
                    break;
                }
                else {
                    break;
                    //MessageBox(NULL, TEXT("请选择一个文件"), NULL, MB_ICONERROR);
                }
            }
           SetWindowTextW(Demoedit,StrFilename);
           startupInfo = { sizeof(STARTUPINFO) }; processInformation = { 0 };
           startupInfo.cb = sizeof(STARTUPINFO);   //不知道有什么用的语句
            //wcscpy_s(StrFilename, L"..\\InjectDll\\Debug\\TestApp.exe");
            //bSuccess = CreateProcessW(L"C:\\Users\\lenovo\\Desktop\\Working\\SoftwareSecurityExperiment\\InjectDll\\Debug\\TestConsole.exe", StrFilename, NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, NULL, &startupInfo, &processInformation);//绝对地址
            bSuccess = CreateProcessW(L"..\\InjectDll\\Debug\\TestConsole.exe", StrFilename, NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, NULL, &startupInfo, &processInformation);//相对地址
            //bSuccess = CreateProcessW(L"..\\Virus\\a+b.exe", StrFilename, NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, NULL, &startupInfo, &processInformation);//相对地址
            BufferStr[0] = 0;
            swprintf(BufferStr, 1000, L"%lS", bSuccess?L"Sucessed":L"Failed");
            MessageBox(NULL, BufferStr, L"Detour ", 0); BufferStr[0] = 0;
            //SendMessage((HWND)lParam, WM_SETTEXT, (WPARAM)NULL, (LPARAM)StrFilename);

            ErrorOuput = CreateWindow(szDemoFunctionInterfaceClass, szDemoFunctionInterfaceClassTitle, WS_OVERLAPPEDWINDOW,
                375, 350, 800, 494, DemoFunctionInterface, nullptr, hInst, nullptr);
            Errordit = CreateWindow(L"edit", L"错误信息输出", WS_CHILD | WS_VISIBLE | WS_BORDER /*边框*/ | ES_AUTOVSCROLL /*垂直滚动*/,
                0 /*x坐标*/, 0 /*y坐标*/, 800 /*宽度*/, 494 /*高度*/, ErrorOuput, nullptr, hInst, NULL);

            SetWindowTextW(Errordit, ERRORInfor);
            ShowWindow(ErrorOuput, SW_SHOW);
            UpdateWindow(ErrorOuput);
            InvalidateRect(ErrorOuput, NULL, FALSE);
            break;
        default:
            break;
        }
        break;
    }
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        // TODO: 在此处添加使用 hdc 的任何绘图代码...
        EndPaint(hWnd, &ps);
    }
    break;
    case WM_CTLCOLORSTATIC:
    {
        HDC hdcStatic = (HDC)wParam;
        //SetTextColor(hdcStatic, RGB(0x41, 0x96, 0x4F));  //翠绿色
        SetBkMode(hdcStatic, TRANSPARENT);  //透明背景
        return (INT_PTR)GetStockObject(NULL_BRUSH);  //无颜色画刷
    }
    break;
    case WM_DESTROY:
        //PostQuitMessage(0);       //向消息序列提交一条WM_QUIT消息，它会使得GetMessage函数返回0。因此对出循环
        ShowWindow(DemoFunctionInterface, SW_HIDE);
        //ShowWindow(MainInterface, SW_SHOW);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);   //让系统继续处理
    }
    return 0;
}


// “关于”框的消息处理程序。
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
