#pragma once
#include <QString>
#include <QMap>
#include <QJsonObject>

class I18n {
public:
    static I18n& instance();
    void setLanguage(const QString& lang);
    QString get(const QString& key) const;
    QString currentLang() const { return m_currentLang; }
    bool isRTL() const { return m_currentLang == "ar"; }
private:
    I18n();
    I18n(const I18n&) = delete;
    I18n& operator=(const I18n&) = delete;
    void loadLanguage(const QString& lang);
    QString m_currentLang;
    QMap<QString, QString> m_strings;
};

inline QString _(const QString& key) { return I18n::instance().get(key); }
