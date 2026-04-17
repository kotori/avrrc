void FleetManager::renameLocalModel(int slot, QString newName) {
    if (slot < 0 || slot >= 20) return;

    // localFleet is a QList<ModelSettings> or QJsonArray you maintain
    QJsonObject model = localFleet[slot].toObject();
    model["name"] = newName;
    localFleet[slot] = model;
}

void FleetManager::deleteLocalModel(int slot) {
    if (slot < 0 || slot >= 20) return;

    QJsonObject factoryDefault;
    factoryDefault["slot"] = slot;
    factoryDefault["name"] = "NEW MODEL";
    factoryDefault["address"] = "e8e8f0f0e1"; // Or a random unique hex
    factoryDefault["trims"] = QJsonArray({0, 0, 0, 0});

    localFleet[slot] = factoryDefault;
}
