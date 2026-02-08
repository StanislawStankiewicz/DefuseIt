#ifndef MODULE_COMMS_H
#define MODULE_COMMS_H

#include <Arduino.h>
#include <Wire.h>

#define CMD_START_GAME  0x01
#define CMD_END_GAME    0x02
#define CMD_GET_STATUS  0x03
#define CMD_SET_STATUS  0x04
#define CMD_GET_VERSION 0x05
#define CMD_SET_VERSION 0x06
#define CMD_IDENTIFY    0x07
#define CMD_SET_REMAINING_SECONDS 0x08

#define CMD_UNKNOWN    0xFF

#define STATUS_UNSOLVED 0x01
#define STATUS_PASSED   0x02
#define STATUS_FAILED   0x03

typedef void (*GameLoop)();
typedef void (*ResetState)();

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
    void sendRemainingSeconds(uint8_t moduleAddress, uint16_t remainingSeconds);
    uint8_t getVersion() const;
    void setVersion(uint8_t newVersion);
    uint8_t getModuleCount() const;
    uint8_t getModuleAddress(uint8_t index) const;
    void sendCommand(uint8_t moduleAddress, uint8_t command);

private:
    uint8_t version;
    uint8_t moduleAddresses[11];
    uint8_t moduleCount;
};

typedef void (*SlaveCallback)();

class Slave {
public:
    Slave(uint8_t address, GameLoop loop, ResetState setState, uint8_t pin);
    void begin();
    uint8_t getVersion();
    uint16_t getRemainingSeconds();
    static void startGame();
    void slaveLoop();
    void pass();
    void fail();
    static void endGame();

private:
    static uint8_t moduleAddress;
    static uint8_t status;
    static uint8_t masterVersion;
    static uint16_t remainingSeconds;
    static uint8_t mistakes;
    static uint8_t command;
    static uint8_t led_pin;
    static bool isGameActive;

    static GameLoop gameLoop;
    static ResetState resetState;
    static 

    static void receiveEvent(int numBytes);
    static void requestEvent();
};

#endif
