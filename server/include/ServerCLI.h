#ifndef SERVERCLI_H
#define SERVERCLI_H

#include <vector>
#include <string>
#include <atomic>
#include <thread>
#include <mutex>
#include <map>
#include <memory>
#include <list>

#include "Metric.h"
#include "MetricStore.h"

class ServerCLI {
public:
    ServerCLI(std::map<std::string, std::shared_ptr<MetricStore> > &client_stores, std::atomic<bool> &shutdown_flag);

    ~ServerCLI();

    void run();

    void stop();

    void postMessage(const std::string &message);

private:
    enum class View { COMMAND, REALTIME };

    std::atomic<View> current_view_{View::COMMAND};
    std::thread cli_thread_;
    std::atomic<bool> is_running_{false};

    std::map<std::string, std::shared_ptr<MetricStore> > &client_stores_;
    std::atomic<bool> &app_shutdown_flag_;

    std::list<std::string> realtime_queue_;
    std::mutex queue_mutex_;

    void cliLoop();

    void runCommandView();

    void runRealtimeView();

    void processCommand(const std::string &line);

    void printPrompt() const;

    void printHelp() const;

    void clearConsoleLine() const;

    void handleListClients() const;

    void handleShowClientData(const std::vector<std::string> &args) const;

    void handleExportClientData(const std::vector<std::string> &args) const;

    void handleSwitchView(const std::vector<std::string> &args);

    void handleExit();
};


#endif //SERVERCLI_H
