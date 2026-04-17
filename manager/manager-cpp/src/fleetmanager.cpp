#include "fleetmanager.h"
#include <cstring>   // For memset and memcpy
#include <QThread>   // For sleeping
#include <QtGlobal>  // For qMin

FleetManager::FleetManager(QObject *parent) : QObject(parent) {
    // Initialize serial port settings
    serial.setBaudRate(QSerialPort::Baud9600);
    serial.setDataBits(QSerialPort::Data8);
    serial.setParity(QSerialPort::NoParity);
    serial.setStopBits(QSerialPort::OneStop);
    serial.setFlowControl(QSerialPort::NoFlowControl);

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
    if (!serial.open(QIODevice::ReadWrite)) return false;

    serial.write("G"); // Send Get command

    for (int i = 0; i < 20; ++i) {
        // Wait for 44 bytes per model
        while (serial.bytesAvailable() < sizeof(ModelSettings)) {
            if (!serial.waitForReadyRead(2000)) {
                serial.close();
                return false;
            }
        }
        serial.read(reinterpret_cast<char*>(&localFleet[i]), sizeof(ModelSettings));
        emit progressUpdated(((i + 1) * 100) / 20);
    }

    serial.close();
    return true;
}

bool FleetManager::uploadToTX(QString portName) {
    serial.setPortName(portName);
    if (!serial.open(QIODevice::ReadWrite)) return false;

    serial.write("S"); // Send Set command
    // Small delay to let Arduino enter Sync Mode
    QThread::msleep(100); 

    for (int i = 0; i < 20; ++i) {
        serial.write(reinterpret_cast<const char*>(&localFleet[i]), sizeof(ModelSettings));
        serial.waitForBytesWritten(1000);
        emit progressUpdated(((i + 1) * 100) / 20);
    }

    serial.close();
    return true;
}
