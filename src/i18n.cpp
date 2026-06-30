#include "i18n.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QCoreApplication>
#include <QDir>

I18n& I18n::instance() {
    static I18n inst;
    return inst;
}

I18n::I18n() : m_currentLang("ar") {
    loadLanguage("ar");
}

void I18n::setLanguage(const QString& lang) {
    m_currentLang = lang;
    loadLanguage(lang);
}

void I18n::loadLanguage(const QString& lang) {
    m_strings.clear();
    QString localeDir = qgetenv("LOCALE_DIR");
    if (localeDir.isEmpty()) localeDir = QCoreApplication::applicationDirPath() + "/locales";
    QString path = localeDir + "/" + lang + ".json";
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    if (!doc.isObject()) return;
    QJsonObject obj = doc.object();
    for (auto it = obj.begin(); it != obj.end(); ++it)
        m_strings[it.key()] = it.value().toString();
}

QString I18n::get(const QString& key) const {
    return m_strings.value(key, key);
}
