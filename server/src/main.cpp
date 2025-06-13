#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <map>
#include <mutex>
#include <atomic>
#include <memory>
#include <csignal>

#include "WSServer.h"
#include "MetricStore.h"
#include "ClientData.h"
#include "ServerCLI.h"

#include <nlohmann/json.hpp>
#include <boost/asio/signal_set.hpp>

using json = nlohmann::json;
namespace net = boost::asio;

std::map<std::string, std::shared_ptr<MetricStore>> g_client_stores;
std::mutex g_stores_mutex;
std::atomic<bool> g_shutdown_flag{false};

std::chrono::system_clock::time_point parse_iso8601(const std::string& iso_str) {
    std::tm tm = {};
    std::stringstream ss(iso_str);
    ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
    return std::chrono::system_clock::from_time_t(std::mktime(&tm));
}


int main() {
    std::cout << "--- WebSocket Performance Monitor Server ---\n";
    unsigned short port = 6969;
    int threads = 4;
    std::cout << "\nConfiguration set:" << std::endl;
    std::cout << "  - Listening on Port: " << port << std::endl;
    std::cout << "  - Worker Threads:    " << threads << std::endl;
    std::cout << "-------------------------------------------\n" << std::endl;

    try {
        net::io_context ioc{threads};
        WSServer server(ioc, port);
        ServerCLI cli(g_client_stores, g_shutdown_flag);


        server.setOnConnectCallback([](std::shared_ptr<Session> session) {
            std::cout << "\n[Server] Client connected: " << session->get_remote_endpoint() << std::endl;
        });

        server.setOnDisconnectCallback([](std::shared_ptr<Session> /*session*/) {
            std::cout << "[Server] Client disconnected." << std::endl;
        });

        server.setOnMessageCallback([&cli](std::shared_ptr<Session> session, const std::string& msg) {
            try {
                json data = json::parse(msg);

                ClientData received_data;
                received_data.clientId = data.at("clientId").get<std::string>();
                received_data.clientIp = session->get_remote_endpoint().address().to_string();
                received_data.timestamp = parse_iso8601(data.at("timestamp").get<std::string>());
                received_data.metrics = data.at("counters").get<std::vector<MetricDataPoint>>();

                std::shared_ptr<MetricStore> client_store;
                {
                    std::lock_guard<std::mutex> lock(g_stores_mutex);
                    auto it = g_client_stores.find(received_data.clientId);
                    if (it == g_client_stores.end()) {
                        client_store = std::make_shared<MetricStore>();
                        g_client_stores[received_data.clientId] = client_store;
                    } else {
                        client_store = it->second;
                    }
                }

                client_store->addData(received_data);

                std::stringstream ss;
                ss << "[Real-time] Data received from " << received_data.clientId
                   << " (" << received_data.metrics.size() << " metrics)";
                cli.postMessage(ss.str());

            } catch (const std::exception& e) {
                std::cerr << "[Error] Failed to process message: " << e.what() << std::endl;
            }
        });


        boost::asio::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&](boost::system::error_code /*ec*/, int /*signum*/) {
            std::cout << "\nSignal received. Initiating shutdown..." << std::endl;
            g_shutdown_flag = true;
        });

        std::thread shutdown_checker([&ioc]() {
            while (!g_shutdown_flag) {
                std::this_thread::sleep_for(std::chrono::milliseconds(250));
            }
            ioc.stop();
        });


        server.run();
        cli.run();

        std::cout << ">>> Server is running. Type 'help' for commands. Press Ctrl+C to exit. <<<\n" << std::endl;

        std::vector<std::thread> v;
        v.reserve(threads - 1);
        for(auto i = threads - 1; i > 0; --i) {
            v.emplace_back([&ioc] { ioc.run(); });
        }
        ioc.run();

        std::cout << "Asio event loop stopped. Waiting for threads to join..." << std::endl;
        for(auto& t : v) {
            t.join();
        }
        shutdown_checker.join();

    } catch (const std::exception& e) {
        std::cerr << "Fatal Error: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "Shutdown complete." << std::endl;
    return 0;
}