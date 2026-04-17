// Returns a list of all 20 names for the UI
QStringList FleetManager::getLocalModelNames() {
    QStringList names;
    for (int i = 0; i < 20; ++i) {
        // We use fromLatin1 because the Arduino 'char' array isn't
        // guaranteed to be null-terminated if it uses all 12 bytes
        names.append(QString::fromLatin1(localFleet[i].name, 12).trimmed());
    }
    return names;
}

// Logic for the Rename action we wrote earlier
void FleetManager::renameLocalModel(int slot, QString newName) {
    if (slot < 0 || slot >= 20) return;

    // Zero out the name first to ensure no leftover characters
    memset(localFleet[slot].name, 0, 12);

    // Copy the new name into the packed struct
    QByteArray bytes = newName.toLatin1();
    memcpy(localFleet[slot].name, bytes.data(), qMin(bytes.length(), 11));
}

// Logic for the Delete action
void FleetManager::deleteLocalModel(int slot) {
    if (slot < 0 || slot >= 20) return;

    memset(&localFleet[slot], 0, sizeof(ModelSettings));
    memcpy(localFleet[slot].name, "NEW MODEL", 9);
    localFleet[slot].boatAddress = 0xE8E8F0F0E1LL + slot; // Default ID
    localFleet[slot].xMin = 0;
    localFleet[slot].xMax = 1023;
    localFleet[slot].yMin = 0;
    localFleet[slot].yMax = 1023;
}
