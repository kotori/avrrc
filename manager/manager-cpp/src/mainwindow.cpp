#include <QInputDialog>
#include <QMessageBox>
#include <QSerialPortInfo>

#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Scan for Arduinos immediately on startup
    refreshPorts();

    // Connect logic signals to UI (Optional but recommended)
    connect(&fleetManager, &FleetManager::progressUpdated, ui->progressBar, &QProgressBar::setValue);
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::refreshPorts() {
    ui->portCombo->clear();

    // Get a list of all available serial ports on the Debian system
    const auto serialPortInfos = QSerialPortInfo::availablePorts();

    for (const QSerialPortInfo &portInfo : serialPortInfos) {
        QString portName = portInfo.portName(); // e.g. "ttyUSB0"
        QString description = portInfo.description();

        // Add to dropdown: "ttyUSB0 (Arduino Mega 2560)"
        ui->portCombo->addItem(portName + " (" + description + ")", portName);
    }

    if (ui->portCombo->count() == 0) {
        ui->statusbar->showMessage("No devices found. Check USB connection.");
    } else {
        ui->statusbar->showMessage("Found " + QString::number(ui->portCombo->count()) + " ports.");
    }
}

// Slot for Rename Button
void MainWindow::on_renameBtn_clicked() {
    QListWidgetItem *item = ui->modelList->currentItem();
    if (!item) return;

    int slot = ui->modelList->currentRow();
    bool ok;
    QString newName = QInputDialog::getText(this, "Rename Model",
                                         "Enter new name (Max 11 chars):",
                                         QLineEdit::Normal,
                                         item->text().split(": ").last(), &ok);
    if (ok && !newName.isEmpty()) {
        fleetManager.renameLocalModel(slot, newName.left(11));
        refreshListView(); // Refresh the list UI
    }
}

// Slot for Delete Button
void MainWindow::on_deleteBtn_clicked() {
    int slot = ui->modelList->currentRow();
    if (slot < 0) return;

    auto reply = QMessageBox::question(this, "Delete Model", 
                                     "Reset this slot to factory defaults?",
                                     QMessageBox::Yes|QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        fleetManager.deleteLocalModel(slot);
        refreshListView();
    }
}

// Whenever you rename or delete a model,
//  the screen updates immediately without
//  you having to re-sync with the transmitter.
void MainWindow::refreshListView() {
    ui->modelList->clear();
    // Assuming fleetManager has a way to return the current names
    QStringList names = fleetManager.getLocalModelNames();
    for(int i=0; i<names.size(); ++i) {
        ui->modelList->addItem(QString("[%1] %2").arg(i).arg(names[i]));
    }
}

void MainWindow::on_syncGetBtn_clicked() {
    QString port = ui->portCombo->currentData().toString();
    if (port.isEmpty()) return;

    ui->statusbar->showMessage("Syncing from Transmitter...");
    if (fleetManager.syncFromTX(port)) {
        refreshListView();
        ui->statusbar->showMessage("Sync Complete!");
    } else {
        ui->statusbar->showMessage("Sync Failed!");
    }
}

void MainWindow::on_syncSetBtn_clicked() {
    QString port = ui->portCombo->currentData().toString();
    if (port.isEmpty()) return;

    ui->statusbar->showMessage("Uploading to Transmitter...");
    if (fleetManager.uploadToTX(port)) {
        ui->statusbar->showMessage("Upload Successful!");
    } else {
        ui->statusbar->showMessage("Upload Failed!");
    }
}
