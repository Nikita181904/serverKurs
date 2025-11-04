#include "processor.h"
#include <limits>
#include <cmath>
#include <sstream>

VectorProcessor::VectorProcessor(Logger& logger) : logger_(logger) {}

float VectorProcessor::calculateProduct(const std::vector<float>& vector) {
    if (vector.empty()) {
        logger_.warning("Attempt to calculate product of empty vector");
        return 0.0f;
    }
    
    // Логируем содержимое вектора
    std::stringstream ss;
    ss << "Calculating product of vector [";
    for (size_t i = 0; i < vector.size(); ++i) {
        if (i > 0) ss << ", ";
        ss << vector[i];
    }
    ss << "]";
    logger_.debug(ss.str());
    
    float product = 1.0f;
    
    for (size_t i = 0; i < vector.size(); ++i) {
        float value = vector[i];
        
        // Проверка на переполнение перед умножением
        if (checkForOverflow(product, value)) {
            logger_.warning("Overflow detected in vector product calculation");
            return -std::numeric_limits<float>::infinity(); // -inf согласно ТЗ
        }
        
        product *= value;
        
        // Проверка на переполнение после умножения
        if (std::isinf(product)) {
            logger_.warning("Overflow occurred in vector product calculation");
            return -std::numeric_limits<float>::infinity(); // -inf согласно ТЗ
        }
    }
    
    logger_.debug("Calculated product: " + std::to_string(product));
    return product;
}

std::vector<float> VectorProcessor::processVectors(const std::vector<std::vector<float>>& vectors) {
    logger_.info("Processing " + std::to_string(vectors.size()) + " vectors");
    
    std::vector<float> results;
    results.reserve(vectors.size());
    
    for (size_t i = 0; i < vectors.size(); ++i) {
        logger_.debug("Processing vector " + std::to_string(i + 1));
        float product = calculateProduct(vectors[i]);
        results.push_back(product);
        
        // Детальное логирование для каждого вектора
        std::stringstream ss;
        ss << "Vector " << (i + 1) << " result: " << product;
        logger_.info(ss.str());
    }
    
    logger_.info("Completed processing " + std::to_string(vectors.size()) + " vectors");
    return results;
}

bool VectorProcessor::checkForOverflow(float a, float b) {
    if (a == 0.0f || b == 0.0f) {
        return false;
    }
    
    float max_value = std::numeric_limits<float>::max();
    float min_value = std::numeric_limits<float>::lowest();
    
    // Проверка переполнения вверх
    if (a > 0 && b > 0) {
        return a > max_value / b;
    }
    // Проверка переполнения вниз
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