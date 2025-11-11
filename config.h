#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <cstdint>
#include "error_handler.h"

struct ServerConfig {
    std::string clientDbFile = "/etc/vcalc.conf";
    std::string logFile = "/var/log/vcalc.log";
    uint16_t port = 33333;  // Значение по умолчанию
    
    bool validate() const;
};

class Config {
private:
    ServerConfig config_;
    
public:
    bool parseCommandLine(int argc, char* argv[]);
    ServerConfig getConfig() const { return config_; }
    static void showHelp();
    
private:
    void setClientDbFile(const std::string& filename);
    void setLogFile(const std::string& filename);
    void setPort(const std::string& portStr);
};

#endif // CONFIG_H