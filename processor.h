#ifndef PROCESSOR_H
#define PROCESSOR_H

#include <vector>
#include <cstdint>
#include "logger.h"

class VectorProcessor {
private:
    Logger& logger_;
    
public:
    VectorProcessor(Logger& logger);
    
    float calculateProduct(const std::vector<float>& vector);
    std::vector<float> processVectors(const std::vector<std::vector<float>>& vectors);
    
private:
    bool checkForOverflow(float a, float b);
};

#endif // PROCESSOR_H