#pragma once
#include <windows.h>
#include <string>
#include <vector>

inline std::wstring GetExeVersionString(const std::wstring& key)
{
    wchar_t filename[MAX_PATH];
    GetModuleFileNameW(nullptr, filename, MAX_PATH);

    DWORD handle = 0;
    DWORD size = GetFileVersionInfoSizeW(filename, &handle);
    if (!size) return L"";

    std::vector<BYTE> data(size);

    if (!GetFileVersionInfoW(filename, handle, size, data.data()))
        return L"";

    void* value = nullptr;
    UINT len = 0;

    std::wstring query = L"\\StringFileInfo\\040904B0\\" + key;

    if (VerQueryValueW(data.data(), query.c_str(), &value, &len))
        return std::wstring((wchar_t*)value, len - 1);

    return L"";
}


std::string Version() {
    return "26.0.5-beta";
}