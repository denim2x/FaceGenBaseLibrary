//
// Copyright (c) 2019 Singular Inversions Inc. (facegen.com)
// Use, modification and distribution is subject to the MIT License,
// see accompanying file LICENSE.txt or facegen.com/base_library_license.txt
//
//

#include "stdafx.h"

#include "FgGuiApiDialogs.hpp"
#include "FgGuiWin.hpp"
#include "FgScopeGuard.hpp"
#include "FgSystemInfo.hpp"

using namespace std;
using namespace std::placeholders;

namespace Fg {

template<class T>
using Unique = std::unique_ptr<T,Fg::Release<T> >;

void
guiDialogMessage(
    const Ustring & cap,
    const Ustring & msg)
{
    MessageBox(
        // Sending the main window handle makes it the OWNER of this window (not parent since
        // this is not a child window but an actual window), which makes this a modal dialog:
        s_guiWin.hwndMain,
        msg.as_wstring().c_str(),
        cap.as_wstring().c_str(),
        MB_OK);
}

Opt<Ustring>
guiDialogFileLoad(
    const Ustring &         description,
    const Strings &         extensions,
    const string &          storeID)
{
    FGASSERT(!extensions.empty());
    Opt<Ustring>          ret;
    HRESULT                 hr;
    IFileDialog *           pfdPtr = NULL;
    hr = CoCreateInstance(CLSID_FileOpenDialog,NULL,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&pfdPtr));
    FGASSERTWINOK(hr);
    Unique<IFileDialog>     pfd(pfdPtr);
    // Giving each dialog a GUID based on it's description will allow Windows to remember
    // previously chosen directories for each dialog (with a different description):
    GUID                    guid;
    size_t                  hashVal = hash<string>{}(description.m_str+storeID);
    guid.Data1 = ulong(hashVal);
    guid.Data2 = ushort(0x7708U);       // Randomly chosen for this function
    guid.Data3 = ushort(0x20DAU);       // "
    for (uint ii=0; ii<8; ++ii)
        guid.Data4[ii] = 0;             // Ensure consistent
    hr = pfd->SetClientGuid(guid);
    FGASSERTWINOK(hr);
    // Get existing (default) options to avoid overwrite:
    DWORD                   dwFlags;
    hr = pfd->GetOptions(&dwFlags);
    FGASSERTWINOK(hr);
    // Only want filesystem items; don't support other shell items:
    hr = pfd->SetOptions(dwFlags | FOS_FORCEFILESYSTEM);
    FGASSERTWINOK(hr);
    wstring                 desc = description.as_wstring(),
                            exts = L"*." + Ustring(extensions[0]).as_wstring();
    for (size_t ii=1; ii<extensions.size(); ++ii)
        exts += L";*." + Ustring(extensions[ii]).as_wstring();
    COMDLG_FILTERSPEC       fs;
    fs.pszName = desc.c_str();
    fs.pszSpec = exts.c_str();
    hr = pfd->SetFileTypes(1,&fs);
    FGASSERTWINOK(hr);
    // Set the selected file type index (starts at 1):
    hr = pfd->SetFileTypeIndex(1);
    FGASSERTWINOK(hr);
    hr = pfd->Show(s_guiWin.hwndMain);    // Blocking call to display dialog
    if (hr == S_OK) {                       // A filename was selected
        IShellItem *        psiResult;
        hr = pfd->GetResult(&psiResult);
        if (hr == S_OK) {
            Unique<IShellItem>  psiResultRaii(psiResult);
            PWSTR               pszFilePath = NULL;
            hr = psiResult->GetDisplayName(SIGDN_FILESYSPATH,&pszFilePath);
            if ((hr == S_OK) && (pszFilePath != NULL)) {
                ret = Ustring(pszFilePath);
                CoTaskMemFree(pszFilePath);
            }
        }
    }
    return ret;
}

