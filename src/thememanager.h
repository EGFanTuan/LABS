#ifndef THEMEMANAGER_H
#define THEMEMANAGER_H

#include <QObject>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <QDir>
#include <QWidget>
#include <QStyle>

class ThemeManager : public QObject
{
    Q_OBJECT
public:
    explicit ThemeManager(QObject *parent = nullptr);
    static void SetComponentProperties(QWidget* component, QString key, QString value);
    void SetAllPropertiesInTheme(QWidget* component, QString componentName);
    void SetSingleProperty(QWidget* component, QString componentName, QString propertyName, QString propertyValue);
    bool LoadTheme(QString path = QString(), QString themeName = "default_theme");
    void SetQssDir(QString qssPath);
    void ReloadAllProperties();

private:
    QJsonObject _theme;
    QMap<QString, QString> _globalProperties;
    QMap<QWidget*, QString> _components;
    QString _defaultThemeDir, _currentQssDir;


signals:
};

#endif // THEMEMANAGER_H
