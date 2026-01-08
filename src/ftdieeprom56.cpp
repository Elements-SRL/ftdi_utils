#include "ftdieeprom56.h"

FtdiEeprom56::FtdiEeprom56(std::string deviceId) :
    FtdiEeprom(deviceId) {

    this->loadData();
}

FtdiEeprom56::~FtdiEeprom56() {

}

bool FtdiEeprom56::loadData() {
    if (!this->openConnection()) {
        return false;
    }

    if (!this->loadDeviceTuple()) {
        return false;
    }

    if (!this->loadVcOffset()) {
        return false;
    }

    if (!this->closeConnection()) {
        return false;
    }

    return true;
}

bool FtdiEeprom56::loadDeviceTuple() {
    WORD firmware;
    WORD versionSubversion;

    if (!this->readEepromWord(E56_DEVICE_VER_ADDR, &versionSubversion)) {
        return false;
    }

    if (!this->readEepromWord(E56_FW_VER_ADDR, &firmware)) {
        return false;
    }

    deviceTuple.version = (DeviceVersion_t)((versionSubversion >> 8) & 0xFF);
    deviceTuple.subversion = (DeviceSubversion_t)(versionSubversion & 0xFF);
    deviceTuple.fwVersion = firmware;

    return true;
}

bool FtdiEeprom56::loadVcOffset() {
    return this->readEepromWord(E56_VC_OFFSET_ADDR, &vcOffset);
}
