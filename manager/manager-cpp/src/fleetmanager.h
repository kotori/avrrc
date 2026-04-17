#ifndef FLEETMANAGER_H
#define FLEETMANAGER_H

#include <QObject>
#include <QSerialPort>
#include <QStringList>
#include <QSqlDatabase>
#include <QVariantMap>  // Required for passing data to the UI
#include <QDateTime>
#include "model_settings.h"

class FleetManager : public QObject {
    Q_OBJECT
public:
    explicit FleetManager(QObject *parent = nullptr);

    // --- Hardware Actions ---
    bool syncFromTX(QString portName);
    bool uploadToTX(QString portName);

    // --- Local Memory Actions (The Active 20) ---
    void renameLocalModel(int slot, QString newName);
    void deleteLocalModel(int slot);
    QStringList getLocalModelNames();

    // --- SQLite Database Actions ---
    bool initDb();
    bool saveToDb();    // Saves the "Active 20" into the permanent Library

    // Library View & Search
    QList<QVariantMap> getFilteredLibrary(QString filter);

    // The "Deploy" Action: Moves a model from SQL Library -> localFleet[slot]
    bool deployToSlot(QString hexAddress, int targetSlot);

    // --- Portable File Actions (JSON Export/Import) ---
    bool saveToFile(QString fileName);   // Export active fleet to JSON
    bool loadFromFile(QString fileName);  // Import JSON into active fleet

signals:
    void progressUpdated(int percent);
    void statusMessage(QString message);

private:
    QSerialPort serial; // serial interface.
    ModelSettings localFleet[20]; // collection of models.
    QSqlDatabase db; // master database.
};

#endif
