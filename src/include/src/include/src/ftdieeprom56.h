#ifndef FTDIEEPROM56_H
#define FTDIEEPROM56_H

#include "ftdieeprom.h"

#define E56_DEVICE_VER_ADDR 0x074
#define E56_FW_VER_ADDR 0x0075
#define E56_VC_OFFSET_ADDR 0x07B

class FTDIUTILSSHARED_EXPORT FtdiEeprom56 : public FtdiEeprom {
public:
    FtdiEeprom56(std::string deviceId);
    ~FtdiEeprom56();

protected:
    /*! FtdiEeprom interface */
    bool loadData() override;
    bool loadDeviceTuple() override;
    bool loadVcOffset() override;
};

#endif // FTDIEEPROM56_H
