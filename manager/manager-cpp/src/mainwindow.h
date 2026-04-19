#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QJsonDocument>
#include <QJsonObject>
#include <QListWidgetItem>
#include <QMainWindow>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>

#include "fleetmanager.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
  Q_OBJECT

 public:
  MainWindow(QWidget *parent = nullptr);
  ~MainWindow();

 private slots:
  // Serial & Device Management
  void refreshPorts();
  void on_refreshBtn_clicked() { refreshPorts(); }

  // Tab 1: Active Transmitter Slot Actions
  void on_syncGetBtn_clicked();
  void on_syncSetBtn_clicked();
  void on_renameBtn_clicked();
  void on_deleteBtn_clicked();

  // Tab 2: Library & Search Actions
  void updateLibraryListView();   // Filters the SQL library via Search Bar
  void on_deployBtn_clicked();    // Moves Library model -> Active Slot
  void on_saveJsonBtn_clicked();  // Export
  void on_loadJsonBtn_clicked();  // Import

  void checkForUpdates();                       // Parse our repo for updates.
  void onUpdateResponse(QNetworkReply *reply);  // Parse update response.

  void showAboutDialog();  // Triggered by the Menu Bar

 private:
  Ui::MainWindow *ui;
  FleetManager fleetManager;

  QLabel *statusLamp;
  void updateConnectionStatus(bool connected);
  QNetworkAccessManager *networkManager;

  // UI Refresh Helpers
  void refreshActiveListView();   // Updates the 20-slot list
  void refreshLibraryListView();  // Initial fill of the SQL list
};

#endif  // MAINWINDOW_H
