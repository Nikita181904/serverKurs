#include "server.h"
#include <iostream>
#include <csignal>
#include <sstream>
#include <cmath>
#include <limits>
#include <thread>

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
      network_(logger_),
      running_(false) {
    updateActivity(); // Инициализируем время последней активности
}

Server::~Server() {
    stop();
}

bool Server::initialize() {
    logger_.info("Initializing server...");
    logger_.info("Server settings: port=" + std::to_string(config_.port) + ", user=P@ssW0rd");
    
    // Загрузка базы пользователей
    if (!authenticator_.loadUsers(config_.clientDbFile)) {
        logger_.error("Failed to load user database");
        return false;
    }
    
    // Инициализация сетевого модуля на указанном порту
    if (!network_.initialize(config_.port)) {
        logger_.error("Failed to initialize network on port " + std::to_string(config_.port));
        return false;
    }
    
    logger_.info("Server initialized successfully on port " + std::to_string(config_.port));
    logger_.info("Waiting for client connections...");
    return true;
}

void Server::updateActivity() {
    lastActivity_ = std::chrono::steady_clock::now();
}

bool Server::shouldShutdownDueToInactivity() {
    // Завершаем работу через 5 минут бездействия
    const auto timeout = std::chrono::minutes(5);
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::minutes>(now - lastActivity_);
    
    return elapsed >= timeout;
}

void Server::run() {
    if (!initialize()) {
        throw ServerException("Failed to initialize server");
    }
    
    running_ = true;
    
    // ВЫВОД СООБЩЕНИЯ О УСПЕШНОМ ЗАПУСКЕ
    std::cout << "Сервер запущен успешно" << std::endl;
    std::cout << "Порт: " << config_.port << std::endl;
    std::cout << "Файл базы клиентов: " << config_.clientDbFile << std::endl;
    std::cout << "Файл логов: " << config_.logFile << std::endl;
    std::cout << "Ожидание подключений..." << std::endl;
    std::cout << "Сервер автоматически завершит работу через 5 минут бездействия" << std::endl;
    
    logger_.info("Server started and listening for connections on port " + 
                 std::to_string(config_.port));
    logger_.info("Server will automatically shutdown after 5 minutes of inactivity");
    
    // Установка обработчиков сигналов
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
    std::signal(SIGPIPE, SIG_IGN);
    
    // ОДНОПОТОЧНЫЙ ЦИКЛ ОБРАБОТКИ
    while (running_ && g_running) {
        // Проверяем таймаут бездействия
        if (shouldShutdownDueToInactivity()) {
            std::cout << "Сервер завершает работу по таймауту бездействия (5 минут)" << std::endl;
            logger_.info("Server shutting down due to inactivity timeout");
            break;
        }
        
        // Неблокирующее ожидание подключения с таймаутом
        struct timeval timeout;
        timeout.tv_sec = 1;  // 1 секунда таймаут для accept
        timeout.tv_usec = 0;
        
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(network_.getServerSocket(), &readfds);
        
        int result = select(network_.getServerSocket() + 1, &readfds, nullptr, nullptr, &timeout);
        
        if (result < 0) {
            if (running_) {
                logger_.error("Error in select()");
            }
            continue;
        } else if (result == 0) {
            // Таймаут - продолжаем цикл для проверки бездействия
            continue;
        }
        
        // Есть подключение
        std::string clientIP;
        int clientSocket = network_.acceptClient(clientIP);
        
        if (clientSocket == -1) {
            if (running_) {
                logger_.error("Failed to accept client connection");
            }
            continue;
        }
        
        // Обновляем время активности
        updateActivity();
        
        // Логируем подключение клиента
        logger_.info("New client connection from: " + clientIP);
        
        try {
            // ОБРАБОТКА КЛИЕНТА В ОСНОВНОМ ПОТОКЕ
            handleClient(clientSocket, clientIP);
        } catch (const std::exception& e) {
            logger_.error("Exception in client handling: " + std::string(e.what()));
        } catch (...) {
            logger_.error("Unknown exception in client handling");
        }
        
        // Закрываем соединение после обработки
        network_.closeClient(clientSocket);
        logger_.info("Client disconnected: " + clientIP);
        
        // Обновляем время активности после обработки клиента
        updateActivity();
        
        // Логируем готовность к следующему подключению
        logger_.info("Ready for next client connection...");
    }
    
    stop();
    std::cout << "Сервер остановлен" << std::endl;
    logger_.info("Server stopped");
}

void Server::stop() {
    if (running_) {
        running_ = false;
        network_.shutdown();
        logger_.info("Server shutdown initiated");
    }
}

// Функция для проверки переполнения при умножении
bool checkMultiplicationOverflow(float a, float b) {
    if (a == 0.0f || b == 0.0f) {
        return false;
    }
    
    float max_value = std::numeric_limits<float>::max();
    float min_value = std::numeric_limits<float>::lowest();
    
    // Проверка переполнения вверх (a * b > max_value)
    if (a > 0 && b > 0) {
        return a > max_value / b;
    }
    // Проверка переполнения вниз (a * b < min_value)  
    else if (a < 0 && b < 0) {
        return a < max_value / b;
    }
    // Проверка отрицательного переполнения
    else if (a > 0 && b < 0) {
        return b < min_value / a;
    }
    else if (a < 0 && b > 0) {
        return a < min_value / b;
    }
    
    return false;
}

