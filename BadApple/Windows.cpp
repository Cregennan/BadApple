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
    // Создаем экземпляр IShellWindows
    spShellWindows.CoCreateInstance(CLSID_ShellWindows); 

    CComVariant vtLoc(CSIDL_DESKTOP);
    CComVariant vtEmpty;
    long lhwnd;
    CComPtr<IDispatch> spdisp;
    // Находим окно по его идентификатору SW (SW_DESKTOP в случае рабочего стола)
    spShellWindows->FindWindowSW(
        &vtLoc, &vtEmpty,
        SWC_DESKTOP, &lhwnd, SWFO_NEEDDISPATCH, &spdisp); 

    CComPtr<IShellBrowser> spBrowser;
    CComQIPtr<IServiceProvider>(spdisp)->
        QueryService(SID_STopLevelBrowser,
            IID_PPV_ARGS(&spBrowser));

    CComPtr<IShellView> spView;
    // Находим активный IShellView в выбранном окне
    spBrowser->QueryActiveShellView(&spView); 
    // Находим выбранный объект (в нашем случае IFolderView2) в IShellView
    spView->QueryInterface(riid, ppv);
}

POINT GetDesktopParams() {
    CCoInitialize init;
    CoInitialize(NULL);
    CComPtr<IFolderView2> spView;
    FindDesktopFolderView(IID_PPV_ARGS(&spView));
    
    CComPtr<IShellFolder> spFolder;
    spView->GetFolder(IID_PPV_ARGS(&spFolder));

    CComPtr<IEnumIDList> spEnum;
    spView->Items(SVGIO_ALLVIEW, IID_PPV_ARGS(&spEnum));
    SetProcessDPIAware();

    spView->SetRedraw(TRUE);

    const auto desiredSize = 67;
    FOLDERVIEWMODE viewMode;
    int iconSize;
    spView->GetViewModeAndIconSize(&viewMode, &iconSize);
    spView->SetViewModeAndIconSize(viewMode, desiredSize);

    RECT desktop;
    // Получаем HANDLE окна рабочего стола
    HWND hDesktop = GetDesktopWindow();
    // Получаем прямоугольник окна рабочего стола
    GetWindowRect(hDesktop, &desktop);

    POINT spacing;
    //Получаем ширину значка вместе с отступами
    spView->GetSpacing(&spacing);
    auto xCount = desktop.right / spacing.x;
    auto yCount = desktop.bottom / spacing.y;

    POINT point{};
    point.x = xCount;
    point.y = yCount;

    return point;
}

void RenameFileByHandle(HANDLE handle, std::wstring newName) {
    auto newNameStr = newName.c_str();
    // Создадим структуру с информацией о длине файла
    union
    {
        FILE_RENAME_INFO file_rename_info;
        BYTE buffer[offsetof(FILE_RENAME_INFO, FileName[MAX_PATH])];
    };

    file_rename_info.ReplaceIfExists = TRUE;
    file_rename_info.RootDirectory = nullptr;
    //Заполним информацию о длине названия файла
    file_rename_info.FileNameLength = (ULONG)wcslen(newNameStr) * sizeof(WCHAR);
    // Запишем нули в название файла (для нормальной работы SetFileInformationByHandle название файла должно кончаться на \0)
    memset(file_rename_info.FileName, 0, MAX_PATH);
    // Скопируем нужное название файла в память
    memcpy_s(file_rename_info.FileName, MAX_PATH * sizeof(WCHAR), newNameStr, file_rename_info.FileNameLength);

    // Переименуем файл
    SetFileInformationByHandle(handle, FileRenameInfo, &buffer, sizeof buffer);
}

void SaveScreenshotToFile(const std::wstring& filename)
{
    // Получим контекст устройства экрана
    HDC hScreenDC = CreateDC(L"DISPLAY", NULL, NULL, NULL);

    // Получим размер экрана
    int ScreenWidth = GetDeviceCaps(hScreenDC, HORZRES);
    int ScreenHeight = GetDeviceCaps(hScreenDC, VERTRES);

    // Создадим изображение
    HDC hMemoryDC = CreateCompatibleDC(hScreenDC);
    HBITMAP hBitmap = CreateCompatibleBitmap(hScreenDC, ScreenWidth, ScreenHeight);
    HBITMAP hOldBitmap = (HBITMAP)SelectObject(hMemoryDC, hBitmap);

    // Скопируем скриншот из контекста экрана в контекст памяти (изображения)
    BitBlt(hMemoryDC, 0, 0, ScreenWidth, ScreenHeight, hScreenDC, 0, 0, SRCCOPY);
    hBitmap = (HBITMAP)SelectObject(hMemoryDC, hOldBitmap);

    // Сохраним изображение в файл
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

    // Скопируем биты изображения в буффер
    GetDIBits(hMemoryDC, hBitmap, 0, (UINT)ScreenHeight, lpbitmap, (BITMAPINFO*)&bi, DIB_RGB_COLORS);

    // Создадим файл с будущим скриншотом
    HANDLE hFile = CreateFile(filename.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    // Размер в байтах заголовка изображения
    DWORD dwSizeofDIB = dwBmpSize + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

    // Сдвиг данных пикселей
    bmfHeader.bfOffBits = (DWORD)sizeof(BITMAPFILEHEADER) + (DWORD)sizeof(BITMAPINFOHEADER);

    //Размер файла
    bmfHeader.bfSize = dwSizeofDIB;

    //0x4d42 = 'BM' в кодировке ASCII, обязательное значение
    bmfHeader.bfType = 0x4D42; //BM   

    DWORD dwBytesWritten = 0;
    WriteFile(hFile, (LPSTR)&bmfHeader, sizeof(BITMAPFILEHEADER), &dwBytesWritten, NULL);
    WriteFile(hFile, (LPSTR)&bi, sizeof(BITMAPINFOHEADER), &dwBytesWritten, NULL);
    WriteFile(hFile, (LPSTR)lpbitmap, dwBmpSize, &dwBytesWritten, NULL);

    //Очищаем данные контекстов
    GlobalUnlock(hDIB);
    GlobalFree(hDIB);

    //Закрываем файлы
    CloseHandle(hFile);

    //Очищаем мусор после себя
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
