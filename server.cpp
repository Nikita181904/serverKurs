#include "server.h"
#include <iostream>

// Глобальная переменная для обработки сигналов
std::atomic<bool> g_running{true};

void signalHandler(int signal) {
    g_running = false;
    std::cout << "Received signal " << signal << ", shutting down..." << std::endl;
}

Server::Server(const ServerConfig& config)
    : config_(config),
      logger_(config.logFile),
      authenticator_(logger_),
      processor_(logger_),
      network_(logger_),
      running_(false) {}

Server::~Server() {
    stop();
}

bool Server::initialize() {
    logger_.info("Initializing server for fixed client...");
    logger_.info("Client settings: 127.0.0.1:33333, user/P@ssW0rd");
    
    // Загрузка базы пользователей
    if (!authenticator_.loadUsers(config_.clientDbFile)) {
        logger_.error("Failed to load user database");
        return false;
    }
    
    // Инициализация сетевого модуля на порту 33333
    if (!network_.initialize(config_.port)) {
        logger_.error("Failed to initialize network on port " + std::to_string(config_.port));
        return false;
    }
    
    logger_.info("Server initialized successfully on port " + std::to_string(config_.port));
    logger_.info("Waiting for client connections...");
    return true;
}

void Server::run() {
    if (!initialize()) {
        throw ServerException("Failed to initialize server");
    }
    
    running_ = true;
    logger_.info("Server started and listening for connections on 127.0.0.1:" + 
                 std::to_string(config_.port));
    
    // Установка обработчиков сигналов
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
    
    // ОДНОПОТОЧНЫЙ ЦИКЛ ОБРАБОТКИ
    while (running_ && g_running) {
        std::string clientIP;
        int clientSocket = network_.acceptClient(clientIP);
        
        if (clientSocket == -1) {
            if (running_) {
                logger_.error("Failed to accept client connection");
            }
            continue;
        }
        
        // Логируем подключение клиента
        logger_.info("New client connection from: " + clientIP);
        
        handleClient(clientSocket, clientIP);
        
        // Закрываем соединение после обработки
        network_.closeClient(clientSocket);
        logger_.info("Client disconnected: " + clientIP);
        
        // Логируем готовность к следующему подключению
        logger_.info("Ready for next client connection...");
    }
    
    stop();
    logger_.info("Server stopped");
}

void Server::stop() {
    if (running_) {
        running_ = false;
        network_.shutdown();
        logger_.info("Server shutdown initiated");
    }
}

void Server::handleClient(int clientSocket, const std::string& clientIP) {
    logger_.info("=== START handling client: " + clientIP + " ===");
    
    std::vector<std::vector<float>> vectors;
    std::vector<float> results;
    
    try {
        // Получение и аутентификация логина
        std::string login;
        if (!network_.receiveLogin(clientSocket, login)) {
            logger_.error("Failed to receive login from " + clientIP);
            sendEmptyResults(clientSocket, "Login failed");
            return;
        }
        
        // Клиент всегда использует логин "user"
        if (login != "user") {
            logger_.warning("Unexpected login from " + clientIP + ": " + login + " (expected: user)");
        }
        
        if (!authenticator_.authenticateUser(clientSocket, login)) {
            logger_.warning("Authentication failed for " + clientIP + " user: " + login);
            sendEmptyResults(clientSocket, "Authentication failed");
            return;
        }
        
        logger_.info("Authentication successful for user: " + login);
        
        // Получение векторов
        if (!network_.receiveVectors(clientSocket, vectors)) {
            logger_.error("Failed to receive vectors from " + clientIP);
            sendEmptyResults(clientSocket, "Vector reception failed");
            return;
        }
        
        if (vectors.empty()) {
            logger_.warning("No vectors received from " + clientIP);
            sendEmptyResults(clientSocket, "No vectors received");
            return;
        }
        
        logger_.info("Successfully received " + std::to_string(vectors.size()) + " vectors from client");
        
        // Обработка векторов
        logger_.info("Starting vector processing...");
        results = processor_.processVectors(vectors);
        logger_.info("Vector processing completed");
        
    } catch (const std::exception& e) {
        logger_.error("Exception while handling client " + clientIP + ": " + e.what());
        sendEmptyResults(clientSocket, "Exception: " + std::string(e.what()));
        return;
    } catch (...) {
        logger_.error("Unknown exception while handling client " + clientIP);
        sendEmptyResults(clientSocket, "Unknown exception");
        return;
    }
    
    // ОТПРАВКА РЕЗУЛЬТАТОВ
    try {
        logger_.info("Sending " + std::to_string(results.size()) + " results to client");
        
        // Логируем результаты перед отправкой
        for (size_t i = 0; i < results.size(); i++) {
            logger_.info("Result " + std::to_string(i) + ": " + std::to_string(results[i]));
        }
        
        if (!network_.sendResults(clientSocket, results)) {
            logger_.error("Failed to send results to " + clientIP);
        } else {
            logger_.info("Successfully sent all results to client");
        }
        
    } catch (const std::exception& e) {
        logger_.error("Exception while sending results to " + clientIP + ": " + e.what());
    }
    
    logger_.info("=== COMPLETED handling client: " + clientIP + " ===");
}

void Server::sendEmptyResults(int clientSocket, const std::string& reason) {
    logger_.warning("Sending empty results to client: " + reason);
    try {
        std::vector<float> emptyResults;
        network_.sendResults(clientSocket, emptyResults);
        logger_.info("Empty results sent to client");
    } catch (const std::exception& e) {
        logger_.error("Failed to send empty results: " + std::string(e.what()));
    }
}

int ServerInterface::run(int argc, char* argv[]) {
    try {
        if (!config_.parseCommandLine(argc, argv)) {
            return 0;
        }
        
        ServerConfig serverConfig = config_.getConfig();
        Server server(serverConfig);
        server.run();
        
    } catch (const ConfigException& e) {
        ErrorHandler::handleError("Configuration error: " + std::string(e.what()));
        Config::showHelp();
        return 1;
    } catch (const ServerException& e) {
        ErrorHandler::handleCritical("Server error: " + std::string(e.what()));
        return 1;
    } catch (const std::exception& e) {
        ErrorHandler::handleCritical("Unexpected error: " + std::string(e.what()));
        return 1;
    }
    
    return 0;
}