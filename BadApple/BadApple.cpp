#include "diy_winapi.h"
#include <vector>
#include <iostream>
#include <chrono>
#include <thread>

static const auto line_separator = L"\\";
static const auto black_extension = L".clrb";
static const auto white_extension = L".clrw";

static char desktop_path[MAX_PATH + 1];

int main()
{
    POINT desktopResolution = GetDesktopIconResolution();
    SHGetSpecialFolderPathA(HWND_DESKTOP, desktop_path, CSIDL_DESKTOP, FALSE);
    
    const auto totalScreenCapacity = desktopResolution.x * desktopResolution.y;
    std::vector<std::wstring> blacks(totalScreenCapacity);
    std::vector<std::wstring> whites(totalScreenCapacity);
    auto desktopPath = std::string(desktop_path);
    auto desktopPathW = std::wstring(desktopPath.begin(), desktopPath.end());
    std::vector<HANDLE> handles(totalScreenCapacity);

    for (auto y = 0; y < desktopResolution.y; y++)
    {
        for (auto x = 0; x < desktopResolution.x; x++)
        {
			blacks[y * desktopResolution.x + x] = desktopPathW + line_separator + std::to_wstring(x) + L"_" + std::to_wstring(y) + black_extension;
            whites[y * desktopResolution.x + x] = desktopPathW + line_separator + std::to_wstring(x) + L"_" + std::to_wstring(y) + white_extension;
		}
	}

    for (int i = 0; i < totalScreenCapacity; i++) {
        handles[i] = CreateFile(blacks[i].c_str(), GENERIC_READ | GENERIC_WRITE | DELETE, 0, NULL, CREATE_ALWAYS, 0, NULL);
    }

    auto desktopWindow = GetDesktopWindow();
    RedrawWindow(desktopWindow, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);



    std::cout << "Wait until icons are fully drawn" << std::endl;

    getchar();

    for (auto i = 0; i < desktopResolution.x; i++) {
        RenameFileByHandle(handles[i], whites[i]);
    }

    return 0;
}