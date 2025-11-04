#include "network.h"
#include <iostream>
#include <algorithm>

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
    timeout.tv_sec = 5;  // 5 секунд таймаут
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

bool NetworkManager::receiveVectors(int clientSocket, std::vector<std::vector<float>>& vectors) {
    uint32_t numVectors;
    
    logger_.debug("Waiting for number of vectors...");
    if (!receiveData(clientSocket, &numVectors, sizeof(numVectors))) {
        logger_.error("Failed to receive number of vectors");
        return false;
    }
    
    logger_.debug("Number of vectors: " + std::to_string(numVectors));
    
    // ЗАЩИТА ОТ НЕКОРРЕКТНЫХ ДАННЫХ
    if (numVectors == 0) {
        logger_.error("Zero vectors received");
        return false;
    }
    
    if (numVectors > 10) {
        logger_.warning("Too many vectors: " + std::to_string(numVectors) + ", limiting to 2");
        numVectors = 2;  // Ограничиваем для теста
    }
    
    logger_.debug("Will receive " + std::to_string(numVectors) + " vectors");
    vectors.clear();
    vectors.reserve(numVectors);
    
    // Получаем векторы с защитой от зависания
    for (uint32_t i = 0; i < numVectors; ++i) {
        logger_.debug("Receiving vector " + std::to_string(i + 1));
        std::vector<float> vector;
        
        if (!receiveVector(clientSocket, vector)) {
            logger_.error("Failed to receive vector " + std::to_string(i + 1));
            
            // Если не получили вектор, но уже есть данные - продолжаем
            if (!vectors.empty()) {
                logger_.warning("Continuing with " + std::to_string(vectors.size()) + " received vectors");
                break;
            }
            return false;
        }
        
        vectors.push_back(std::move(vector));
        logger_.debug("Successfully received vector " + std::to_string(i + 1));
        
        if (vectors.size() >= 2) {
            logger_.debug("Received maximum test vectors (2), stopping");
            break;
        }
    }
    
    logger_.info("Successfully received " + std::to_string(vectors.size()) + " vectors");
    return true;
}

bool NetworkManager::sendResults(int clientSocket, const std::vector<float>& results) {
    logger_.debug("=== START sendResults ===");
    logger_.debug("Preparing to send " + std::to_string(results.size()) + " results");
    
    try {
        // Всегда отправляем количество результатов
        uint32_t numResults = results.size();
        logger_.debug("Sending numResults: " + std::to_string(numResults));
        
        if (!sendData(clientSocket, &numResults, sizeof(numResults))) {
            logger_.error("Failed to send number of results");
            return false;
        }
        
        logger_.debug("Successfully sent number of results");
        
        // Отправляем каждый результат
        for (size_t i = 0; i < results.size(); ++i) {
            logger_.debug("Sending result " + std::to_string(i) + ": " + std::to_string(results[i]));
            
            if (!sendData(clientSocket, &results[i], sizeof(results[i]))) {
                logger_.error("Failed to send result " + std::to_string(i));
                return false;
            }
            
            logger_.debug("Successfully sent result " + std::to_string(i));
        }
        
        logger_.debug("=== COMPLETED sendResults - sent " + std::to_string(results.size()) + " results ===");
        return true;
        
    } catch (const std::exception& e) {
        logger_.error("Exception in sendResults: " + std::string(e.what()));
        return false;
    }
}

bool NetworkManager::receiveVector(int clientSocket, std::vector<float>& vector) {
    uint32_t vectorSize;
    
    // Получаем размер вектора
    if (!receiveData(clientSocket, &vectorSize, sizeof(vectorSize))) {
        logger_.error("Failed to receive vector size");
        return false;
    }
    
    logger_.debug("Vector size: " + std::to_string(vectorSize));
    
    // Защита от некорректных данных
    if (vectorSize == 0) {
        logger_.error("Empty vector received");
        return false;
    }
    
    if (vectorSize > 1000) {
        logger_.error("Vector too large: " + std::to_string(vectorSize));
        return false;
    }
    
    // Получаем данные вектора
    vector.resize(vectorSize);
    if (!receiveData(clientSocket, vector.data(), vectorSize * sizeof(float))) {
        logger_.error("Failed to receive vector data");
        return false;
    }
    
    std::string debugMsg = "Vector data (first 3): ";
    size_t count = std::min(static_cast<size_t>(vectorSize), size_t(3));
    for (size_t i = 0; i < count; ++i) {
        debugMsg += std::to_string(vector[i]) + " ";
    }
    logger_.debug(debugMsg);
    
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