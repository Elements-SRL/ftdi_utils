#include "ftdieepromdemo.h"

FtdiEepromDemo::FtdiEepromDemo(std::string deviceId) :
    FtdiEeprom(deviceId) {

    this->loadData();
}

bool FtdiEepromDemo::openConnection(char channel) {
    /*! Appends the channel to the serial */
    communicationSerial = deviceId+channel;

    /*! Fake connection */
    connectionOpened = true;
    return connectionOpened;
}

bool FtdiEepromDemo::closeConnection() {
    /*! Fake disconnection */
    connectionOpened = false;
    return !connectionOpened;
}

bool FtdiEepromDemo::loadData() {
    this->openConnection();
    this->loadDeviceTuple();
    this->closeConnection();

    return true;
}

bool FtdiEepromDemo::loadDeviceTuple() {
    if (deviceId == "ePatch Demo") {
        deviceTuple.version = DeviceVersionDemo;
        deviceTuple.subversion = DeviceSubversionEPatchDemo;
        deviceTuple.fwVersion = 129;
    }
    else if (deviceId == "eP8 Demo") {
        deviceTuple.version = DeviceVersionDemo;
        deviceTuple.subversion = DeviceSubversionE8PatchDemo;
        deviceTuple.fwVersion = 129;
    }
    else if (deviceId == "eNPR Demo") {
        deviceTuple.version = DeviceVersionDemo;
        deviceTuple.subversion = DeviceSubversionEnprDemo;
        deviceTuple.fwVersion = 129;
    }
    else {
        deviceTuple.version = DeviceVersionDemo;
        deviceTuple.subversion = DeviceSubversionEnprDemo;
        deviceTuple.fwVersion = 129;
    }

    return true;
}

bool FtdiEepromDemo::loadVcOffset() {
    vcOffset = 1650.0;

    return true;
}
