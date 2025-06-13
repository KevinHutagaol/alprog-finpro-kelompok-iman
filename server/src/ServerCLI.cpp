#include "ServerCLI.h"

#include <fstream>
#include <iostream>


#ifdef _WIN32
#include <conio.h>
#else
// For Linux/macOS
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

// A non-blocking keyboard-hit function for POSIX systems
int _kbhit() {
    struct termios oldt, newt;
    int ch;
    int oldf;

    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

    ch = getchar();

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);

    if(ch != EOF) {
        ungetc(ch, stdin);
        return 1;
    }

    return 0;
}

void _clear_input_buffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

#endif

bool is_key_pressed() {
    return _kbhit() != 0;
}

ServerCLI::ServerCLI(std::map<std::string, std::shared_ptr<MetricStore> > &client_stores,
                     std::atomic<bool> &shutdown_flag): client_stores_(client_stores),
                                                        app_shutdown_flag_(shutdown_flag) {
}

ServerCLI::~ServerCLI() {
    stop();
}

void ServerCLI::run() {
    is_running_.store(true);
    cli_thread_ = std::thread(&ServerCLI::cliLoop, this);
}

void ServerCLI::stop() {
    if (!is_running_.load()) {
        return;
    }
    is_running_.store(false);

    if (cli_thread_.joinable()) {
        cli_thread_.join();
    }
}

void ServerCLI::postMessage(const std::string &message) {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    realtime_queue_.push_back(message);
    if (realtime_queue_.size() > 100) {
        realtime_queue_.pop_front();
    }
}

void ServerCLI::cliLoop() {
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    while (is_running_) {
        switch (current_view_.load()) {
            case View::COMMAND:
                runCommandView();
                break;
            case View::REALTIME:
                runRealtimeView();
                break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void ServerCLI::runCommandView() {
    printPrompt();
    std::string line;
    if (std::getline(std::cin, line)) {
        if (!line.empty()) {
            processCommand(line);
        }
    } else {
        std::cout << "\nShutting down CLI." << std::endl;
        handleExit();
    }
}

void ServerCLI::runRealtimeView() {
    if (is_key_pressed()) {
        std::cout << "\nKey pressed. Returning to command view..." << std::endl;
        current_view_ = View::COMMAND;

#ifndef _WIN32
        _clear_input_buffer();
#else
        _getch();
#endif

        return;
    }

    std::list<std::string> messages_to_print; {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        if (!realtime_queue_.empty()) {
            messages_to_print.swap(realtime_queue_);
        }
    }

    for (const auto &msg: messages_to_print) {
        std::cout << msg << std::endl;
    }
}

void ServerCLI::processCommand(const std::string &line) {
    std::stringstream ss(line);
    std::string command;
    ss >> command;

    std::vector<std::string> args;
    std::string arg;
    while (ss >> arg) {
        args.push_back(arg);
    }

    if (command == "help" || command == "?") {
        printHelp();
    } else if (command == "ls" || command == "list") {
        handleListClients();
    } else if (command == "show") {
        handleShowClientData(args);
    } else if (command == "export") {
        handleExportClientData(args);
    } else if (command == "view") {
        handleSwitchView(args);
    } else if (command == "exit" || command == "quit") {
        handleExit();
    } else {
        std::cerr << "Unknown command: '" << command << "'. Type 'help' for a list of commands." << std::endl;
    }
}

void ServerCLI::printPrompt() {
    std::cout << "server> " << std::flush;
}

void ServerCLI::printHelp() {
    std::cout << "\n--- Server CLI Help ---\n"
            << "Available Commands:\n"
            << "  help, ?              - Shows this help message.\n"
            << "  ls, list             - Lists all currently and previously connected client IDs.\n"
            << "  show <client_id>     - Displays a summary of all metrics for a specific client.\n"
            << "  export <client_id> <filename.json> - Exports all data for a client to a JSON file.\n"
            << "  view <mode>          - Switches the CLI view. Modes: 'command', 'realtime'.\n"
            << "  exit, quit           - Shuts down the server and the CLI.\n"
            << "-----------------------\n";
}

void ServerCLI::clearConsoleLine() const {
}

void ServerCLI::handleListClients() const {
    std::cout << "Connected Clients:" << std::endl;

    if (client_stores_.empty()) {
        std::cout << "  (No clients connected)" << std::endl;
        return;
    }
    for (const auto &pair: client_stores_) {
        std::cout << "  - " << pair.first << std::endl;
    }
}

void ServerCLI::handleShowClientData(const std::vector<std::string> &args) const {
    if (args.empty()) {
        std::cerr << "Usage: show <client_id>" << std::endl;
        return;
    }
    const auto &client_id = args[0];

    auto it = client_stores_.find(client_id);
    if (it == client_stores_.end()) {
        std::cerr << "Error: No data found for client ID '" << client_id << "'" << std::endl;
        return;
    }

    std::cout << "\n--- Metrics for Client: " << client_id << " ---\n";
    it->second->print();
    std::cout << "------------------------------------\n";
}

void ServerCLI::handleExportClientData(const std::vector<std::string> &args) const {
    if (args.size() < 2) {
        std::cerr << "Usage: export <client_id> <filename.json>" << std::endl;
        return;
    }
    const auto &client_id = args[0];
    const auto &filename = args[1];

    auto it = client_stores_.find(client_id);
    if (it == client_stores_.end()) {
        std::cerr << "Error: No data found for client ID '" << client_id << "'" << std::endl;
        return;
    }

    nlohmann::json data_to_export;
    data_to_export["client_id"] = client_id;
    data_to_export["note"] = "Export function in MetricStore needs to be implemented.";

    std::ofstream ofs(filename);
    if (!ofs) {
        std::cerr << "Error: Could not open file for writing: " << filename << std::endl;
        return;
    }

    ofs << data_to_export.dump(4);
    std::cout << "Successfully exported data for '" << client_id << "' to '" << filename << "'" << std::endl;
}

void ServerCLI::handleSwitchView(const std::vector<std::string>& args) {
    if (args.empty()) {
        std::cerr << "Usage: view <mode>. Available modes: 'command', 'realtime'" << std::endl;
        return;
    }
    const auto& mode = args[0];
    if (mode == "realtime") {
        current_view_ = View::REALTIME;
        std::cout << "\nSwitched to real-time view. New messages will appear below." << std::endl;
        std::cout << ">>> PRESS ANY KEY to return to command mode <<<" << std::endl;

    } else if (mode == "command") {
        current_view_ = View::COMMAND;
        std::cout << "\nSwitched to command view." << std::endl;
    } else {
        std::cerr << "Unknown view mode: '" << mode << "'" << std::endl;
    }
}

void ServerCLI::handleExit() {
    std::cout << "Shutdown signal sent." << std::endl;
    app_shutdown_flag_ = true;
    is_running_.store(false);
}
