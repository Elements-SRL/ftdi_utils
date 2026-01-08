#include "ftdieeprom.h"

FtdiEeprom::FtdiEeprom(std::string deviceId) :
    deviceId(deviceId) {

}

FtdiEeprom::~FtdiEeprom() {

}

FtdiEepromId_t FtdiEeprom::getFtdiEepromId(std::string deviceId) {
    if (deviceId == "ePatch Demo") {
        return FtdiEepromIdDemo;
    }

    if (deviceId == "eP8 Demo") {
        return FtdiEepromIdDemo;
    }
    return FtdiEepromId56;
}

bool FtdiEeprom::openConnection(char channel) {
    if (connectionOpened) {
        return true;
    }

    /*! Appends the channel to the serial */
    communicationSerial = deviceId+channel;

    /*! Opens the connection with the handler */
    FT_STATUS result = FT_OpenEx((PVOID)communicationSerial.c_str(), FT_OPEN_BY_SERIAL_NUMBER, &handler);
    connectionOpened = (result == FT_OK);

    return connectionOpened;
}

bool FtdiEeprom::closeConnection() {
    if (!connectionOpened) {
        return true;
    }
    connectionOpened = (FT_Close(handler) != FT_OK);

    return !connectionOpened;
}

bool FtdiEeprom::readEepromWord(DWORD address, LPWORD result) {
    if (!connectionOpened) {
        return false;
    }
    FT_STATUS ftRet = FT_ReadEE(handler, address, result);
    if (ftRet != FT_OK) {
        return false;
    }
    return true;
}

DeviceTuple_t FtdiEeprom::getDeviceTuple() {
    return deviceTuple;
}

uint16_t FtdiEeprom::getVcOffset() {
    return vcOffset;
}