Opt<Ustring>
guiDialogFileSave(
    const Ustring &    description,
    const string &      extension)
{
    FGASSERT(!extension.empty());
    Opt<Ustring>          ret;
    HRESULT                 hr;
    IFileDialog *           pfdPtr = NULL;
    hr = CoCreateInstance(CLSID_FileSaveDialog,NULL,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&pfdPtr));
    FGASSERTWINOK(hr);
    Unique<IFileDialog>     pfd(pfdPtr);
    // Giving each dialog a GUID based on it's description will allow Windows to remember
    // previously chosen directories for each dialog (with a different description):
    GUID                    guid;
    size_t                  hashVal = hash<string>{}(description.m_str);
    guid.Data1 = ulong(hashVal);
    guid.Data2 = ushort(0x0F3FU);       // Randomly chosen
    guid.Data3 = ushort(0x574CU);       // "
    for (uint ii=0; ii<8; ++ii)
        guid.Data4[ii] = 0;             // Ensure consistent
    hr = pfd->SetClientGuid(guid);
    FGASSERTWINOK(hr);
    // Get existing (default) options to avoid overwrite:
    DWORD                   dwFlags;
    hr = pfd->GetOptions(&dwFlags);
    FGASSERTWINOK(hr);
    // Only want filesystem items; don't support other shell items:
    hr = pfd->SetOptions(dwFlags | FOS_FORCEFILESYSTEM);
    FGASSERTWINOK(hr);
    wstring                 desc = description.as_wstring(),
                            exts = L"*." + Ustring(extension).as_wstring();
    COMDLG_FILTERSPEC       fs;
    fs.pszName = desc.data();
    fs.pszSpec = exts.data();
    hr = pfd->SetFileTypes(1,&fs);
    FGASSERTWINOK(hr);
    // Set the selected file type index (starts at 1):
    hr = pfd->SetFileTypeIndex(1);
    FGASSERTWINOK(hr);
    wstring                 ext = Ustring(extension).as_wstring();
    hr = pfd->SetDefaultExtension(ext.c_str());
    hr = pfd->Show(s_guiWin.hwndMain);    // Blocking call to display dialog
    if (hr == S_OK) {                       // A filename was selected
        IShellItem *        psiResult;
        hr = pfd->GetResult(&psiResult);
        if (hr == S_OK) {
            Unique<IShellItem>  psiResultRaii(psiResult);
            PWSTR           pszFilePath = NULL;
            hr = psiResult->GetDisplayName(SIGDN_FILESYSPATH,&pszFilePath);
            if (hr == S_OK) {
                ret = Ustring(pszFilePath);
                CoTaskMemFree(pszFilePath);
            }
        }
    }
    return ret;
}

Opt<Ustring>
guiDialogDirSelect()
{
    Opt<Ustring>            ret;
    HRESULT                 hr;
    IFileDialog *           pfdPtr = NULL;
    hr = CoCreateInstance(CLSID_FileOpenDialog,NULL,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&pfdPtr));
    FGASSERTWINOK(hr);
    Unique<IFileDialog>     pfd(pfdPtr);
    // Get existing (default) options to avoid overwrite:
    DWORD                   dwFlags;
    hr = pfd->GetOptions(&dwFlags);
    FGASSERTWINOK(hr);
    // Only want filesystem items; don't support other shell items:
    hr = pfd->SetOptions(dwFlags | FOS_FORCEFILESYSTEM | FOS_PICKFOLDERS);
    FGASSERTWINOK(hr);
    hr = pfd->Show(s_guiWin.hwndMain);    // Blocking call to display dialog
    if (hr == S_OK) {                       // A filename was selected
        IShellItem *        psiResult;
        hr = pfd->GetResult(&psiResult);
        if (hr == S_OK) {
            Unique<IShellItem>  psiResultRaii(psiResult);
            PWSTR               pszFilePath = NULL;
            hr = psiResult->GetDisplayName(SIGDN_FILESYSPATH,&pszFilePath);
            if (hr == S_OK) {
                Ustring         str(pszFilePath);
                CoTaskMemFree(pszFilePath);
                return fgAsDirectory(str);
            }
        }
    }
    return ret;
}

struct  GuiDialogProgressWin
{
    uint        progressSteps;
    HWND        hwndThis;
    HWND        hwndProgBar;
    HWND        hwndButton;
    // Doesn't need to be atomic since it's only written by one thread and any state change is valid:
    bool        cancelFlag = false;