// Функция для вычисления произведения с проверкой переполнения
float calculateProductWithOverflowCheck(const std::vector<float>& vector, Logger& logger) {
    if (vector.empty()) {
        return 0.0f;
    }
    
    float product = 1.0f;
    
    for (size_t i = 0; i < vector.size(); ++i) {
        float value = vector[i];
        
        // Проверка на переполнение перед умножением
        if (checkMultiplicationOverflow(product, value)) {
            logger.warning("Overflow detected in vector product calculation");
            return -std::numeric_limits<float>::infinity(); // -inf согласно ТЗ
        }
        
        product *= value;
        
        // Проверка на переполнение после умножения
        if (std::isinf(product)) {
            logger.warning("Overflow occurred in vector product calculation");
            return -std::numeric_limits<float>::infinity(); // -inf согласно ТЗ
        }
    }
    
    return product;
}

void Server::handleClient(int clientSocket, const std::string& clientIP) {
    logger_.info("=== START handling client: " + clientIP + " ===");
    
    try {
        // Получение и аутентификация логина
        std::string login;
        if (!network_.receiveLogin(clientSocket, login)) {
            logger_.error("Failed to receive login from " + clientIP);
            return;
        }
        
        // Клиент всегда использует логин "user"
        if (login != "user") {
            logger_.warning("Unexpected login from " + clientIP + ": " + login + " (expected: user)");
        }
        
        if (!authenticator_.authenticateUser(clientSocket, login)) {
            logger_.warning("Authentication failed for " + clientIP + " user: " + login);
            return;
        }
        
        logger_.info("Authentication successful for user: " + login);
        
        // Получаем количество векторов
        uint32_t numVectors;
        logger_.debug("Waiting for number of vectors...");
        if (!network_.receiveData(clientSocket, &numVectors, sizeof(numVectors))) {
            logger_.error("Failed to receive number of vectors");
            return;
        }
        
        numVectors = le32toh(numVectors);
        logger_.info("Number of vectors: " + std::to_string(numVectors));
        
        if (numVectors == 0 || numVectors > 100) {
            logger_.error("Invalid number of vectors: " + std::to_string(numVectors));
            return;
        }
        
        // Обрабатываем каждый вектор и сразу отправляем результат
        for (uint32_t i = 0; i < numVectors; ++i) {
            logger_.info("Processing vector " + std::to_string(i + 1));
            
            // Получаем размер текущего вектора
            uint32_t vectorSize;
            logger_.debug("Waiting for size of vector " + std::to_string(i + 1));
            if (!network_.receiveData(clientSocket, &vectorSize, sizeof(vectorSize))) {
                logger_.error("Failed to receive size for vector " + std::to_string(i + 1));
                return;
            }
            
            vectorSize = le32toh(vectorSize);
            logger_.info("Vector " + std::to_string(i + 1) + " size: " + std::to_string(vectorSize));
            
            if (vectorSize == 0 || vectorSize > 1000) {
                logger_.error("Invalid vector size: " + std::to_string(vectorSize));
                return;
            }
            
            // Получаем данные вектора
            std::vector<float> vector(vectorSize);
            
            for (uint32_t j = 0; j < vectorSize; ++j) {
                float value;
                if (!network_.receiveData(clientSocket, &value, sizeof(value))) {
                    logger_.error("Failed to receive vector " + std::to_string(i + 1) + " element " + std::to_string(j));
                    return;
                }
                
                uint32_t temp;
                memcpy(&temp, &value, sizeof(float));
                temp = le32toh(temp);
                memcpy(&value, &temp, sizeof(float));
                
                vector[j] = value;
                logger_.debug("Vector " + std::to_string(i + 1) + " element " + std::to_string(j) + ": " + std::to_string(value));
            }
            
            // Логируем весь вектор
            std::string debugMsg = "Vector " + std::to_string(i + 1) + " data: [";
            for (size_t j = 0; j < vector.size(); ++j) {
                if (j > 0) debugMsg += ", ";
                debugMsg += std::to_string(vector[j]);
            }
            debugMsg += "]";
            logger_.info(debugMsg);
            
            float product = calculateProductWithOverflowCheck(vector, logger_);
            
            if (std::isinf(product)) {
                logger_.info("Vector " + std::to_string(i + 1) + " product: -inf (OVERFLOW)");
            } else {
                logger_.info("Vector " + std::to_string(i + 1) + " product: " + std::to_string(product));
            }
            
            logger_.debug("Sending result for vector " + std::to_string(i + 1) + ": " + std::to_string(product));
            
            // Конвертируем результат в little-endian
            uint32_t temp;
            memcpy(&temp, &product, sizeof(float));
            temp = htole32(temp);
            
            if (!network_.sendData(clientSocket, &temp, sizeof(temp))) {
                logger_.error("Failed to send result for vector " + std::to_string(i + 1));
                return;
            }
            
            logger_.info("Successfully sent result for vector " + std::to_string(i + 1));
        }
        
        logger_.info("Completed processing all " + std::to_string(numVectors) + " vectors");
        
    } catch (const std::exception& e) {
        logger_.error("Exception while handling client " + clientIP + ": " + e.what());
        return;
    } catch (...) {
        logger_.error("Unknown exception while handling client " + clientIP);
        return;
    }
    
    logger_.info("=== COMPLETED handling client: " + clientIP + " ===");
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
