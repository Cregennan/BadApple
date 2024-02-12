#include "diy_winapi.h"
#include <tuple>

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

std::tuple<POINT, CComPtr<IFolderView2>, IShellView*> GetDesktopParams() {
    CCoInitialize init;
    CComPtr<IFolderView2> spView;
    FindDesktopFolderView(IID_PPV_ARGS(&spView));
    CComPtr<IShellFolder> spFolder;
    spView->GetFolder(IID_PPV_ARGS(&spFolder));

    CSFV csfv;
    ZeroMemory(&csfv, sizeof(CSFV));
    csfv.cbSize = sizeof(CSFV);
    csfv.pshf = spFolder;

    IShellView* pShellView;
    SHCreateShellFolderViewEx(&csfv, &pShellView);

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

    return std::make_tuple(point, spView, pShellView);
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