    LRESULT
    wndProc(HWND hwnd,UINT message,WPARAM wParam,LPARAM lParam)
    {
        if (message == WM_CREATE) {
            hwndThis = hwnd;
            hwndProgBar =
                CreateWindowEx(
                    0,PROGRESS_CLASS,(LPTSTR)NULL,
                    WS_CHILD | WS_BORDER | WS_VISIBLE,
                    100,20,300,50,
                    hwndThis,(HMENU)1,s_guiWin.hinst,NULL);
            FGASSERTWIN(hwndProgBar != 0);
            SendMessage(hwndProgBar,PBM_SETRANGE,0,MAKELPARAM(0,progressSteps));
            SendMessage(hwndProgBar,PBM_SETSTEP,(WPARAM)1,0);
            wstring     cancel = L"Cancel";
            hwndButton =
                CreateWindowEx(0,
                    TEXT("button"),     // Standard controls class name for all buttons
                    cancel.c_str(),
                    WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                    100,120,300,20,     // Will be sent MOVEWINDOW messages.
                    hwnd,
                    HMENU(0),
                    s_guiWin.hinst,
                    NULL);              // No WM_CREATE parameter
            FGASSERTWIN(hwndButton != 0);
            return 0;
        }
        else if (message == WM_COMMAND) {
            WORD    ident = LOWORD(wParam);
            WORD    code = HIWORD(wParam);
            if (code == 0) {
                FGASSERT(ident == 0);
                cancelFlag = true;
            }
            return 0;
        }
        return DefWindowProc(hwnd,message,wParam,lParam);
    }
};

static
void
threadWorker(WorkerFunc const & worker,HWND hwndMain,HWND hwndProgBar,bool & cancelFlag,Ustring & errMsg)
{
    WorkerCallback          callback = [=](bool isMilestone)
        {
            if (cancelFlag) {
                // It's critical to use 'PostMessage' here instead of 'SendMessage' since the latter bypasses
                // the message queue so the modal message loop below would have no way of knowing when it's
                // being closed:
                PostMessage(hwndMain,WM_CLOSE,0,0);
                return true;
            }
            if (isMilestone)
                SendMessage(hwndProgBar,PBM_STEPIT,0,0);
            return false;
        };

        //fnUpdateDialog = bind(updateDialog,hwndMain,hwndProgBar,_1);
    try {
        worker(callback);
    }
    catch(FgException const & e)
    {
        errMsg = "FG Exception: " + e.no_tr_message();
    }
    catch(std::bad_alloc const &)
    {
        errMsg = "OUT OF MEMORY ";
#ifndef FG_64
        if (fg64bitOS())
            errMsg += "(install 64-bit version if possible) ";
#endif
    }
    catch(std::exception const & e)
    {
        errMsg = "std::exception: " + Ustring(e.what());
    }
    catch(...)
    {
        errMsg = "Unknown type: ";
    }
    // It's critical to use 'PostMessage' here instead of 'SendMessage' since the latter bypasses
    // the message queue so the modal message loop below would have no way of knowing when it's
    // being closed:
    PostMessage(hwndMain,WM_CLOSE,0,0);
}

