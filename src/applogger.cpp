#include "applogger.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QMutex>
#include <QMutexLocker>
#include <QStandardPaths>
#include <QTextStream>

#include <QtGlobal>

#include <cstring>

namespace {

QRecursiveMutex g_logMutex;
QtMessageHandler g_previousHandler = nullptr;

QString qtTypeToCategory(QtMsgType type)
{
    switch (type) {
    case QtDebugMsg: return QStringLiteral("Qt.Debug");
    case QtInfoMsg: return QStringLiteral("Qt.Info");
    case QtWarningMsg: return QStringLiteral("Qt.Warning");
    case QtCriticalMsg: return QStringLiteral("Qt.Critical");
    case QtFatalMsg: return QStringLiteral("Qt.Fatal");
    }
    return QStringLiteral("Qt");
}

LogLevel qtTypeToLevel(QtMsgType type)
{
    switch (type) {
    case QtDebugMsg: return LogLevel::Debug;
    case QtInfoMsg: return LogLevel::Info;
    case QtWarningMsg: return LogLevel::Warning;
    case QtCriticalMsg:
    case QtFatalMsg: return LogLevel::Error;
    }
    return LogLevel::Info;
}

void qtMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &message)
{
    if (g_previousHandler)
        g_previousHandler(type, context, message);

    // Skip Qt internal font/plugin noise in the UI log — it can fire during widget layout.
    if (qstrcmp(context.category, "qt.qpa.fonts") == 0)
        return;

    const QString category = context.category && *context.category
                                 ? QString::fromUtf8(context.category)
                                 : qtTypeToCategory(type);

    AppLogger::instance().log(qtTypeToLevel(type), category, message);
}

} // namespace

AppLogger &AppLogger::instance()
{
    static AppLogger logger;
    return logger;
}

void AppLogger::initialize(const QString &logDirectory)
{
    QMutexLocker locker(&g_logMutex);
    if (m_initialized)
        return;

    QString directory = logDirectory;
    if (directory.isEmpty()) {
        directory = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        if (directory.isEmpty())
            directory = QDir::homePath() + QStringLiteral("/.celested");
    }

    QDir().mkpath(directory);
    m_logFilePath = QDir(directory).filePath(QStringLiteral("celested.log"));

    QFile file(m_logFilePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        QTextStream stream(&file);
        stream << "\n=== Session " << QDateTime::currentDateTime().toString(Qt::ISODate) << " ===\n";
    }

    g_previousHandler = qInstallMessageHandler(qtMessageHandler);
    m_initialized = true;

    info(QStringLiteral("App"), QStringLiteral("Logger initialized. Log file: %1").arg(m_logFilePath));
}

void AppLogger::shutdown()
{
    QMutexLocker locker(&g_logMutex);
    if (!m_initialized)
        return;

    info(QStringLiteral("App"), QStringLiteral("Logger shutting down."));
    qInstallMessageHandler(g_previousHandler);
    g_previousHandler = nullptr;
    m_uiSink = nullptr;
    m_initialized = false;
}

void AppLogger::setUiSink(UiSink sink)
{
    QMutexLocker locker(&g_logMutex);
    m_uiSink = std::move(sink);
}

QString AppLogger::logFilePath() const
{
    return m_logFilePath;
}

void AppLogger::log(LogLevel level, const QString &category, const QString &message)
{
    writeToFile(level, category, message);

    UiSink sink;
    {
        QMutexLocker locker(&g_logMutex);
        sink = m_uiSink;
    }

    if (sink) {
        const QString uiLine = QStringLiteral("[%1] %2: %3")
                                   .arg(levelPrefix(level), category, message);
        sink(level, uiLine);
    }
}

void AppLogger::debug(const QString &category, const QString &message)
{
    log(LogLevel::Debug, category, message);
}

void AppLogger::info(const QString &category, const QString &message)
{
    log(LogLevel::Info, category, message);
}

void AppLogger::warning(const QString &category, const QString &message)
{
    log(LogLevel::Warning, category, message);
}

void AppLogger::error(const QString &category, const QString &message)
{
    log(LogLevel::Error, category, message);
}

void AppLogger::writeToFile(LogLevel level, const QString &category, const QString &message)
{
    if (m_logFilePath.isEmpty())
        return;

    QMutexLocker locker(&g_logMutex);

    QFile file(m_logFilePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
        return;

    QTextStream stream(&file);
    stream << QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd hh:mm:ss.zzz"))
           << ' ' << levelLabel(level) << ' ' << category << ": " << message << '\n';
}

QString AppLogger::levelLabel(LogLevel level)
{
    switch (level) {
    case LogLevel::Debug: return QStringLiteral("DEBUG");
    case LogLevel::Info: return QStringLiteral("INFO");
    case LogLevel::Warning: return QStringLiteral("WARN");
    case LogLevel::Error: return QStringLiteral("ERROR");
    }
    return QStringLiteral("INFO");
}

QString AppLogger::levelPrefix(LogLevel level)
{
    switch (level) {
    case LogLevel::Debug: return QStringLiteral("DBG");
    case LogLevel::Info: return QStringLiteral("INF");
    case LogLevel::Warning: return QStringLiteral("WRN");
    case LogLevel::Error: return QStringLiteral("ERR");
    }
    return QStringLiteral("INF");
}
