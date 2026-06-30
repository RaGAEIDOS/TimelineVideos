#include <QApplication>
#include <QSplashScreen>
#include <QPixmap>
#include <QIcon>
#include <QFont>
#include <QPainter>
#include <QMessageBox>
#include <QDir>
#include <QDebug>
#include <QTimer>
#include <exception>
#include <csignal>
#include "mainwindow.h"
#include "utils/logger.h"

static void globalExceptionHandler() {
    try {
        throw;
    } catch (const std::exception& e) {
        Logger::instance().error("Global", QString("Unhandled exception: %1").arg(e.what()));
        QMessageBox::critical(nullptr, "Fatal Error",
            QString("An unexpected error occurred:\n%1\n\nCheck data/timeline.log for details.").arg(e.what()));
    } catch (...) {
        Logger::instance().error("Global", "Unhandled unknown exception");
        QMessageBox::critical(nullptr, "Fatal Error",
            "An unknown error occurred.\nCheck data/timeline.log for details.");
    }
}

static void sigHandler(int sig) {
    Logger::instance().error("Global", QString("Signal received: %1").arg(sig));
    QApplication::quit();
}

int main(int argc, char* argv[]) {
    std::set_terminate([](){
        globalExceptionHandler();
        std::abort();
    });
    signal(SIGSEGV, sigHandler);
    signal(SIGABRT, sigHandler);

    QApplication app(argc, argv);
    app.setApplicationName("TimelineVideo");
    app.setApplicationVersion(APP_VERSION);

    // Set data/locale dirs as qApp properties
    qputenv("DATA_DIR", DATA_DIR);
    qputenv("LOCALE_DIR", LOCALE_DIR);

    Logger::instance().info("Main", "Starting TimelineVideo " APP_VERSION);

    // --- Splash screen ---
    QPixmap splashPixmap(":/logo.png");
    if (splashPixmap.isNull()) {
        // Fallback: try loading from file system
        splashPixmap.load(QString(DATA_DIR) + "/../Img/logo-design.png");
    }
    // Scale down if too large
    if (splashPixmap.width() > 600 || splashPixmap.height() > 500)
        splashPixmap = splashPixmap.scaled(600, 500, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    QSplashScreen splash(splashPixmap);
    splash.setEnabled(false); // Don't capture input

    // Draw version text on splash
    if (!splashPixmap.isNull()) {
        QPainter painter(&splashPixmap);
        painter.setPen(QColor(180, 180, 200));
        QFont font("Segoe UI", 10);
        painter.setFont(font);
        painter.drawText(splashPixmap.rect().adjusted(10, 10, -10, -10),
                         Qt::AlignBottom | Qt::AlignRight,
                         QString("v%1").arg(APP_VERSION));
        painter.end();
        splash.setPixmap(splashPixmap);
    }

    splash.show();
    app.processEvents();

    // Show status messages on splash
    splash.showMessage(QObject::tr("Initializing..."),
                       Qt::AlignBottom | Qt::AlignCenter,
                       QColor(200, 200, 220));
    app.processEvents();

    // Set application icon from logo
    if (!splashPixmap.isNull())
        app.setWindowIcon(QIcon(splashPixmap));

    // Create and show main window, then close splash
    splash.showMessage(QObject::tr("Loading interface..."),
                       Qt::AlignBottom | Qt::AlignCenter,
                       QColor(200, 200, 220));
    app.processEvents();

    MainWindow window;
    window.setWindowIcon(QIcon(splashPixmap));

    splash.showMessage(QObject::tr("Ready!"),
                       Qt::AlignBottom | Qt::AlignCenter,
                       QColor(108, 92, 231));
    app.processEvents();

    window.show();
    splash.finish(&window);

    Logger::instance().info("Main", "Application window displayed");

    return app.exec();
}
