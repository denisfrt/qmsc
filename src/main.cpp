#include <QApplication>
#include <QLocale>
#include <QObject>
#include <QTranslator>
#if !defined(WIN32) && !defined(NDEBUG)
#include <valgrind/valgrind.h>
#else
#define RUNNING_ON_VALGRIND 0
#endif

//#define QMSC_TEST

#ifndef QMSC_TEST
#include "qmsc_main_window.h"

int main(int argc, char *argv[])
{
    if (RUNNING_ON_VALGRIND) {
        // QRegularExpression issue with valgrind
        static char envvar[] = "QT_ENABLE_REGEXP_JIT=0";
        putenv(envvar);
    }
    QApplication app(argc, argv);
    app.setWindowIcon(QIcon(":/images/app.png"));

    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &locale : uiLanguages) {
        const QString baseName = "qmsc_" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName)) {
            app.installTranslator(&translator);
            break;
        }
    }
    qmsc::MscMainWindow mainWindow;
    mainWindow.show();
    return QCoreApplication::exec();
}
#else // QMSC_TEST
#include "qmsc_graphics.h"

int main()
{
    qmsc::GraphicMsc gm(nullptr);
    gm.test();
}
#endif // QMSC_TEST
