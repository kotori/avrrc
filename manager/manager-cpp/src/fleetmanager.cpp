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
    QStringList names;
    for (int i = 0; i < 20; ++i) {
        // Create a string from the 12-char name array
        QString name = QString::fromLatin1(localFleet[i].name, 12).trimmed();
        if (name.isEmpty()) name = "---";
        names.append(name);
    }
    return names;
}

void FleetManager::renameLocalModel(int slot, QString newName) {
    if (slot < 0 || slot >= 20) return;

    memset(localFleet[slot].name, 0, 12);
    QByteArray bytes = newName.toLatin1();
    memcpy(localFleet[slot].name, bytes.data(), qMin(bytes.length(), 11));
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
        emit statusMessage("Error: Could not open " + portName);
        return false;
    }

    // 1. Flush buffers and send the 'G'et command
    serial.clear();
    serial.write("G");
    if (!serial.waitForBytesWritten(1000)) return false;

    // 2. Receive 20 models sequentially
    for (int i = 0; i < 20; ++i) {
        char* buffer = reinterpret_cast<char*>(&localFleet[i]);
        qint64 bytesToRead = sizeof(ModelSettings);
        qint64 totalRead = 0;

        // Loop until the full 44-byte struct is captured for this slot
        while (totalRead < bytesToRead) {
            if (!serial.waitForReadyRead(3000)) { // 3-second timeout safety
                emit statusMessage("Error: Sync timed out at model " + QString::number(i));
                serial.close();
                return false;
            }
            qint64 chunk = serial.read(buffer + totalRead, bytesToRead - totalRead);
            if (chunk < 0) { serial.close(); return false; }
            totalRead += chunk;
        }
        emit progressUpdated(((i + 1) * 100) / 20);
    }

    serial.close();
    saveToDb(); // Persistent SQL backup
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
    // UNIQUE(address) is critical for the INSERT OR REPLACE logic
    return query.exec(
        "CREATE TABLE IF NOT EXISTS models ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "last_slot INTEGER, "
        "name TEXT, "
        "address TEXT UNIQUE, " 
        "ch1_trim INTEGER, ch2_trim INTEGER, "
        "ch3_trim INTEGER, ch4_trim INTEGER, "
        "xmin INTEGER, xmax INTEGER, ymin INTEGER, ymax INTEGER"
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
                      "(last_slot, name, address, ch1_trim, ch2_trim, ch3_trim, ch4_trim, xmin, xmax, ymin, ymax) "
                      "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");

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
