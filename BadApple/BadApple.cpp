#include "diy_winapi.h"

static const auto line_separator = L"\\";
static const auto black_extension = L".baclrb";
static const auto white_extension = L".baclrw";
static const auto pixel_source_path = "PATH_TO_BAPL_FILE";
static const auto screenshot_path = std::wstring(L"YOUR_SCREENSHOT_FOLDER");
static const char BYTE_ONE = 255;
static const char BYTE_ZERO = 0;
static const char BYTE_FRAME_PIXEL = 0;
static const char BYTE_FRAME_SCREENSHOT = 1;
static const auto DEFAULT_SLEEP_TIME = std::chrono::milliseconds(3000);


static auto desktopWindow = GetDesktopWindow();

void FillDesktop(POINT desktopResolution, std::vector<HANDLE>& handles, std::vector<std::wstring>& blacks, std::vector<std::wstring>& whites) {
    // desktop_path будет указывать на строку - путь к рабочему столу текущего пользователя
    char desktop_path[MAX_PATH + 1];
    SHGetSpecialFolderPathA(HWND_DESKTOP, desktop_path, CSIDL_DESKTOP, FALSE);

    const auto totalScreenCapacity = desktopResolution.x * desktopResolution.y;
    auto desktopPath = std::string(desktop_path);
    auto desktopPathW = std::wstring(desktopPath.begin(), desktopPath.end());
  
    for (auto y = 0; y < desktopResolution.y; y++)
    {
        for (auto x = 0; x < desktopResolution.x; x++)
        {
            blacks[y * desktopResolution.x + x] = desktopPathW + line_separator + std::to_wstring(x) + L"_" + std::to_wstring(y) + black_extension;
            whites[y * desktopResolution.x + x] = desktopPathW + line_separator + std::to_wstring(x) + L"_" + std::to_wstring(y) + white_extension;
        }
    }

    for (int i = 0; i < totalScreenCapacity; i++) {
        // Создаем файлы с черной иконкой с доступом на чтение, запись и удаление
        handles[i] = CreateFile(blacks[i].c_str(), GENERIC_READ | GENERIC_WRITE | DELETE, 0, NULL, CREATE_ALWAYS, 0, NULL);
    }
}

void TakeScreenshot(int index){
    // Путь i-того скриншота
    auto path = screenshot_path + L"shot_" + std::to_wstring(index) + L".png";
    // Сообщение - обновление окон
    SendMessage(HWND_BROADCAST, WM_SETTINGCHANGE, 0, 0);
    // Ждем DEFAULT_SLEEP_TIME миллисекунд
    std::this_thread::sleep_for(DEFAULT_SLEEP_TIME);
    // Делаем скриншот
    SaveScreenshotToFile(path);
}

int main()
{
    // Получаем параметры рабочего стола
    auto desktopResolution = GetDesktopParams();
    const auto totalScreenCapacity = desktopResolution.x * desktopResolution.y;

    // Создаем файлы и заполняем векторы с названиями файлов и дескрипторами
    std::vector<HANDLE> handles(totalScreenCapacity);
    std::vector<std::wstring> blacks(totalScreenCapacity);
    std::vector<std::wstring> whites(totalScreenCapacity);
    FillDesktop(desktopResolution, handles, blacks, whites);

    // Считываем содержимое файла framedata.bapl
    auto bytes = ReadAllBytes(pixel_source_path);
    auto i = 0; 
    auto frame = 0;

    // Отрисовываем созданные файлы
    SendMessage(HWND_BROADCAST, WM_SETTINGCHANGE, 0, 0);

    std::cout << "5" << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    std::cout << "4" << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    std::cout << "3" << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    std::cout << "2" << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    std::cout << "1" << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    
    while (i < bytes.size()) {
        i++;
        //Считываем очередной байт
        char value = bytes[i];
        // Если это команда для снимка экрана - делаем скриншот
        if (value == BYTE_FRAME_SCREENSHOT) {
            TakeScreenshot(frame);
            frame++;
        }
        else {
            // Получаем координаты и цвет пикселя
            auto x = bytes[i + 1];
            auto y = bytes[i + 2];
            auto color = bytes[i + 3];
            i += 3;

            // Переименовываем соответствующий файл
            auto position = y * desktopResolution.x + x;
            RenameFileByHandle(handles[position], color == BYTE_ONE ? whites[position] : blacks[position]);
        }
    }
    // Делаем финальный скриншот
    TakeScreenshot(frame);
    return 0;
}