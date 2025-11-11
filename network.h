#ifndef NETWORK_H
#define NETWORK_H

#include <cstdint>
#include <vector>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <algorithm>
#include "logger.h"
#include "error_handler.h"

class NetworkManager {
private:
    Logger& logger_;
    int serverSocket_;
    bool initialized_;
    
public:
    NetworkManager(Logger& logger);
    ~NetworkManager();
    
    bool initialize(uint16_t port);
    void shutdown();
    int acceptClient(std::string& clientIP);
    void closeClient(int clientSocket);
    
    // Методы для работы с клиентом
    bool receiveLogin(int clientSocket, std::string& login);
    
    // Публичные методы для доступа к базовым операциям
    bool receiveData(int clientSocket, void* buffer, size_t size);
    bool sendData(int clientSocket, const void* data, size_t size);
    
    // Метод для получения серверного сокета (для select)
    int getServerSocket() const { return serverSocket_; }
    
private:
    bool createSocket();
    bool bindSocket(uint16_t port);
    bool startListening();
    bool setSocketOptions();
};

#endif // NETWORK_H