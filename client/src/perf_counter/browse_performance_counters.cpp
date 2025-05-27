#include <cstdio>
#include <conio.h>
#include <cstdint>
#include <iostream>
#include <pdh.h>
#include <pdhmsg.h>
#include <windows.h>
#include <string>


#pragma comment(lib, "pdh.lib")

constexpr uint32_t SAMPLE_INTERVAL_MS = 1000;
CONST std::wstring BROWSE_DIALOG_CAPTION = L"Select a counter to monitor.";

std::string ConvertLPCWSTRToString(LPCWSTR lpcwszStr) {
    if (lpcwszStr == nullptr || *lpcwszStr == L'\0') return "";
    const int strLength
            = WideCharToMultiByte(CP_UTF8, 0, lpcwszStr, -1,
                                  nullptr, 0, nullptr, nullptr);
    std::string str(strLength, 0);
    WideCharToMultiByte(CP_UTF8, 0, lpcwszStr, -1, &str[0],
                        strLength, nullptr, nullptr);
    return str;
}

int main() {
    PDH_STATUS Status;
    HQUERY Query = nullptr;
    HCOUNTER Counter;
    PDH_FMT_COUNTERVALUE DisplayValue;
    uint32_t CounterType;
    SYSTEMTIME SampleTime;
    PDH_BROWSE_DLG_CONFIG_W BrowseDlgData;
    wchar_t CounterPathBuffer[PDH_MAX_COUNTER_PATH];

    //
    // Create a query.
    //

    Status = PdhOpenQueryW(nullptr, NULL, &Query);
    if (Status != ERROR_SUCCESS) {
        wprintf(L"\nPdhOpenQuery failed with status 0x%x.", Status);
        goto Cleanup;
    }

    //
    // Initialize the browser dialog window settings.
    //

    memset(&CounterPathBuffer, 0, sizeof(CounterPathBuffer));
    memset(&BrowseDlgData, 0, sizeof(PDH_BROWSE_DLG_CONFIG));

    BrowseDlgData.bIncludeInstanceIndex = FALSE;
    BrowseDlgData.bSingleCounterPerAdd = TRUE;
    BrowseDlgData.bSingleCounterPerDialog = TRUE;
    BrowseDlgData.bLocalCountersOnly = FALSE;
    BrowseDlgData.bWildCardInstances = TRUE;
    BrowseDlgData.bHideDetailBox = TRUE;
    BrowseDlgData.bInitializePath = FALSE;
    BrowseDlgData.bDisableMachineSelection = FALSE;
    BrowseDlgData.bIncludeCostlyObjects = FALSE;
    BrowseDlgData.bShowObjectBrowser = FALSE;
    BrowseDlgData.hWndOwner = nullptr;
    BrowseDlgData.szReturnPathBuffer = CounterPathBuffer;
    BrowseDlgData.cchReturnPathLength = PDH_MAX_COUNTER_PATH;
    BrowseDlgData.pCallBack = nullptr;
    BrowseDlgData.dwCallBackArg = 0;
    BrowseDlgData.CallBackStatus = ERROR_SUCCESS;
    BrowseDlgData.dwDefaultDetailLevel = PERF_DETAIL_WIZARD;
    BrowseDlgData.szDialogBoxCaption = const_cast<LPWSTR>(BROWSE_DIALOG_CAPTION.c_str());

    //
    // Display the counter browser window. The dialog is configured
    // to return a single selection from the counter list.
    //

    Status = PdhBrowseCountersW(&BrowseDlgData);

    if (Status != ERROR_SUCCESS) {
        if (Status == PDH_DIALOG_CANCELLED) {
            wprintf(L"\nDialog canceled by user.");
        } else {
            wprintf(L"\nPdhBrowseCounters failed with status 0x%x.", Status);
        }
        goto Cleanup;
    } else if (wcslen(CounterPathBuffer) == 0) {
        wprintf(L"\nUser did not select any counter.");
        goto Cleanup;
    } else {
        std::cout << "\nCounter selected: " << ConvertLPCWSTRToString(CounterPathBuffer) << std::endl;
    }

    //
    // Add the selected counter to the query.
    //

    Status = PdhAddCounterW(Query, CounterPathBuffer, 0, &Counter);
    if (Status != ERROR_SUCCESS) {
        wprintf(L"\nPdhAddCounter failed with status 0x%x.", Status);
        goto Cleanup;
    }

    //
    // Most counters require two sample values to display a formatted value.
    // PDH stores the current sample value and the previously collected
    // sample value. This call retrieves the first value that will be used
    // by PdhGetFormattedCounterValue in the first iteration of the loop
    // Note that this value is lost if the counter does not require two
    // values to compute a displayable value.
    //

    Status = PdhCollectQueryData(Query);
    if (Status != ERROR_SUCCESS) {
        wprintf(L"\nPdhCollectQueryData failed with 0x%x.\n", Status);
        goto Cleanup;
    }

    //
    // Print counter values until a key is pressed.
    //

    while (!_kbhit()) {
        Sleep(SAMPLE_INTERVAL_MS);

        GetLocalTime(&SampleTime);

        Status = PdhCollectQueryData(Query);
        if (Status != ERROR_SUCCESS) {
            wprintf(L"\nPdhCollectQueryData failed with status 0x%x.", Status);
        }

        wprintf(L"\n\"%2.2d/%2.2d/%4.4d %2.2d:%2.2d:%2.2d.%3.3d\"",
                SampleTime.wMonth,
                SampleTime.wDay,
                SampleTime.wYear,
                SampleTime.wHour,
                SampleTime.wMinute,
                SampleTime.wSecond,
                SampleTime.wMilliseconds);

        //
        // Compute a displayable value for the counter.
        //

        Status = PdhGetFormattedCounterValue(Counter,
                                             PDH_FMT_DOUBLE,
                                             reinterpret_cast<LPDWORD>(&CounterType),
                                             &DisplayValue);
        if (Status != ERROR_SUCCESS) {
            wprintf(L"\nPdhGetFormattedCounterValue failed with status 0x%x.", Status);
            goto Cleanup;
        }

        wprintf(L",\"%.20g\"", DisplayValue.doubleValue);
    }

Cleanup:

    //
    // Close the query.
    //

    if (Query) {
        PdhCloseQuery(Query);
    }
}
