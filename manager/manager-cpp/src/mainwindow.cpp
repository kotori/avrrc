#include "mainwindow.h"

#include <QDesktopServices>
#include <QFileDialog>
#include <QInputDialog>
#include <QJsonDocument>
#include <QMessageBox>
#include <QSerialPortInfo>

#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
  ui->setupUi(this);
  networkManager = new QNetworkAccessManager(this);

  // Sync the window title with the Git version
  this->setWindowTitle("AVRRC Ensign | Fleet Management - v" + QString(APP_VERSION));

  // 1. Initial Device Scan
  refreshPorts();

  // 2. Initial UI Population
  refreshActiveListView();
  updateLibraryListView();

  // 3. Connect Signals & Slots
  connect(&fleetManager, &FleetManager::progressUpdated, ui->progressBar,
          &QProgressBar::setValue);
  connect(ui->searchEdit, &QLineEdit::textChanged, this,
          &MainWindow::updateLibraryListView);
  connect(&fleetManager, &FleetManager::statusMessage, ui->statusbar,
          [this](const QString &message) {
            ui->statusbar->showMessage(message, 5000);
          });
  connect(networkManager, &QNetworkAccessManager::finished, this,
          &MainWindow::onUpdateResponse);
  connect(ui->actionCheck_for_Updates, &QAction::triggered, this,
          &MainWindow::checkForUpdates);
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::refreshPorts() {
  ui->portCombo->clear();

  // Cross-platform scan (Works on Linux /dev/tty and Windows COMx)
  const auto serialPortInfos = QSerialPortInfo::availablePorts();

  for (const QSerialPortInfo &portInfo : serialPortInfos) {
    QString portName = portInfo.portName();
    QString description = portInfo.description();

    // Sane naming: If description exists, use it. Otherwise, just show the
    // port.
    QString displayName = portName;
    if (!description.isEmpty()) {
      displayName += " (" + description + ")";
    }

    // Store the raw port name (COM3 or ttyUSB0) in UserRole
    ui->portCombo->addItem(displayName, portName);
  }

  if (ui->portCombo->count() == 0) {
    ui->statusbar->showMessage(
        "No Serial devices found. Check your USB connection.");
  } else {
    ui->statusbar->showMessage("Found " +
                               QString::number(ui->portCombo->count()) +
                               " available ports.");
  }
}

// Slot for Rename Button
void MainWindow::on_renameBtn_clicked() {
  QListWidgetItem *item = ui->modelList->currentItem();
  if (!item) return;

  int slot = ui->modelList->currentRow();
  bool ok;
  QString newName = QInputDialog::getText(
      this, "Rename Model", "Enter new name (Max 11 chars):", QLineEdit::Normal,
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
                                     QMessageBox::Yes | QMessageBox::No);
  if (reply == QMessageBox::Yes) {
    fleetManager.deleteLocalModel(slot);
    refreshActiveListView();
  }
}

void MainWindow::on_syncGetBtn_clicked() {
  // Correctly pulls "COM3" or "ttyUSB0" regardless of the "Pretty Name"
  // displayed
  QString port = ui->portCombo->currentData().toString();
  if (port.isEmpty()) {
    ui->statusbar->showMessage("Select a port first!");
    return;
  }

  ui->statusbar->showMessage("Syncing from Transmitter...");
  if (fleetManager.syncFromTX(port)) {
    refreshActiveListView();
    ui->statusbar->showMessage("Sync Complete!");
  } else {
    ui->statusbar->showMessage("Sync Failed! Check connection.");
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
    item->setData(Qt::UserRole,
                  boat["address"].toString());  // Store Hex ID for "Deploy"
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
    ui->statusbar->showMessage(
        "Please select a target slot in the 'Transmitter Slots' tab.");
    ui->tabWidget->setCurrentIndex(
        0);  // Switch them back to the first tab to pick a slot
    return;
  }

  // 3. Extract the Hex Address we stored in the UserRole earlier
  QString hexAddress = libraryItem->data(Qt::UserRole).toString();

  // 4. Confirm with the user (Safety first!)
  QString boatName = libraryItem->text().split(" (").first();
  auto reply = QMessageBox::question(
      this, "Deploy Model",
      QString("Overwrite Slot %1 with '%2'?").arg(targetSlot).arg(boatName),
      QMessageBox::Yes | QMessageBox::No);

  if (reply == QMessageBox::Yes) {
    if (fleetManager.deployToSlot(hexAddress, targetSlot)) {
      refreshActiveListView();  // Update the Slot list text
      ui->statusbar->showMessage("Deployed " + boatName + " to Slot " +
                                 QString::number(targetSlot));
      ui->tabWidget->setCurrentIndex(0);  // Show them the result
    } else {
      ui->statusbar->showMessage("Error: Deploy failed.");
    }
  }
}

void MainWindow::on_saveJsonBtn_clicked() {
  QString fileName = QFileDialog::getSaveFileName(this, "Export Fleet JSON", "",
                                                  "JSON Files (*.json)");
  if (!fileName.isEmpty()) {
    if (fleetManager.saveToFile(fileName)) {
      ui->statusbar->showMessage("Exported to " + fileName, 5000);
    }
  }
}

void MainWindow::on_loadJsonBtn_clicked() {
  QString fileName = QFileDialog::getOpenFileName(this, "Import Fleet JSON", "",
                                                  "JSON Files (*.json)");
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
  for (int i = 0; i < (int)names.size();
       ++i) {  // Cast to int for Windows compiler safety
    ui->modelList->addItem(names[i]);
  }
}

void MainWindow::checkForUpdates() {
  // Replace with your actual GitHub repo path
  QUrl url("https://api.github.com/repos/kotori/avrrc/releases/latest");
  QNetworkRequest request(url);

  // GitHub API requires a User-Agent header
  request.setHeader(QNetworkRequest::UserAgentHeader, "AVRRC-Manager");
  networkManager->get(request);
  ui->statusbar->showMessage("Connecting to GitHub...");
}

void MainWindow::onUpdateResponse(QNetworkReply *reply) {
  if (reply->error() != QNetworkReply::NoError) {
    ui->statusbar->showMessage("Update check failed: " + reply->errorString());
    reply->deleteLater();
    return;
  }

  QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
  QJsonObject obj = doc.object();

  QString latestTag = obj["tag_name"].toString();
  QString currentTag = QString(APP_VERSION);

  if (!latestTag.isEmpty() && latestTag != currentTag) {
    auto res =
        QMessageBox::information(this, "Update Available",
                                 QString("New version (%1) available!\nYour "
                                         "version: %2\n\nOpen download page?")
                                     .arg(latestTag)
                                     .arg(currentTag),
                                 QMessageBox::Yes | QMessageBox::No);

    if (res == QMessageBox::Yes) {
      QDesktopServices::openUrl(QUrl(obj["html_url"].toString()));
    }
  } else {
    ui->statusbar->showMessage("You are running the latest version: " +
                               currentTag);
  }
  reply->deleteLater();
}

// At the bottom of mainwindow.cpp
void MainWindow::on_actionAbout_triggered() {
    QString aboutText = QString(
        "<h3>AVRRC Ensign</h3>"
        "<p><b>Version:</b> %1</p>"
        "<p><b>Fleet Logistics:</b> Kotori</p>"
        "<p>The central hub for identifying and managing the AVR-RC fleet.</p>"
        "<p><a href='https://github.com/kotori/avrrc'>AVRRC Repository</a></p>"
    ).arg(APP_VERSION);

    QMessageBox::about(this, "About AVRRC Manager", aboutText);
}
