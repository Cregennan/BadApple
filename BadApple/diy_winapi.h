#pragma once
#define UNICODE
#define _UNICODE
#include <atlalloc.h>
#include <atlbase.h>
#include <exdisp.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <stdio.h>
#include <string>
#include <comdef.h>
#include <windows.h>
#include <vector>
#include <tuple>
#include <iostream>
#include <fstream>
#include <chrono>
#include <thread>
#include <GdiPlus.h>
#include <GdiPlusFlat.h>


#define _S(exp) (([](HRESULT hr) { if (FAILED(hr)) _com_raise_error(hr); return hr; })(exp));

void FindDesktopFolderView(REFIID riid, void** ppv);

POINT GetDesktopParams();

void RenameFileByHandle(HANDLE handle, std::wstring newName);

void SaveScreenshotToFile(const std::wstring& fileName);

std::vector<char> ReadAllBytes(char const* filename);