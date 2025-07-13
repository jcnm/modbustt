#pragma once

#include <string>
#include <fstream>
#include <mutex>
#include <memory>

/**
 * Niveaux de log
 */
enum class LogLevel {
    DEBUG = 0,
    INFO = 1,
    WARN = 2,
    ERROR = 3
};

/**
 * Classe singleton pour la gestion des logs
 */
class Logger {
public:
    static Logger& getInstance();
    
    void setLogLevel(LogLevel level);
    void setLogFile(const std::string& filename);
    
    void debug(const std::string& message);
    void info(const std::string& message);
    void warn(const std::string& message);
    void error(const std::string& message);
    
    void log(LogLevel level, const std::string& message);

private:
    Logger() = default;
    ~Logger() = default;
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    
    void writeLog(LogLevel level, const std::string& message);
    std::string levelToString(LogLevel level) const;
    std::string getCurrentTimestamp() const;
    
    LogLevel currentLevel_ = LogLevel::INFO;
    std::unique_ptr<std::ofstream> logFile_;
    std::mutex logMutex_;
    bool logToConsole_ = true;
};

// Macros pour faciliter l'utilisation
#define LOG_DEBUG(msg) Logger::getInstance().debug(msg)
#define LOG_INFO(msg) Logger::getInstance().info(msg)
#define LOG_WARN(msg) Logger::getInstance().warn(msg)
#define LOG_ERROR(msg) Logger::getInstance().error(msg)

