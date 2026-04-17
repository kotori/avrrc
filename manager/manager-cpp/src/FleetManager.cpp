#include <QSerialPort>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include "model_settings.h"

// --- BACKUP: TX -> JSON ---
void backupFleetToJson(QString portName, QString fileName) {
    QSerialPort serial;
    serial.setPortName(portName);
    serial.setBaudRate(QSerialPort::Baud9600);

    if (!serial.open(QIODevice::ReadWrite)) return;

    serial.write("G"); // Send 'G'et command
    QJsonArray fleetArray;

    for (int i = 0; i < 20; ++i) {
        ModelSettings m;
        // Wait for exactly 44 bytes
        while (serial.bytesAvailable() < sizeof(ModelSettings)) {
            serial.waitForReadyRead(100);
        }
        
        serial.read(reinterpret_cast<char*>(&m), sizeof(ModelSettings));

        // Map struct to JSON
        QJsonObject modelObj;
        modelObj["name"] = QString::fromLatin1(m.name);
        modelObj["address"] = QString::number(m.boatAddress, 16); // Hex string
        modelObj["trims"] = QJsonArray({m.trims[0], m.trims[1], m.trims[2], m.trims[3]});
        fleetArray.append(modelObj);
    }

    QJsonDocument doc(fleetArray);
    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
    }
}

// --- RESTORE: JSON -> TX ---
void restoreFleetFromJson(QString portName, QString fileName) {
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) return;

    QJsonDocument doc = QJsonDocument::fromJson(file.read());
    QJsonArray fleetArray = doc.array();

    QSerialPort serial;
    serial.setPortName(portName);
    serial.open(QIODevice::ReadWrite);
    serial.write("S"); // Send 'S'et command

    for (const QJsonValue &v : fleetArray) {
        QJsonObject obj = v.toObject();
        ModelSettings m = {};

        // Pack JSON back into raw bytes
        strncpy(m.name, obj["name"].toString().toLatin1().data(), 11);
        m.boatAddress = obj["address"].toString().toULongLong(nullptr, 16);
        
        QJsonArray trims = obj["trims"].toArray();
        for(int i=0; i<4; i++) m.trims[i] = trims[i].toInt();

        serial.write(reinterpret_cast<const char*>(&m), sizeof(ModelSettings));
        serial.waitForBytesWritten(100);
    }
}
