#include "logger.h"
#include <QDir>
#include <QDateTime>
#include <QDebug>
#include <QException>

Logger& Logger::instance() {
    static Logger inst;
    return inst;
}

Logger::Logger() {
    QString dir = qgetenv("DATA_DIR");
    if (dir.isEmpty()) dir = QDir::currentPath() + "/data";
    QDir().mkpath(dir);
    m_file.setFileName(dir + "/timeline.log");
    m_file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);
    m_stream.setDevice(&m_file);
}

Logger::~Logger() {
    if (m_file.isOpen()) m_file.close();
}

QString Logger::levelStr(LogLevel lv) {
    switch (lv) {
        case DEBUG: return "DEBUG";
        case INFO:  return "INFO";
        case WARN:  return "WARN";
        case ERROR: return "ERROR";
    }
    return "UNKNOWN";
}

void Logger::log(LogLevel level, const QString& tag, const QString& message) {
    QMutexLocker locker(&m_mutex);
    QString ts = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    QString line = QString("%1 | %2 | %3 | %4\n").arg(ts, levelStr(level), tag, message);
    m_stream << line;
    m_stream.flush();
    if (level >= WARN)
        qWarning().noquote() << line.trimmed();
    else
        qDebug().noquote() << line.trimmed();
}

void Logger::exception(const QString& tag, const QString& context) {
    QString msg = context + " - UNCAUGHT EXCEPTION";
    log(ERROR, tag, msg);
}
