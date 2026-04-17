#include "fleetmanager.h"
#include <cstring>   // For memset and memcpy
#include <QThread>   // For sleeping
#include <QtGlobal>  // For qMin
#include <QSqlDatabase> // For Sqlite3 access
#include <QSqlQuery>
#include <QSqlError>
#include <QStandardPaths>
#include <QDir>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

FleetManager::FleetManager(QObject *parent) : QObject(parent) {
    // Initialize serial port settings
    serial.setBaudRate(QSerialPort::Baud9600);
    serial.setDataBits(QSerialPort::Data8);
    serial.setParity(QSerialPort::NoParity);
    serial.setStopBits(QSerialPort::OneStop);
    serial.setFlowControl(QSerialPort::NoFlowControl);

    initDb();
    // Clear local memory
    memset(localFleet, 0, sizeof(localFleet));
}

QStringList FleetManager::getLocalModelNames() {
    QStringList displayList;

    for (int i = 0; i < 20; ++i) {
        QSqlQuery query;
        query.prepare("SELECT name, last_synced FROM models WHERE last_slot = ?");
        query.addBindValue(i);

        QString displayName;
        if (query.exec() && query.next()) {
            QString name = query.value(0).toString();
            // Format the timestamp to be shorter (e.g., 2024-05-20)
            QString date = query.value(1).toDateTime().toString("yyyy-MM-dd");
            displayName = QString("[%1] %2 (%3)").arg(i).arg(name).arg(date);
        } else {
            // Fallback for empty slots
            displayName = QString("[%1] --- Empty Slot ---").arg(i);
        }
        displayList.append(displayName);
    }
    return displayList;
}

void FleetManager::renameLocalModel(int slot, QString newName) {
    if (slot < 0 || slot >= 20) return;
    memset(localFleet[slot].name, 0, 12); // Clears all 12 bytes
    QByteArray bytes = newName.toLatin1();
    memcpy(localFleet[slot].name, bytes.data(), qMin(bytes.length(), 11));
    localFleet[slot].name[11] = '\0'; // Explicitly guarantee null termination
}

void FleetManager::deleteLocalModel(int slot) {
    if (slot < 0 || slot >= 20) return;

    memset(&localFleet[slot], 0, sizeof(ModelSettings));
    memcpy(localFleet[slot].name, "NEW MODEL", 9);
    localFleet[slot].boatAddress = 0xE8E8F0F0E1LL + slot;
    localFleet[slot].xMin = 0;
    localFleet[slot].xMax = 1023;
    localFleet[slot].yMin = 0;
    localFleet[slot].yMax = 1023;
}

bool FleetManager::syncFromTX(QString portName) {
    serial.setPortName(portName);
    serial.setBaudRate(QSerialPort::Baud9600);
    
    if (!serial.open(QIODevice::ReadWrite)) {
        emit statusMessage("Error: Could not open port " + portName);
        return false;
    }

    // 1. Flush buffers and trigger the Mega
    serial.clear();
    serial.write("G");
    if (!serial.waitForBytesWritten(1000)) return false;

    // 2. The "Total Read" Loop
    for (int i = 0; i < 20; ++i) {
        char* ptr = reinterpret_cast<char*>(&localFleet[i]);
        qint64 bytesToRead = sizeof(ModelSettings); // 44 bytes
        qint64 totalRead = 0;

        // Keep reading until this specific 44-byte struct is complete
        while (totalRead < bytesToRead) {
            // Wait up to 3 seconds for data to arrive in the Linux buffer
            if (!serial.waitForReadyRead(3000)) {
                emit statusMessage("Sync Timeout at Model " + QString::number(i));
                serial.close();
                return false;
            }

            // Read only what is missing for the current struct
            qint64 chunk = serial.read(ptr + totalRead, bytesToRead - totalRead);
            if (chunk <= 0) break; 
            
            totalRead += chunk;
        }
        
        emit progressUpdated(((i + 1) * 100) / 20);
    }

    serial.close();
    saveToDb(); // Auto-archive to SQLite
    return true;
}

