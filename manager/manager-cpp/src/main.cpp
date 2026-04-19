#include <QApplication>

#include "mainwindow.h"

int main(int argc, char *argv[]) {
  // 1. Initialize the Application
  QApplication app(argc, argv);

  // 2. Set Application Metadata (Useful for Linux Desktop integration)
  app.setApplicationName("AVRRC Ensign");
  app.setOrganizationName("Kotori");
  app.setApplicationVersion(APP_VERSION);

  // 3. Optional: Global Dark Mode styling for a "Stealth" look
  // This makes the app look consistent across different Debian Desktop
  // Environments
  // app.setStyle("Fusion");

  // OR a more nautical theme...
  app.setStyleSheet(
      // 1. Overall window: Soft grey background, dark text
      "QMainWindow { background-color: #E6E9ED; color: #2C3E50; font-size: "
      "13px; }"

      // 2. Tabs: High-contrast selection
      "QTabWidget::pane { border: 1px solid #BDC3C7; background-color: "
      "#F5F7FA; }"
      "QTabBar::tab { background: #D1D5DA; padding: 8px 20px; border: 1px "
      "solid #BDC3C7; border-bottom: none; }"
      "QTabBar::tab:selected { background: #FFFFFF; font-weight: bold; "
      "border-bottom: 2px solid #00A8E8; }"

      // 3. Lists: Alternating row colors for better tracking
      "QListWidget { background-color: #FFFFFF; border: 1px solid #CCD1D9; "
      "color: #333333; outline: 0; }"
      "QListWidget::item:alternate { background-color: #F9FBFC; }"
      "QListWidget::item:selected { background-color: #00A8E8; color: white; }"

      // 4. Buttons: Industrial blue with rounded edges
      "QPushButton { background-color: #4A69BD; color: white; border-radius: "
      "3px; padding: 6px 12px; font-weight: 600; }"
      "QPushButton:hover { background-color: #1E3799; }"
      "QPushButton:disabled { background-color: #BDC3C7; }"

      // 5. Inputs & Dropdowns
      "QLineEdit, QComboBox { background-color: white; border: 1px solid "
      "#AAB2BD; padding: 4px; border-radius: 2px; }"

      // 6. Progress Bar: High-visibility Sea Green
      "QProgressBar { border: 1px solid #BDC3C7; border-radius: 4px; "
      "text-align: center; background-color: #E6E9ED; }"
      "QProgressBar::chunk { background-color: #27AE60; }");

  // 4. Create and show the Main Window
  MainWindow w;
  w.show();

  // 5. Start the event loop
  return app.exec();
}
