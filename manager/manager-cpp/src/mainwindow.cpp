#include <QInputDialog>
#include <QFileDialog>
#include <QMessageBox>
#include <QSerialPortInfo>

#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // 1. Initial Device Scan
    refreshPorts();

    // 2. Initial UI Population
    // Pull current data from the local memory and the SQL database
    refreshActiveListView();
    updateLibraryListView();

    // 3. Connect Signals & Slots
    
    // Progress Bar: Link FleetManager's progress to the UI bar
    connect(&fleetManager, &FleetManager::progressUpdated, ui->progressBar, &QProgressBar::setValue);

    // Search Bar: Every time the user types, filter the Library Tab
    connect(ui->searchEdit, &QLineEdit::textChanged, this, &MainWindow::updateLibraryListView);

    // Status Messages: If the logic sends a message, show it in the status bar
    // Use a Lambda to bridge the signal to the status bar
    connect(&fleetManager, &FleetManager::statusMessage, ui->statusbar, [this](const QString &message) {
      ui->statusbar->showMessage(message, 5000); // Show message for 5 seconds
    });
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
        refreshActiveListView();
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
        refreshActiveListView();
    }
}

void MainWindow::on_syncGetBtn_clicked() {
    QString port = ui->portCombo->currentData().toString();
    if (port.isEmpty()) return;

    ui->statusbar->showMessage("Syncing from Transmitter...");
    if (fleetManager.syncFromTX(port)) {
        refreshActiveListView();
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

void MainWindow::updateLibraryListView() {
    ui->libraryList->clear();
    QString filter = ui->searchEdit->text();
    
    // Get filtered list from database
    QList<QVariantMap> boats = fleetManager.getFilteredLibrary(filter);
    
    for (const auto &boat : boats) {
        QString itemText = QString("%1 (%2) - Last Synced: %3")
            .arg(boat["name"].toString())
            .arg(boat["address"].toString())
            .arg(boat["date"].toString());
            
        QListWidgetItem *item = new QListWidgetItem(itemText);
        item->setData(Qt::UserRole, boat["address"].toString()); // Store Hex ID for "Deploy"
        ui->libraryList->addItem(item);
    }
}

void MainWindow::on_deployBtn_clicked() {
    // 1. Get the selected boat from the Library List
    QListWidgetItem *libraryItem = ui->libraryList->currentItem();
    if (!libraryItem) {
        ui->statusbar->showMessage("Please select a boat from the Library first.");
        return;
    }

    // 2. Get the target slot from the Active List
    int targetSlot = ui->modelList->currentRow();
    if (targetSlot < 0) {
        ui->statusbar->showMessage("Please select a target slot in the 'Transmitter Slots' tab.");
        ui->tabWidget->setCurrentIndex(0); // Switch them back to the first tab to pick a slot
        return;
    }

    // 3. Extract the Hex Address we stored in the UserRole earlier
    QString hexAddress = libraryItem->data(Qt::UserRole).toString();

    // 4. Confirm with the user (Safety first!)
    QString boatName = libraryItem->text().split(" (").first();
    auto reply = QMessageBox::question(this, "Deploy Model",
                                     QString("Overwrite Slot %1 with '%2'?")
                                     .arg(targetSlot).arg(boatName),
                                     QMessageBox::Yes|QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        if (fleetManager.deployToSlot(hexAddress, targetSlot)) {
            refreshActiveListView(); // Update the Slot list text
            ui->statusbar->showMessage("Deployed " + boatName + " to Slot " + QString::number(targetSlot));
            ui->tabWidget->setCurrentIndex(0); // Show them the result
        } else {
            ui->statusbar->showMessage("Error: Deploy failed.");
        }
    }
}

void MainWindow::on_saveJsonBtn_clicked() {
    QString fileName = QFileDialog::getSaveFileName(this, "Export Fleet JSON", "", "JSON Files (*.json)");
    if (!fileName.isEmpty()) {
        if (fleetManager.saveToFile(fileName)) {
            ui->statusbar->showMessage("Exported to " + fileName, 5000);
        }
    }
}

void MainWindow::on_loadJsonBtn_clicked() {
    QString fileName = QFileDialog::getOpenFileName(this, "Import Fleet JSON", "", "JSON Files (*.json)");
    if (!fileName.isEmpty()) {
        if (fleetManager.loadFromFile(fileName)) {
            refreshActiveListView();
            ui->statusbar->showMessage("Imported " + fileName, 5000);
        }
    }
}

void MainWindow::refreshActiveListView() {
    ui->modelList->clear();
    // Fetch the 20 names (with dates) from the SQLite-aware helper
    QStringList names = fleetManager.getLocalModelNames(); 
    for(int i = 0; i < names.size(); ++i) {
        ui->modelList->addItem(names[i]);
    }
}