bool
guiDialogProgress(Ustring const & title,uint progressSteps,WorkerFunc worker)
{
    GuiDialogProgressWin    progBar;
    progBar.progressSteps = progressSteps;
    // Create the dialog window with the main window as its parent window:
    HWND                    hwndMain = winCreateDialog(title,s_guiWin.hwndMain,&progBar);
    EnableWindow(s_guiWin.hwndMain,FALSE);    // Disable main window for model dialog
    ShowWindow(hwndMain,SW_SHOWNORMAL);
    UpdateWindow(hwndMain);
    Ustring                 errMsg;
    thread                  compute(threadWorker,
        cref(worker),hwndMain,progBar.hwndProgBar,ref(progBar.cancelFlag),ref(errMsg));
    MSG                     msg;
    while (GetMessage(&msg,NULL,0,0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        // 'GetMessage' will never return 0 since that only happens for WM_QUIT, which only happens
        // for program termination:
        if (msg.message == WM_CLOSE)
            break;
    }
    compute.join();
    DestroyWindow(hwndMain);
    EnableWindow(s_guiWin.hwndMain,TRUE);    // Re-enable main window
    if (!errMsg.empty())
        fgThrow("Exception in guiDialogProgress",errMsg);
    return !progBar.cancelFlag;
}

// **************************************** Splash Screen *****************************************

static
const int s_splashSize = 256;

struct  GuiDialogSplashScreenWin
{
    HWND            hwndThis;
    ImgC4UC     img;

    GuiDialogSplashScreenWin() : hwndThis(0), img(s_splashSize,s_splashSize,RgbaUC(0,255,0,255)) {}

    LRESULT
    wndProc(HWND hwnd,UINT msg,WPARAM,LPARAM)
    {
        if (msg == WM_INITDIALOG) {
            hwndThis = hwnd;
            RECT            rect;
            SystemParametersInfo(SPI_GETWORKAREA,NULL,&rect,0);
            int             x = (rect.right - rect.left - s_splashSize)/2,
                            y = (rect.bottom - rect.top - s_splashSize)/2;
            // Necessary as the DLGTEMPLATE values don't seem to be respected:
            SetWindowPos(hwnd,0,x,y,s_splashSize,s_splashSize,SWP_NOZORDER);
            return TRUE;
        }
        else if (msg == WM_ERASEBKGND) {
            return TRUE;    // Don't erase background for cool icon superposition
        }
        else if (msg == WM_PAINT) {
            PAINTSTRUCT     ps;
            HDC             hdc = BeginPaint(hwnd,&ps);
            HANDLE          icon = LoadImage(GetModuleHandle(NULL),MAKEINTRESOURCE(101),IMAGE_ICON,s_splashSize,s_splashSize,0);
            if (icon == NULL) {
                struct  FGBMI
                {
                    BITMAPINFOHEADER    bmiHeader;
                    DWORD               redMask;
                    DWORD               greenMask;
                    DWORD               blueMask;
                };
                FGBMI                   bmi;
                memset(&bmi,0,sizeof(bmi));
                BITMAPINFOHEADER &      bmih = bmi.bmiHeader;
                bmih.biSize = sizeof(BITMAPINFOHEADER);
                bmih.biWidth = img.width();
                bmih.biHeight = -int(img.height());
                bmih.biPlanes = 1;                  // Must always be 1
                bmih.biBitCount = 32;
                bmih.biCompression = BI_BITFIELDS;  // Uncompressed
                bmi.redMask = 0xFF;
                bmi.greenMask = 0xFF00;
                bmi.blueMask = 0xFF0000;
                SetDIBitsToDevice(
                    hdc,
                    0,0,
                    img.width(),img.height(),
                    0,0,
                    0,img.height(),
                    img.data(),      // This pointer is kept after function return
                    (BITMAPINFO*)&bmi,
                    DIB_RGB_COLORS);
            }
            else {
                BOOL res = DrawIconEx(hdc,0,0,(HICON)icon,s_splashSize,s_splashSize,0,NULL,DI_NORMAL);
                FGASSERTWIN(res != 0);
            }
            EndPaint(hwnd,&ps);
            return 0;
        }
        return FALSE;
    }
};

static
GuiDialogSplashScreenWin  s_fgGuiWinDialogSplashScreen;

static
INT_PTR CALLBACK
fgGuiWinDialogFunc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{return s_fgGuiWinDialogSplashScreen.wndProc(hwndDlg,uMsg,wParam,lParam); }

static
void
fgGuiWinDialogSplashClose()
{
    if (s_fgGuiWinDialogSplashScreen.hwndThis != 0)
        EndDialog(s_fgGuiWinDialogSplashScreen.hwndThis,0);
}

std::function<void(void)>
guiDialogSplashScreen()
{
    // Need 4 bytes of zero after the DLGTEMPLATE structure for 'CreateDialog...' to work:
    void *          mem = malloc(sizeof(DLGTEMPLATE)+4);
    memset(mem,0,sizeof(DLGTEMPLATE)+4);
    DLGTEMPLATE    *d = (DLGTEMPLATE*)mem;
    d->style = WS_POPUP | WS_VISIBLE;
    HWND    res = CreateDialogIndirectParam(NULL,d,NULL,&fgGuiWinDialogFunc,0);
    free(mem);
    FGASSERTWIN(res != NULL);
    return &fgGuiWinDialogSplashClose;
}

}

// */
