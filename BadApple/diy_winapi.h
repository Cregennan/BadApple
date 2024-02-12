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
#include <windows.h>


void FindDesktopFolderView(REFIID riid, void** ppv);

std::tuple<POINT, CComPtr<IFolderView2>, IShellView*> GetDesktopParams();

void RenameFileByHandle(HANDLE handle, std::wstring newName);