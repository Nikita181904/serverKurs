#include "logger.h"

Logger::Logger(const std::string& logFile) : logFile_(logFile), enabled_(true) {
    ensureFileOpen();
}

Logger::~Logger() {
    if (fileStream_.is_open()) {
        fileStream_.close();
    }
}

void Logger::ensureFileOpen() {
    if (!fileStream_.is_open()) {
        fileStream_.open(logFile_, std::ios::app);
        if (!fileStream_.is_open()) {
            enabled_ = false;
            ErrorHandler::handleWarning("Cannot open log file: " + logFile_);
        }
    }
}

void Logger::log(LogLevel level, const std::string& message) {
    if (!enabled_) return;
    
    std::lock_guard<std::mutex> lock(logMutex_);
    ensureFileOpen();
    
    if (fileStream_.is_open()) {
        fileStream_ << getCurrentTime() << " [" << levelToString(level) << "] " 
                   << message << std::endl;
        fileStream_.flush();
    }
}

void Logger::info(const std::string& message) {
    log(LogLevel::INFO, message);
}

void Logger::warning(const std::string& message) {
    log(LogLevel::WARNING, message);
}

void Logger::error(const std::string& message) {
    log(LogLevel::ERROR, message);
}

void Logger::debug(const std::string& message) {
    log(LogLevel::DEBUG, message);
}

std::string Logger::getCurrentTime() const {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

std::string Logger::levelToString(LogLevel level) const {
    switch (level) {
        case LogLevel::INFO: return "INFO";
        case LogLevel::WARNING: return "WARNING";
        case LogLevel::ERROR: return "ERROR";
        case LogLevel::DEBUG: return "DEBUG";
        default: return "UNKNOWN";
    }
}