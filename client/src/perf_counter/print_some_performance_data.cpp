#include <cstdio>
#include <conio.h>
#include <cstdint>
#include <iostream>
#include <pdh.h>
#include <windows.h>
#include <string>
#include <chrono>
#include <thread>
#include <ctime>

const std::wstring COUNTER_PATH_1 = L"\\Processor(_Total)\\% Processor Time";
const std::wstring COUNTER_PATH_2 = L"\\Memory\\Available MBytes";
constexpr uint32_t SAMPLE_INTERVAL_MS = 1000;

int main() {
    HCOUNTER hCounter;
    PDH_STATUS pdhStatus;
    HQUERY hQuery = nullptr;

    pdhStatus = PdhOpenQueryW(nullptr, 0, &hQuery);

    if (pdhStatus != ERROR_SUCCESS) {
        std::cout << "PdhOpenQuery failed with 0x" << std::hex << pdhStatus << std::endl;
    }

    pdhStatus = PdhAddCounterW(hQuery,
                               COUNTER_PATH_1.c_str(),
                               0,
                               &hCounter);


    if (pdhStatus != ERROR_SUCCESS) {
        std::cout << "PdhAddCounter failed with 0x" << std::hex << pdhStatus << std::endl;
    }

    time_t currentTime;

    pdhStatus = PdhCollectQueryData(hQuery);
    if (pdhStatus != ERROR_SUCCESS) {
        std::cerr << "\nPdhCollectQueryData failed with 0x" << std::hex << pdhStatus << std::endl;
    }

    while (!_kbhit()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(SAMPLE_INTERVAL_MS));


        pdhStatus = PdhCollectQueryData(hQuery);
        if (pdhStatus != ERROR_SUCCESS) {
            std::cerr << "\nPdhCollectQueryData (loop) failed with 0x" << std::hex << pdhStatus << std::endl;
            continue;
        }

        time(&currentTime);
        tm *localTime = localtime(&currentTime);
        int hour = localTime->tm_hour;
        int min = localTime->tm_min;
        int sec = localTime->tm_sec;

        std::cout << "\nData: \"" << hour << ":" << min << ":" << sec << "\"";


        PDH_FMT_COUNTERVALUE DisplayValue;
        uint32_t CounterType;
        pdhStatus = PdhGetFormattedCounterValue(hCounter,
                                                PDH_FMT_DOUBLE,
                                                reinterpret_cast<LPDWORD>(&CounterType),
                                                &DisplayValue);

        if (pdhStatus != ERROR_SUCCESS) {
            std::cout << "\nPdhGetFormattedCounterValue failed with status 0x" << std::hex << pdhStatus << std::endl;
        }

        std::cout << ",\"" << DisplayValue.doubleValue << "\"";
    }
}
