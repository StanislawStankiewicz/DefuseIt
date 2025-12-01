#include "ModuleComms.h"

Master::Master(uint8_t version) : version(version), moduleCount(0) {}

void Master::begin() {
    Wire.begin();
}

void Master::sendVersion(uint8_t index) {
    if (index >= moduleCount) return;
    Wire.beginTransmission(moduleAddresses[index]);
    Wire.write(CMD_SET_VERSION);
    Wire.write(version);
    Wire.endTransmission();
}

void Master::discoverModules() {
    moduleCount = 0;

    for (uint8_t address = 1; address < 127; address++) {
        Serial.println(address, HEX);
        Wire.beginTransmission(address);
        Wire.write(CMD_IDENTIFY);
        int result = Wire.endTransmission();

        delay(2);

        Wire.requestFrom(address, (uint8_t)1);

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
        sendVersion(i);
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

uint8_t Master::getVersion() const {
    return version;
}

void Master::setVersion(uint8_t newVersion) {
    version = newVersion;
}

uint8_t Master::getModuleCount() const {
    return moduleCount;
}

uint8_t Master::getModuleAddress(uint8_t index) const {
    if (index >= moduleCount) return 0;
    return moduleAddresses[index];
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
uint8_t Slave::mistakes = 0;
uint8_t Slave::led_pin;
bool Slave::isGameActive = false;

GameLoop Slave::gameLoop = nullptr;
ResetState Slave::resetState = nullptr;

Slave::Slave(uint8_t address, GameLoop loop, ResetState resetState, uint8_t pin) {
    moduleAddress = address;
    led_pin = pin;
    gameLoop = loop;
    Slave::resetState = resetState;
    pinMode(led_pin, OUTPUT);
}

void Slave::begin() {
    Wire.begin(moduleAddress);
    Wire.onReceive(receiveEvent);
    Wire.onRequest(requestEvent);
}

uint8_t Slave::getVersion() {
    return masterVersion;
}

void Slave::startGame() {
    digitalWrite(led_pin, LOW);
    resetState();
    status = STATUS_UNSOLVED;
    isGameActive = true;
}

void Slave::slaveLoop() {
    if (isGameActive) {
        gameLoop();
    }
}

void Slave::pass() {
    status = STATUS_PASSED;
    isGameActive = false;
    digitalWrite(led_pin, HIGH);
    resetState();
}

void Slave::fail() {
    status = STATUS_FAILED;
    isGameActive = false;
}

void Slave::endGame() {
    digitalWrite(led_pin, LOW);
    isGameActive = false;
    resetState();
}

void Slave::receiveEvent(int numBytes) {
    if (numBytes < 1) return;

    command = Wire.read();

    switch (command) {
        case CMD_START_GAME:
            Slave::startGame();
            break;

        case CMD_END_GAME:
            Slave::endGame();
            break;

        case CMD_SET_VERSION:
            if (Wire.available() >= 1) {
                masterVersion = Wire.read();
            }
            break;

        case CMD_SET_STATUS:
            if (Wire.available() >= 1) {
                status = Wire.read();
            }
            break;

        default:
            break;
    }
}

void Slave::requestEvent() {
    if (command == CMD_GET_STATUS) {
        Wire.write(status);
    } else if (command == CMD_IDENTIFY) {
        Wire.write(CMD_IDENTIFY);
    } else {
        Wire.write(CMD_UNKNOWN);
    }
}
