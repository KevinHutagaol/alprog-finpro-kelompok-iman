#include <windows.h>
#include <cstdio>
#include <pdh.h>
#include <pdhmsg.h>
#include <iostream>
#include <memory>
#include <ostream>
#include <winstring.h>
#include <string>
#include <vector>


CONST LPCWSTR COUNTER_OBJECT = L"Process";

std::string ConvertLPCWSTRToString(LPCWSTR lpcwszStr) {
    if (lpcwszStr == nullptr || *lpcwszStr == L'\0') return "";
    int strLength
            = WideCharToMultiByte(CP_UTF8, 0, lpcwszStr, -1,
                                  nullptr, 0, nullptr, nullptr);
    std::string str(strLength, 0);
    WideCharToMultiByte(CP_UTF8, 0, lpcwszStr, -1, &str[0],
                        strLength, nullptr, nullptr);
    return str;
}

int main() {
    PDH_STATUS status = ERROR_SUCCESS;
    std::vector<wchar_t> pwsCounterListBuffer;
    DWORD dwCounterListSize = 0;
    std::vector<wchar_t> pwsInstanceListBuffer;
    DWORD dwInstanceListSize = 0;
    LPWSTR pTemp = nullptr;

    // Determine the required buffer size for the data.
    status = PdhEnumObjectItemsW(
        nullptr, // real-time source
        nullptr, // local machine
        COUNTER_OBJECT, // object to enumerate
        nullptr, // pass NULL and 0
        &dwCounterListSize, // to get required buffer size
        nullptr,
        &dwInstanceListSize,
        PERF_DETAIL_WIZARD, // counter detail level
        0);

    if (status != PDH_MORE_DATA) {
        std::cout << "\nPdhEnumObjectItems failed with 0x" << std::hex << status << std::endl;
        return 1;
    }

    pwsCounterListBuffer.resize(dwCounterListSize);
    pwsInstanceListBuffer.resize(dwInstanceListSize);

    if (pwsCounterListBuffer.empty() || pwsInstanceListBuffer.empty()) {
        std::cout << "Unable to allocate buffers." << std::endl;
        status = ERROR_OUTOFMEMORY;
        return 1;
    }

    status = PdhEnumObjectItemsW(
        nullptr, // real-time source
        nullptr, // local machine
        COUNTER_OBJECT, // object to enumerate
        pwsCounterListBuffer.data(),
        &dwCounterListSize,
        pwsInstanceListBuffer.data(),
        &dwInstanceListSize,
        PERF_DETAIL_WIZARD, // counter detail level
        0);

    if (status != ERROR_SUCCESS) {
        std::cout << "Second PdhEnumObjectItems failed with 0x" << std::hex << status << std::endl;
    }

    std::cout << "Counters that the Process objects defines:\n\n";

    for (pTemp = pwsCounterListBuffer.data(); *pTemp != L'\0'; pTemp += wcslen(pTemp) + 1) {
        std::string tempStr = ConvertLPCWSTRToString(pTemp);
        std::cout << tempStr << std::endl;
    }

    std::cout << "\nInstances of the Process object:\n\n";

    for (pTemp = pwsInstanceListBuffer.data(); *pTemp != L'\0'; pTemp += wcslen(pTemp) + 1) {
        std::string tempStr = ConvertLPCWSTRToString(pTemp);
        std::cout << tempStr << std::endl;
    }

    return 0;
}
