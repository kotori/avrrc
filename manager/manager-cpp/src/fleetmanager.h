#ifndef FLEETMANAGER_H
#define FLEETMANAGER_H

#include <QObject>
#include <QSerialPort>
#include <QStringList>
#include <QSqlDatabase>
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
    bool exportToJson(QString fileName);

    // File Actions (Move these OUT of signals)
    bool saveToFile(QString fileName);
    bool loadFromFile(QString fileName);

    bool initDb();
    bool saveToDb();

signals:
    void progressUpdated(int percent);
    void statusMessage(QString message);

private:
    QSerialPort serial;
    ModelSettings localFleet[20]; // Array for 20 models
    QSqlDatabase db; // master sql database.
};

#endif
