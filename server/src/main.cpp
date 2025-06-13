#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <cstring>
#include <chrono>

#include <winsock2.h>
#include <ws2tcpip.h>

#include "Metric.h"
#include "MetricStore.h"
#include "WSServer.h"

#pragma comment(lib, "Ws2_32.lib")

const int PORT = 12345;
const int MAX_PENDING_CONNECTIONS = 5;
const int BUFFER_SIZE = 1024;


Metric parseMetricData(const std::string& data, const std::string& client_ip, const std::string& hostname) {
    long long timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                                  std::chrono::system_clock::now().time_since_epoch()
                              ).count();

    double cpu_usage = 0.0;
    double memory_usage = 0.0;

    try {
        size_t cpu_pos = data.find("CPU:");
        size_t mem_pos = data.find("MEM:");

        if (cpu_pos != std::string::npos && mem_pos != std::string::npos && cpu_pos < mem_pos) {
            cpu_usage = std::stod(data.substr(cpu_pos + 4, mem_pos - (cpu_pos + 4)));
            memory_usage = std::stod(data.substr(mem_pos + 4));
        } else {
            std::cout << "[SERVER] Peringatan: Format data tidak dikenali: " << data << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "[SERVER ERROR] Gagal parse data: " << e.what() << " untuk data: " << data << std::endl;
    }
    return Metric(timestamp, client_ip, hostname, cpu_usage, memory_usage);
}

void handleClient(SOCKET clientSocket) {
    char buffer[BUFFER_SIZE];
    int bytesReceived;

    sockaddr_in clientAddr;
    int clientAddrLen = sizeof(clientAddr);
    getpeername(clientSocket, (sockaddr*)&clientAddr, &clientAddrLen);
    char clientIp[INET_ADDRSTRLEN];
    InetNtopA(AF_INET, &(clientAddr.sin_addr), clientIp, INET_ADDRSTRLEN);
    int clientPort = ntohs(clientAddr.sin_port);

    std::string clientHostname = "unknown_hostname";

    std::cout << "[SERVER] Klien terhubung dari " << clientIp << ":" << clientPort << " (Socket: " << clientSocket << ")" << std::endl;

    while ((bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[bytesReceived] = '\0';
        std::string receivedData(buffer);

        std::cout << "[SERVER] Menerima dari " << clientIp << ":" << clientPort << ": " << receivedData << std::endl;

        Metric newMetric = parseMetricData(receivedData, std::string(clientIp), clientHostname);

        MetricStore::getInstance().saveMetric(newMetric);

        const char* response = "ACK: Data diterima!\n";
        send(clientSocket, response, strlen(response), 0);
    }

    if (bytesReceived == 0) {
        std::cout << "[SERVER] Klien terputus: " << clientIp << ":" << clientPort << " (Socket: " << clientSocket << ")" << std::endl;
    } else {
        std::cerr << "[SERVER ERROR] Gagal menerima data dari " << clientIp << ":" << clientPort << ". Kode Error: " << WSAGetLastError() << std::endl;
    }

    closesocket(clientSocket);
}

int main() {
    WSADATA wsaData;
    SOCKET serverSocket, clientSocket;
    struct sockaddr_in serverAddr{};
    struct sockaddr_in clientAddr{};
    int clientAddrLen;

    std::cout << "[SERVER] Memulai Performance Logger Server..." << std::endl;

    MetricStore::getInstance().loadAllMetricsFromBinaryFile("metrics.bin");

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "[SERVER ERROR] WSAStartup gagal. Kode Error: " << WSAGetLastError() << std::endl;
        return 1;
    }
    std::cout << "[SERVER] Winsock berhasil diinisialisasi." << std::endl;

    serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == INVALID_SOCKET) {
        std::cerr << "[SERVER ERROR] Gagal membuat socket. Kode Error: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }
    std::cout << "[SERVER] Socket server berhasil dibuat." << std::endl;

    char optval = 1;
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == SOCKET_ERROR) {
        std::cerr << "[SERVER ERROR] setsockopt(SO_REUSEADDR) gagal. Kode Error: " << WSAGetLastError() << std::endl;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);

    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "[SERVER ERROR] Gagal bind socket ke port " << PORT << ". Kode Error: " << WSAGetLastError() << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }
    std::cout << "[SERVER] Socket berhasil di-bind ke port " << PORT << std::endl;

    if (listen(serverSocket, MAX_PENDING_CONNECTIONS) == SOCKET_ERROR) {
        std::cerr << "[SERVER ERROR] Listen gagal. Kode Error: " << WSAGetLastError() << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }
    std::cout << "[SERVER] Server mendengarkan di port " << PORT << "..." << std::endl;

    while (true) {
        clientAddrLen = sizeof(clientAddr);
        clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientAddrLen);
        if (clientSocket == INVALID_SOCKET) {
            std::cerr << "[SERVER ERROR] Terima koneksi gagal. Kode Error: " << WSAGetLastError() << std::endl;
            continue;
        }

        std::thread(handleClient, clientSocket).detach();
    }

    closesocket(serverSocket);
    WSACleanup();
    std::cout << "[SERVER] Server berhenti." << std::endl;
    return 0;
}
