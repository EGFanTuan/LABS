#include "widget.h"

#include <QApplication>
#include <QLocale>
#include <QTranslator>
#include <QRandomGenerator>
#include <QScreen>
#include <QWindow>
#include <QRect>

int main(int argc, char *argv[])
{
    //QCoreApplication::setAttribute(Qt::AA_UseSoftwareOpenGL);

    QApplication a(argc, argv);

    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &locale : uiLanguages) {
        const QString baseName = "LABS_" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName)) {
            a.installTranslator(&translator);
            break;
        }
    }
    Widget w;
    w.show();

    QScreen* primary = QGuiApplication::primaryScreen();
    w.windowHandle()->setScreen(primary);   
    QRect avail = primary->availableGeometry();
    w.move(avail.center() - w.rect().center());

    return a.exec();
}
