#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <fstream>
#include <mutex>
#include <chrono>
#include <iomanip>
#include <sstream>
#include "error_handler.h"

enum class LogLevel {
    INFO,
    WARNING,
    ERROR,
    DEBUG
};

class Logger {
private:
    std::string logFile_;
    std::ofstream fileStream_;
    std::mutex logMutex_;
    bool enabled_;
    
public:
    Logger(const std::string& logFile);
    ~Logger();
    
    void log(LogLevel level, const std::string& message);
    void info(const std::string& message);
    void warning(const std::string& message);
    void error(const std::string& message);
    void debug(const std::string& message);
    
private:
    std::string getCurrentTime() const;
    std::string levelToString(LogLevel level) const;
    void ensureFileOpen();
};

#endif // LOGGER_H