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
    //app.setStyle("Fusion");

    // OR a more nautical theme...
    app.setStyleSheet(
      "QMainWindow { background-color: #101820; color: #D3D3D3; }" // Navy Black
      "QTabWidget::pane { border: 1px solid #1D2935; }"
      "QPushButton { background-color: #1D2935; color: #FFFFFF; border-radius: 2px; border: 1px solid #2C3E50; }"
      "QPushButton:hover { background-color: #2C3E50; border-color: #00A8E8; }" // Glowing Sea Blue highlight
      "QProgressBar::chunk { background-color: #00A8E8; }"
    );

    // 4. Create and show the Main Window
    MainWindow w;
    w.show();

    // 5. Start the event loop
    return app.exec();
}
