#include "diy_winapi.h"


// CCoInitialize incorporated by reference
class CCoInitialize {
public:
    CCoInitialize() : m_hr(CoInitialize(NULL)) { }
    ~CCoInitialize() { if (SUCCEEDED(m_hr)) CoUninitialize(); }
    operator HRESULT() const { return m_hr; }
    HRESULT m_hr;
};

void FindDesktopFolderView(REFIID riid, void** ppv)
{
    CComPtr<IShellWindows> spShellWindows;
    auto _ = spShellWindows.CoCreateInstance(CLSID_ShellWindows);

    CComVariant vtLoc(CSIDL_DESKTOP);
    CComVariant vtEmpty;
    long lhwnd;
    CComPtr<IDispatch> spdisp;
    spShellWindows->FindWindowSW(
        &vtLoc, &vtEmpty,
        SWC_DESKTOP, &lhwnd, SWFO_NEEDDISPATCH, &spdisp);

    CComPtr<IShellBrowser> spBrowser;
    CComQIPtr<IServiceProvider>(spdisp)->
        QueryService(SID_STopLevelBrowser,
            IID_PPV_ARGS(&spBrowser));

    CComPtr<IShellView> spView;
    spBrowser->QueryActiveShellView(&spView);

    spView->QueryInterface(riid, ppv);
}

POINT GetDesktopParams() {
    CCoInitialize init;
    CComPtr<IFolderView2> spView;
    FindDesktopFolderView(IID_PPV_ARGS(&spView));
    CComPtr<IShellFolder> spFolder;
    spView->GetFolder(IID_PPV_ARGS(&spFolder));

    IShellView* pShellView;
    CSFV csfv;
    ZeroMemory(&csfv, sizeof(csfv));
    csfv.cbSize = sizeof(csfv);
    csfv.pshf = spFolder;
    auto desktopShellGetResult = SHCreateShellFolderViewEx(&csfv, &pShellView);

    CComPtr<IEnumIDList> spEnum;
    spView->Items(SVGIO_ALLVIEW, IID_PPV_ARGS(&spEnum));
    SetProcessDPIAware();

    spView->SetRedraw(TRUE);

    const auto desiredSize = 67;
    POINT spacing;
    spView->GetSpacing(&spacing);
    FOLDERVIEWMODE viewMode;
    int iconSize;
    spView->GetViewModeAndIconSize(&viewMode, &iconSize);
    spView->SetViewModeAndIconSize(viewMode, desiredSize);

    RECT desktop;
    HWND hDesktop = GetDesktopWindow();
    GetWindowRect(hDesktop, &desktop);


    auto xCount = desktop.right / spacing.x;
    auto yCount = desktop.bottom / spacing.y;

    POINT point{};
    point.x = xCount;
    point.y = yCount;

    return point;
}

void RenameFileByHandle(HANDLE handle, std::wstring newName) {
    auto newNameStr = newName.c_str();
    union
    {
        FILE_RENAME_INFO file_rename_info;
        BYTE buffer[offsetof(FILE_RENAME_INFO, FileName[MAX_PATH])];
    };

    file_rename_info.ReplaceIfExists = TRUE;
    file_rename_info.RootDirectory = nullptr;
    file_rename_info.FileNameLength = (ULONG)wcslen(newNameStr) * sizeof(WCHAR);
    memset(file_rename_info.FileName, 0, MAX_PATH);
    memcpy_s(file_rename_info.FileName, MAX_PATH * sizeof(WCHAR), newNameStr, file_rename_info.FileNameLength);

    SetFileInformationByHandle(handle, FileRenameInfo, &buffer, sizeof buffer);
}


void SaveScreenshotToFile(const std::wstring& filename)
{
    // Get the device context of the screen
    HDC hScreenDC = CreateDC(L"DISPLAY", NULL, NULL, NULL);

    // Get the size of the screen
    int ScreenWidth = GetDeviceCaps(hScreenDC, HORZRES);
    int ScreenHeight = GetDeviceCaps(hScreenDC, VERTRES);

    // Create a bitmap
    HDC hMemoryDC = CreateCompatibleDC(hScreenDC);
    HBITMAP hBitmap = CreateCompatibleBitmap(hScreenDC, ScreenWidth, ScreenHeight);
    HBITMAP hOldBitmap = (HBITMAP)SelectObject(hMemoryDC, hBitmap);

    // Copy from the screen DC to the memory DC
    BitBlt(hMemoryDC, 0, 0, ScreenWidth, ScreenHeight, hScreenDC, 0, 0, SRCCOPY);
    hBitmap = (HBITMAP)SelectObject(hMemoryDC, hOldBitmap);

    // Save the bitmap to a file
    BITMAPFILEHEADER bmfHeader;
    BITMAPINFOHEADER bi;

    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = ScreenWidth;
    bi.biHeight = ScreenHeight;
    bi.biPlanes = 1;
    bi.biBitCount = 32;
    bi.biCompression = BI_RGB;
    bi.biSizeImage = 0;
    bi.biXPelsPerMeter = 0;
    bi.biYPelsPerMeter = 0;
    bi.biClrUsed = 0;
    bi.biClrImportant = 0;

    DWORD dwBmpSize = ((ScreenWidth * bi.biBitCount + 31) / 32) * 4 * ScreenHeight;

    HANDLE hDIB = GlobalAlloc(GHND, dwBmpSize);
    char* lpbitmap = (char*)GlobalLock(hDIB);

    // Gets the "bits" from the bitmap and copies them into a buffer 
    GetDIBits(hMemoryDC, hBitmap, 0, (UINT)ScreenHeight, lpbitmap, (BITMAPINFO*)&bi, DIB_RGB_COLORS);

    // A file is created, this is where we will save the screen capture.
    HANDLE hFile = CreateFile(filename.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    // Add the size of the headers to the size of the bitmap to get the total file size
    DWORD dwSizeofDIB = dwBmpSize + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

    //Offset to where the actual bitmap bits start.
    bmfHeader.bfOffBits = (DWORD)sizeof(BITMAPFILEHEADER) + (DWORD)sizeof(BITMAPINFOHEADER);

    //Size of the file
    bmfHeader.bfSize = dwSizeofDIB;

    //bfType must always be BM for Bitmaps
    bmfHeader.bfType = 0x4D42; //BM   

    DWORD dwBytesWritten = 0;
    WriteFile(hFile, (LPSTR)&bmfHeader, sizeof(BITMAPFILEHEADER), &dwBytesWritten, NULL);
    WriteFile(hFile, (LPSTR)&bi, sizeof(BITMAPINFOHEADER), &dwBytesWritten, NULL);
    WriteFile(hFile, (LPSTR)lpbitmap, dwBmpSize, &dwBytesWritten, NULL);

    //Unlock and Free the DIB from the heap
                                                                           GlobalUnlock(hDIB);
    GlobalFree(hDIB);

    //Close the handle for the file that was created
    CloseHandle(hFile);

    //Clean up
    DeleteObject(hBitmap);
    DeleteDC(hMemoryDC);
    DeleteDC(hScreenDC);
}

std::vector<char> ReadAllBytes(char const* filename)
{
    std::ifstream ifs(filename, std::ios_base::binary | std::ios_base::ate);
    std::ifstream::pos_type pos = ifs.tellg();

    if (pos == 0) {
        return std::vector<char>{};
    }

    std::vector<char>  result(pos);

    ifs.seekg(0, std::ios::beg);
    ifs.read(&result[0], pos);

    return result;
}
