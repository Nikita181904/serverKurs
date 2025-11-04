#ifndef SERVER_H
#define SERVER_H

#include "config.h"
#include "logger.h"
#include "authenticator.h"
#include "processor.h"
#include "network.h"
#include <atomic>
#include <memory>
#include <csignal>

class Server {
private:
    ServerConfig config_;
    Logger logger_;
    Authenticator authenticator_;
    VectorProcessor processor_;
    NetworkManager network_;
    std::atomic<bool> running_;
    
public:
    Server(const ServerConfig& config);
    ~Server();
    
    bool initialize();
    void run();
    void stop();
    
private:
    void handleClient(int clientSocket, const std::string& clientIP);
    void sendEmptyResults(int clientSocket, const std::string& reason);  // Добавьте этот метод
};

class ServerInterface {
private:
    Config config_;
    std::unique_ptr<Server> server_;
    
public:
    int run(int argc, char* argv[]);
};

#endif // SERVER_H