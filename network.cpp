#include "network.h"
#include <iostream>
#include <algorithm>

// Если endian.h не доступен, определяем функции самостоятельно
#ifndef le32toh
#if __BYTE_ORDER == __LITTLE_ENDIAN
#define le32toh(x) (x)
#define htole32(x) (x)
#else
#define le32toh(x) __bswap_32(x)
#define htole32(x) __bswap_32(x)
#endif
#endif

NetworkManager::NetworkManager(Logger& logger) 
    : logger_(logger), serverSocket_(-1), initialized_(false) {}

NetworkManager::~NetworkManager() {
    shutdown();
}

bool NetworkManager::initialize(uint16_t port) {
    if (initialized_) {
        logger_.warning("Network manager already initialized");
        return true;
    }
    
    if (!createSocket()) {
        return false;
    }
    
    if (!setSocketOptions()) {
        close(serverSocket_);
        serverSocket_ = -1;
        return false;
    }
    
    if (!bindSocket(port)) {
        close(serverSocket_);
        serverSocket_ = -1;
        return false;
    }
    
    if (!startListening()) {
        close(serverSocket_);
        serverSocket_ = -1;
        return false;
    }
    
    initialized_ = true;
    logger_.info("Network manager initialized on port " + std::to_string(port));
    return true;
}

void NetworkManager::shutdown() {
    if (serverSocket_ != -1) {
        close(serverSocket_);
        serverSocket_ = -1;
        logger_.info("Network manager shutdown");
    }
    initialized_ = false;
}

bool NetworkManager::createSocket() {
    serverSocket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket_ == -1) {
        logger_.error("Failed to create socket: " + std::string(strerror(errno)));
        throw NetworkException("Cannot create socket");
    }
    return true;
}

bool NetworkManager::setSocketOptions() {
    int opt = 1;
    if (setsockopt(serverSocket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        logger_.error("Failed to set socket options: " + std::string(strerror(errno)));
        return false;
    }
    return true;
}

bool NetworkManager::bindSocket(uint16_t port) {
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);
    
    if (bind(serverSocket_, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        logger_.error("Failed to bind socket to port " + std::to_string(port) + 
                     ": " + std::string(strerror(errno)));
        throw NetworkException("Cannot bind to port " + std::to_string(port));
    }
    return true;
}

bool NetworkManager::startListening() {
    if (listen(serverSocket_, 10) < 0) {
        logger_.error("Failed to start listening: " + std::string(strerror(errno)));
        throw NetworkException("Cannot start listening");
    }
    return true;
}

int NetworkManager::acceptClient(std::string& clientIP) {
    struct sockaddr_in clientAddr;
    socklen_t clientLen = sizeof(clientAddr);
    
    int clientSocket = accept(serverSocket_, (struct sockaddr*)&clientAddr, &clientLen);
    if (clientSocket < 0) {
        logger_.error("Failed to accept client connection: " + std::string(strerror(errno)));
        return -1;
    }
    
    // Устанавливаем таймаут на операции приема данных
    struct timeval timeout;
    timeout.tv_sec = 10;
    timeout.tv_usec = 0;
    setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    
    char ipBuffer[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &clientAddr.sin_addr, ipBuffer, INET_ADDRSTRLEN);
    clientIP = ipBuffer;
    
    logger_.info("Client connected from: " + clientIP);
    return clientSocket;
}

void NetworkManager::closeClient(int clientSocket) {
    if (clientSocket != -1) {
        close(clientSocket);
    }
}

bool NetworkManager::receiveLogin(int clientSocket, std::string& login) {
    char buffer[256] = {0};
    
    logger_.debug("Waiting for login...");
    ssize_t bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
    
    if (bytesReceived <= 0) {
        if (bytesReceived == 0) {
            logger_.error("Client disconnected during login");
        } else {
            logger_.error("Failed to receive login from client: " + std::string(strerror(errno)));
        }
        return false;
    }
    
    login.assign(buffer, bytesReceived);
    logger_.debug("Received login: " + login);
    return true;
}

bool NetworkManager::receiveData(int clientSocket, void* buffer, size_t size) {
    size_t totalReceived = 0;
    char* data = static_cast<char*>(buffer);
    
    logger_.debug("Waiting for " + std::to_string(size) + " bytes...");
    
    while (totalReceived < size) {
        ssize_t bytesReceived = recv(clientSocket, data + totalReceived, 
                                   size - totalReceived, 0);
        
        if (bytesReceived <= 0) {
            if (bytesReceived == 0) {
                logger_.error("Client disconnected during data transfer");
            } else {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    logger_.error("Receive timeout - client not sending data");
                } else {
                    logger_.error("Failed to receive data: " + std::string(strerror(errno)));
                }
            }
            return false;
        }
        
        totalReceived += bytesReceived;
    }
    
    logger_.debug("Successfully received " + std::to_string(totalReceived) + " bytes");
    return true;
}

bool NetworkManager::sendData(int clientSocket, const void* data, size_t size) {
    const char* byteData = static_cast<const char*>(data);
    size_t totalSent = 0;
    
    while (totalSent < size) {
        ssize_t bytesSent = send(clientSocket, byteData + totalSent, 
                               size - totalSent, 0);
        if (bytesSent <= 0) {
            logger_.error("Failed to send data to client: " + std::string(strerror(errno)));
            return false;
        }
        totalSent += bytesSent;
    }
    
    return true;
}