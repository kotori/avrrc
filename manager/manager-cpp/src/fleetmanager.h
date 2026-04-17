#ifndef FLEETMANAGER_H
#define FLEETMANAGER_H

#include <QObject>
#include <QSerialPort>
#include <QStringList>
#include "model_settings.h"

class FleetManager : public QObject {
    Q_OBJECT
public:
    explicit FleetManager(QObject *parent = nullptr);

    // Hardware Actions
    bool syncFromTX(QString portName);
    bool uploadToTX(QString portName);

    // Local Data Actions
    void renameLocalModel(int slot, QString newName);
    void deleteLocalModel(int slot);
    QStringList getLocalModelNames();

    // File Actions (JSON)
    bool saveToFile(QString fileName);
    bool loadFromFile(QString fileName);

signals:
    void progressUpdated(int percent);
    void statusMessage(QString message);

private:
    QSerialPort serial;
    ModelSettings localFleet[20]; // The actual data storage
};

#endif // FLEETMANAGER_H
