#include <QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[]) {
    // 1. Initialize the Application
    QApplication app(argc, argv);

    // 2. Set Application Metadata (Useful for Linux Desktop integration)
    app.setApplicationName("AVRRC Manager");
    app.setOrganizationName("Kotori");
    app.setApplicationVersion(APP_VERSION);

    // 3. Optional: Global Dark Mode styling for a "Stealth" look
    // This makes the app look consistent across different Debian Desktop Environments
    app.setStyle("Fusion");

    // 4. Create and show the Main Window
    MainWindow w;
    w.show();

    // 5. Start the event loop
    return app.exec();
}
