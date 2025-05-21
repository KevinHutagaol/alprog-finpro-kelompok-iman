// This program needs only the essential Windows header files.
#define WIN32_LEAN_AND_MEAN 1

#include <windows.h>
#include <cstdio>
#include <pdh.h>
#include <pdhmsg.h>
#include <io.h>
#include <fcntl.h>


#pragma comment(lib, "pdh.lib")

CONST LPCWSTR  COUNTER_OBJECT = L"Process";

int main() {
    int prev_mode = _setmode(_fileno(stdout), _O_U16TEXT);

    wprintf(L"prev mode: %d\n", prev_mode);

    PDH_STATUS status = ERROR_SUCCESS;
    LPWSTR pwsCounterListBuffer = nullptr;
    DWORD dwCounterListSize = 0;
    LPWSTR pwsInstanceListBuffer = nullptr;
    DWORD dwInstanceListSize = 0;
    LPWSTR pTemp = nullptr;

    // Determine the required buffer size for the data.
    status = PdhEnumObjectItemsW(
        nullptr,                    // real-time source
        nullptr,                    // local machine
        COUNTER_OBJECT,             // object to enumerate
        pwsCounterListBuffer,       // pass NULL and 0
        &dwCounterListSize,         // to get required buffer size
        pwsInstanceListBuffer,
        &dwInstanceListSize,
        PERF_DETAIL_WIZARD,         // counter detail level
        0);

    if (status == PDH_MORE_DATA) {
        // Allocate the buffers and try the call again.
        pwsCounterListBuffer = static_cast<LPWSTR>(malloc(dwCounterListSize * sizeof(WCHAR)));
        pwsInstanceListBuffer = static_cast<LPWSTR>(malloc(dwInstanceListSize * sizeof(WCHAR)));

        if (nullptr != pwsCounterListBuffer && nullptr != pwsInstanceListBuffer) {
            status = PdhEnumObjectItemsW(
                nullptr, // real-time source
                nullptr, // local machine
                COUNTER_OBJECT, // object to enumerate
                pwsCounterListBuffer,
                &dwCounterListSize,
                pwsInstanceListBuffer,
                &dwInstanceListSize,
                PERF_DETAIL_WIZARD, // counter detail level
                0);

            if (status == ERROR_SUCCESS) {
                wprintf(L"Counters that the Process objects defines:\n\n");

                for (pTemp = pwsCounterListBuffer; *pTemp != L'\0'; pTemp += wcslen(pTemp) + 1) {
                    wprintf(L"%s\n", pTemp);
                }

                wprintf(L"\nInstances of the Process object:\n\n");

                for (pTemp = pwsInstanceListBuffer; *pTemp != L'\0'; pTemp += wcslen(pTemp) + 1) {
                    wprintf(L"%s\n", pTemp);
                }
            } else {
                wprintf(L"Second PdhEnumObjectItems failed with 0x%x.\n", status);
            }
        } else {
            wprintf(L"Unable to allocate buffers.\n");
            status = ERROR_OUTOFMEMORY;
        }
    } else {
        wprintf(L"\nPdhEnumObjectItems failed with 0x%x.\n", status);
    }

    if (pwsCounterListBuffer != nullptr)
        free(pwsCounterListBuffer);

    if (pwsInstanceListBuffer != nullptr)
        free(pwsInstanceListBuffer);
}
