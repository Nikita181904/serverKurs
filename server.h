#ifndef SERVER_H
#define SERVER_H

#include "config.h"
#include "logger.h"
#include "authenticator.h"
#include "network.h"
#include <atomic>
#include <memory>
#include <csignal>
#include <vector>
#include <cmath>
#include <limits>
#include <chrono>

class Server {
private:
    ServerConfig config_;
    Logger logger_;
    Authenticator authenticator_;
    NetworkManager network_;
    std::atomic<bool> running_;
    std::chrono::steady_clock::time_point lastActivity_;
    
public:
    Server(const ServerConfig& config);
    ~Server();
    
    bool initialize();
    void run();
    void stop();
    
private:
    void handleClient(int clientSocket, const std::string& clientIP);
    void updateActivity();
    bool shouldShutdownDueToInactivity();
};

class ServerInterface {
private:
    Config config_;
    std::unique_ptr<Server> server_;
    
public:
    int run(int argc, char* argv[]);
};

#endif // SERVER_H