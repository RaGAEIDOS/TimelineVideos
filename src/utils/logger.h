#pragma once
#include <QString>
#include <QFile>
#include <QMutex>
#include <QTextStream>

enum LogLevel { DEBUG, INFO, WARN, ERROR };

class Logger {
public:
    static Logger& instance();
    void log(LogLevel level, const QString& tag, const QString& message);
    void debug(const QString& tag, const QString& msg) { log(DEBUG, tag, msg); }
    void info(const QString& tag, const QString& msg) { log(INFO, tag, msg); }
    void warn(const QString& tag, const QString& msg) { log(WARN, tag, msg); }
    void error(const QString& tag, const QString& msg) { log(ERROR, tag, msg); }
    void exception(const QString& tag, const QString& context);

private:
    Logger();
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    QFile m_file;
    QTextStream m_stream;
    QMutex m_mutex;
    QString levelStr(LogLevel lv);
};
