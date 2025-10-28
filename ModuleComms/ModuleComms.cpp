#include "ModuleComms.h"

Master::Master(uint8_t version) : version(version), moduleCount(0) {}

void Master::begin() {
    Wire.begin();
}

void Master::discoverModules() {
    moduleCount = 0;

    for (uint8_t address = 1; address < 127; address++) {
        Wire.beginTransmission(address);
        Wire.write(CMD_IDENTIFY);
        Wire.endTransmission();

        delay(2);

        Wire.requestFrom(address, (uint8_t)1);  // Ask for a response

        if (Wire.available() && Wire.read() == CMD_IDENTIFY) {
            if (moduleCount < 10) {
                moduleAddresses[moduleCount++] = address;
            }
        }
    }
}

void Master::startGame() {
    for (uint8_t i = 0; i < moduleCount; i++) {
        sendCommand(moduleAddresses[i], CMD_START_GAME);
    }
}

void Master::endGame() {
    for (uint8_t i = 0; i < moduleCount; i++) {
        sendCommand(moduleAddresses[i], CMD_END_GAME);
    }
}

void Master::restartFailedModule(uint8_t index) {
    if (index >= moduleCount) return;
    setModuleStatus(index, STATUS_UNSOLVED);
}

void Master::setModuleStatus(uint8_t index, uint8_t status) {
    if (index >= moduleCount) return;
    Wire.beginTransmission(moduleAddresses[index]);
    Wire.write(CMD_SET_STATUS);
    Wire.write(status);
    Wire.endTransmission();
}

uint8_t Master::getModuleStatus(uint8_t index) {
    if (index >= moduleCount) 
        return STATUS_UNSOLVED;
    Wire.beginTransmission(moduleAddresses[index]);
    Wire.write(CMD_GET_STATUS);
    Wire.endTransmission();
    Wire.requestFrom(moduleAddresses[index], (uint8_t)1);
    return Wire.available() ? Wire.read() : STATUS_UNSOLVED;
}

void Master::sendVersion(uint8_t index) {
    if (index >= moduleCount) return;
    Wire.beginTransmission(moduleAddresses[index]);
    Wire.write(CMD_SET_VERSION);
    Wire.write((uint8_t)(version >> 8));
    Wire.write((uint8_t)(version & 0xFF));
    Wire.endTransmission();
}

uint8_t Master::getVersion() const {
    return version;
}

void Master::setVersion(uint8_t newVersion) {
    version = newVersion;
}

uint8_t Master::getModuleCount() const {
    return moduleCount;
}

void Master::sendCommand(uint8_t moduleAddress, uint8_t command) {
    Wire.beginTransmission(moduleAddress);
    Wire.write(command);
    Wire.endTransmission();
}

// SLAVE
uint8_t Slave::moduleAddress = 0;
uint8_t Slave::status = STATUS_UNSOLVED;
uint8_t Slave::masterVersion = 0;
uint8_t Slave::command = 0;

SlaveCallback Slave::gameStartCallback = nullptr;
SlaveCallback Slave::gameEndCallback = nullptr;
SlaveCallback Slave::versionReceivedCallback = nullptr;

Slave::Slave(uint8_t address) {
    moduleAddress = address;
}

void Slave::begin() {
    Wire.begin(moduleAddress);
    Wire.onReceive(receiveEvent);
    Wire.onRequest(requestEvent);
}

void Slave::setStatus(uint8_t newStatus) {
    status = newStatus;
}

uint8_t Slave::getStatus() {
    return status;
}

uint8_t Slave::getMasterVersion() const {
    return masterVersion;
}

void Slave::onGameStart(SlaveCallback callback) {
    gameStartCallback = callback;
}

void Slave::onGameEnd(SlaveCallback callback) {
    gameEndCallback = callback;
}

void Slave::onVersionReceived(SlaveCallback callback) {
    versionReceivedCallback = callback;
}

void Slave::receiveEvent(int numBytes) {
    if (numBytes < 1) return;  // Ignore empty messages

    command = Wire.read();  // Store last received command

    switch (command) {
        case CMD_START_GAME:
            if (gameStartCallback) gameStartCallback();
            break;

        case CMD_END_GAME:
            if (gameEndCallback) gameEndCallback();
            break;

        case CMD_SET_VERSION:
            if (Wire.available() >= 1) {
                masterVersion = Wire.read();
                if (versionReceivedCallback) versionReceivedCallback();
            }
            break;

        case CMD_SET_STATUS:
            if (Wire.available() >= 1) {
                status = Wire.read();
            }
            break;

        case CMD_GET_STATUS:
            break;

        default:
            break;
    }
}

void Slave::requestEvent() {
    if (command == CMD_GET_STATUS) {
        Wire.write(status);  // Respond with current status
    } else if (command == CMD_IDENTIFY) {
        Wire.write(CMD_IDENTIFY);  // Respond with IDENTIFY only during scan
    } else {
        Wire.write(0x00);  // Default unknown response
    }
}
