#ifndef MODULE_COMMS_H
#define MODULE_COMMS_H

#include <Arduino.h>
#include <Wire.h>

#define CMD_START_GAME  0x01
#define CMD_END_GAME    0x02
#define CMD_GET_STATUS  0x03
#define CMD_GET_VERSION 0x04
#define CMD_SET_VERSION 0x05
#define CMD_IDENTIFY    0x06
#define CMD_SET_STATUS  0x07

#define STATUS_UNSOLVED 0x00
#define STATUS_PASSED   0x01
#define STATUS_FAILED   0x02

typedef void (*SlaveCallback)();

class Master {
public:
    Master(uint8_t version);
    void begin();
    void discoverModules();
    void startGame();
    void endGame();
    void restartFailedModule(uint8_t index);
    uint8_t getModuleStatus(uint8_t index);
    void setModuleStatus(uint8_t index, uint8_t status);
    void sendVersion(uint8_t index);
    uint8_t getVersion() const;
    void setVersion(uint8_t newVersion);
    uint8_t getModuleCount() const;

private:
    uint8_t version;
    uint8_t moduleAddresses[10]; // Store up to 10 module addresses
    uint8_t moduleCount;
    void sendCommand(uint8_t moduleAddress, uint8_t command);
};

class Slave {
public:
    Slave(uint8_t address);
    void begin();
    void setStatus(uint8_t newStatus);
    uint8_t getStatus();
    uint8_t getMasterVersion() const;

    void onGameStart(SlaveCallback callback);
    void onGameEnd(SlaveCallback callback);
    void onVersionReceived(SlaveCallback callback);

private:
    static uint8_t moduleAddress;
    static uint8_t status;
    static uint8_t masterVersion;
    static uint8_t command;

    static SlaveCallback gameStartCallback;
    static SlaveCallback gameEndCallback;
    static SlaveCallback versionReceivedCallback;

    static void receiveEvent(int numBytes);
    static void requestEvent();
};

#endif
