#include "thememanager.h"

ThemeManager::ThemeManager(QObject *parent)
    : QObject{parent}
    , _defaultThemeDir(QDir::currentPath() + "/themes")
    , _currentQssDir(QDir::currentPath() + "/qss")
{
    LoadTheme();
}

bool ThemeManager::LoadTheme(QString path, QString themeName){
    if(path.isEmpty()) path = _defaultThemeDir;
    QString rpath = path + '/' + themeName + ".json";
    qDebug() << rpath;
    qDebug() << path;
    QFile file(rpath);
    if(!file.open(QIODevice::ReadOnly)){
        qDebug()<<"ThemeManager: Failed to open theme file";
        return false;
    }
    QByteArray data = file.readAll();
    file.close();

    QJsonParseError error;
    _theme = QJsonDocument::fromJson(data,&error).object();
    if(error.error!=QJsonParseError::NoError){
        qDebug()<<"ThemeManager: Failed to parse theme file";
        return false;
    }
    if(_theme.isEmpty()){
        qDebug()<<"ThemeManager: Failed to load theme";
        return false;
    }
    for(auto key:_theme["global"].toObject().keys()){
        _globalProperties[key]=_theme["global"].toObject()[key].toString();
    }
    return true;
}

void ThemeManager::SetQssDir(QString qssPath){
    _currentQssDir = qssPath;
}

void ThemeManager::SetAllPropertiesInTheme(QWidget* component, QString componentName){
    QJsonObject componentTheme = _theme[componentName].toObject();
    if(componentTheme.isEmpty()){
        qDebug()<<"ThemeManager: Failed to load component theme";
        qDebug()<<"check if the component name is correct: "<<componentName;
        return;
    }
    QStringList qssParts;
    for(auto qssPartName:componentTheme.keys()){
        QFile file(_currentQssDir + '/' + qssPartName + ".qss");
        if(!file.open(QIODevice::ReadOnly)){
            qDebug()<<"ThemeManager: Failed to open qss file";
            return;
        }
        QString qssPart = file.readAll();
        file.close();
        for(auto key:_globalProperties.keys()){
            qssPart.replace("$"+key,_globalProperties[key]);
        }
        QJsonObject qssPartProperties = componentTheme[qssPartName].toObject();
        for(auto key:qssPartProperties.keys()){
            qssPart.replace("$"+key,qssPartProperties[key].toString());
        }
        qssParts.append(qssPart);
    }
    component->setStyleSheet(qssParts.join("\n"));
    _components[component] = componentName;
}

void ThemeManager::ReloadAllProperties(){
    for(auto component:_components.keys()){
        SetAllPropertiesInTheme(component,_components[component]);
    }
}

void ThemeManager::SetComponentProperties(QWidget* component, QString key, QString value){
    component->setProperty(key.toUtf8().constData(),value);
    component->style()->unpolish(component);
    component->style()->polish(component);
    component->update();
}

void ThemeManager::SetSingleProperty(QWidget* component, QString componentName, QString propertyName, QString propertyValue){
    Q_UNUSED(componentName);
    Q_UNUSED(propertyName);
    Q_UNUSED(propertyValue);
    Q_UNUSED(component);
}