bool FleetManager::uploadToTX(QString portName) {
    serial.setPortName(portName);
    if (!serial.open(QIODevice::ReadWrite)) return false;

    // 1. Enter Sync Mode
    serial.write("S");
    serial.waitForBytesWritten(1000);
    
    // Give the Mega 100ms to enter its 'S' loop and clear its buzzer
    QThread::msleep(100); 

    for (int i = 0; i < 20; ++i) {
        const char* buffer = reinterpret_cast<const char*>(&localFleet[i]);
        serial.write(buffer, sizeof(ModelSettings));
        
        // Wait for hardware buffer to empty before sending the next boat
        if (!serial.waitForBytesWritten(1000)) {
            serial.close();
            return false;
        }
        emit progressUpdated(((i + 1) * 100) / 20);
    }

    serial.close();
    return true;
}

bool FleetManager::initDb() {
    // Store the DB in the standard Linux local data path (~/.local/share/avrrc/)
    QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(path);

    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(path + "/fleet_library.db");

    if (!db.open()) return false;

    QSqlQuery query;
    // PRIMARY KEY(address) is critical for the INSERT OR REPLACE logic
    return query.exec(
      "CREATE TABLE IF NOT EXISTS models ("
      "address TEXT PRIMARY KEY, "
      "last_slot INTEGER, "
      "name TEXT, "
      "ch1_trim INTEGER, "
      "ch2_trim INTEGER, "
      "ch3_trim INTEGER, "
      "ch4_trim INTEGER, "
      "xmin INTEGER, "
      "xmax INTEGER, "
      "ymin INTEGER, "
      "ymax INTEGER, "
      "last_synced DATETIME DEFAULT CURRENT_TIMESTAMP"
      ")"
    );
}

bool FleetManager::saveToDb() {
    if (!db.isOpen()) return false;

    QSqlQuery query;
    db.transaction(); // Critical for performance over 20 rows

    for (int i = 0; i < 20; ++i) {
        // 'INSERT OR REPLACE' handles the "Upsert" based on the UNIQUE address
        query.prepare("INSERT OR REPLACE INTO models "
          "(last_slot, name, address, ch1_trim, ch2_trim, ch3_trim, ch4_trim, "
          "xmin, xmax, ymin, ymax, last_synced) "
          "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, CURRENT_TIMESTAMP)");

        query.addBindValue(i);
        query.addBindValue(QString::fromLatin1(localFleet[i].name, 12).trimmed());
        query.addBindValue(QString::number(localFleet[i].boatAddress, 16)); // Hex string
        query.addBindValue(localFleet[i].trims[0]);
        query.addBindValue(localFleet[i].trims[1]);
        query.addBindValue(localFleet[i].trims[2]);
        query.addBindValue(localFleet[i].trims[3]);
        query.addBindValue(localFleet[i].xMin);
        query.addBindValue(localFleet[i].xMax);
        query.addBindValue(localFleet[i].yMin);
        query.addBindValue(localFleet[i].yMax);

        if (!query.exec()) {
            db.rollback();
            return false;
        }
    }
    return db.commit();
}

bool FleetManager::exportToJson(QString filePath) {
    QJsonArray root;
    for (int i = 0; i < 20; ++i) {
        QJsonObject boat;
        boat["name"] = QString::fromLatin1(localFleet[i].name, 12).trimmed();
        boat["address"] = QString::number(localFleet[i].boatAddress, 16);
        boat["trims"] = QJsonArray({localFleet[i].trims[0], localFleet[i].trims[1], 
                                    localFleet[i].trims[2], localFleet[i].trims[3]});
        root.append(boat);
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) return false;
    file.write(QJsonDocument(root).toJson());
    return true;
}
