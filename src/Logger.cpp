#include "Logger.h"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>

Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

void Logger::setLogLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(logMutex_);
    currentLevel_ = level;
}

void Logger::setLogFile(const std::string& filename) {
    std::lock_guard<std::mutex> lock(logMutex_);
    if (logFile_) {
        logFile_->close();
    }
    logFile_ = std::make_unique<std::ofstream>(filename, std::ios::app);
    if (!logFile_->is_open()) {
        std::cerr << "Erreur: Impossible d'ouvrir le fichier de log: " << filename << std::endl;
        logFile_.reset();
    }
}

void Logger::debug(const std::string& message) {
    log(LogLevel::DEBUG, message);
}

void Logger::info(const std::string& message) {
    log(LogLevel::INFO, message);
}

void Logger::warn(const std::string& message) {
    log(LogLevel::WARN, message);
}

void Logger::error(const std::string& message) {
    log(LogLevel::ERROR, message);
}

void Logger::log(LogLevel level, const std::string& message) {
    if (level >= currentLevel_) {
        writeLog(level, message);
    }
}

void Logger::writeLog(LogLevel level, const std::string& message) {
    std::lock_guard<std::mutex> lock(logMutex_);
    
    std::string timestamp = getCurrentTimestamp();
    std::string levelStr = levelToString(level);
    std::string logLine = "[" + timestamp + "] [" + levelStr + "] " + message;
    
    // Log vers la console
    if (logToConsole_) {
        if (level >= LogLevel::ERROR) {
            std::cerr << logLine << std::endl;
        } else {
            std::cout << logLine << std::endl;
        }
    }
    
    // Log vers le fichier
    if (logFile_ && logFile_->is_open()) {
        *logFile_ << logLine << std::endl;
        logFile_->flush();
    }
}

std::string Logger::levelToString(LogLevel level) const {
    switch (level) {
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO:  return "INFO ";
        case LogLevel::WARN:  return "WARN ";
        case LogLevel::ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

std::string Logger::getCurrentTimestamp() const {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    
    return ss.str();
}

