#ifndef APPLOGGER_H
#define APPLOGGER_H

#include <QString>
#include <functional>

enum class LogLevel {
    Debug,
    Info,
    Warning,
    Error
};

class AppLogger
{
public:
    using UiSink = std::function<void(LogLevel level, const QString &message)>;

    static AppLogger &instance();

    void initialize(const QString &logDirectory = QString());
    void shutdown();

    void setUiSink(UiSink sink);

    void log(LogLevel level, const QString &category, const QString &message);

    void debug(const QString &category, const QString &message);
    void info(const QString &category, const QString &message);
    void warning(const QString &category, const QString &message);
    void error(const QString &category, const QString &message);

    QString logFilePath() const;

private:
    AppLogger() = default;

    void writeToFile(LogLevel level, const QString &category, const QString &message);
    static QString levelLabel(LogLevel level);
    static QString levelPrefix(LogLevel level);

    UiSink m_uiSink;
    QString m_logFilePath;
    bool m_initialized = false;
};

#define LOG_DEBUG(category, message) AppLogger::instance().debug((category), (message))
#define LOG_INFO(category, message) AppLogger::instance().info((category), (message))
#define LOG_WARN(category, message) AppLogger::instance().warning((category), (message))
#define LOG_ERROR(category, message) AppLogger::instance().error((category), (message))

#endif // APPLOGGER_H
