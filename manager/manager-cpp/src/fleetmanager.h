#ifndef FLEETMANAGER_H
#define FLEETMANAGER_H

#include <QObject>
#include <QSerialPort>
#include <QJsonArray>
#include <QString>
#include "model_settings.h"

class FleetManager : public QObject {
    Q_OBJECT
public:
    explicit FleetManager(QObject *parent = nullptr);

    // Core Functions
    bool backupFleet(QString portName, QString fileName);
    bool restoreFleet(QString portName, QString fileName);

signals:
    void progressUpdated(int percent);
    void statusMessage(QString message);
    void syncFinished(bool success);

private:
    QSerialPort serial;
};

#endif // FLEETMANAGER_H
