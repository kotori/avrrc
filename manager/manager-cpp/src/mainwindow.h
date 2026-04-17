#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidgetItem>
#include "fleetmanager.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void refreshPorts();
    void on_renameBtn_clicked();
    void on_deleteBtn_clicked();
    void on_syncGetBtn_clicked();
    void on_syncSetBtn_clicked();
    void on_refreshBtn_clicked() { refreshPorts(); } // Small inline slot

private:
    Ui::MainWindow *ui;
    FleetManager fleetManager;
    void refreshListView(); // Helper to update the UI list
};

#endif // MAINWINDOW_H
