// pbcopy.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include <windows.h>

void paste()
{
    OpenClipboard(NULL);
    HANDLE data = GetClipboardData(CF_TEXT);
    char* data_text = static_cast<char*>(GlobalLock(data));

    GlobalUnlock(data);
    
    std::cout << data_text << "\n";
    CloseClipboard();
}

void copy_to_clip(const WCHAR* src_data) {
    OpenClipboard(NULL);
    EmptyClipboard(); // must have this
    auto src_data_len = lstrlenW(src_data);
    auto ghandle = GlobalAlloc(GMEM_MOVEABLE, 2* (src_data_len + 1));
    auto ghandle_lock = GlobalLock(ghandle);
    WCHAR* gbuf_chars = static_cast<WCHAR*>(ghandle_lock);
    memcpy(gbuf_chars, src_data, src_data_len*2);
    gbuf_chars[src_data_len] = 0;
    GlobalUnlock(ghandle);
    if (SetClipboardData(CF_UNICODETEXT, ghandle) == NULL) {
        printf("SetClipboardData error: %d\n", GetLastError());
    }
    CloseClipboard();
}

HANDLE m_hChildStd_OUT_Rd = NULL;
HANDLE m_hChildStd_OUT_Wr = NULL;
HANDLE m_hreadDataFromExtProgram = NULL;

DWORD __stdcall readDataFromExtProgram(void* argh)
{
    DWORD dwRead;
    CHAR chBuf[1024];
    BOOL bSuccess = FALSE;
    //printf("111\n");
    for (;;)
    {
        bSuccess = ReadFile(m_hChildStd_OUT_Rd, chBuf, 1024, &dwRead, NULL);
        if (!bSuccess || dwRead == 0) continue;

        // Log chBuf
        std::cout << "thread output: " << chBuf << "\n";

        if (!bSuccess) break;
    }
    return 0;
}


HRESULT RunExternalProgram(std::string extProgram, std::string arguments)
{
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    SECURITY_ATTRIBUTES saAttr;

    ZeroMemory(&saAttr, sizeof(saAttr));
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

    // Create a pipe for the child process's STDOUT. 

    if (!CreatePipe(&m_hChildStd_OUT_Rd, &m_hChildStd_OUT_Wr, &saAttr, 0))
    {
        // log error
        return HRESULT_FROM_WIN32(GetLastError());
    }

    // Ensure the read handle to the pipe for STDOUT is not inherited.

    if (!SetHandleInformation(m_hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0))
    {
        // log error
        return HRESULT_FROM_WIN32(GetLastError());
    }

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.hStdError = m_hChildStd_OUT_Wr;
    si.hStdOutput = m_hChildStd_OUT_Wr;
    si.dwFlags |= STARTF_USESTDHANDLES;

    ZeroMemory(&pi, sizeof(pi));

    std::string commandLine = extProgram + " " + arguments;

    // Start the child process. 
    if (!CreateProcessA(NULL,           // No module name (use command line)
        (char*)commandLine.c_str(),    // Command line
        NULL,                           // Process handle not inheritable
        NULL,                           // Thread handle not inheritable
        TRUE,                           // Set handle inheritance
        0,                              // No creation flags
        NULL,                           // Use parent's environment block
        NULL,                           // Use parent's starting directory 
        &si,                            // Pointer to STARTUPINFO structure
        &pi)                            // Pointer to PROCESS_INFORMATION structure
        )
    {
        printf("error: %d\n", GetLastError());
        return HRESULT_FROM_WIN32(GetLastError());
    }
    else
    {
        m_hreadDataFromExtProgram = CreateThread(0, 0, readDataFromExtProgram, NULL, 0, NULL);
    }
    return S_OK;
}

void main() {
   // copy_to_clip(L"菜单sfaf");
    RunExternalProgram("D:\\installed\\Python39\\python.exe", "D:\\echo.py");
}
