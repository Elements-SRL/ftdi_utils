#ifndef FTDIEEPROMDEMO_H
#define FTDIEEPROMDEMO_H

#include "ftdieeprom.h"

class FTDIUTILSSHARED_EXPORT FtdiEepromDemo : public FtdiEeprom {
public:
    FtdiEepromDemo(std::string deviceId);
    ~FtdiEepromDemo();

    bool openConnection(char channel = 'A') override;
    bool closeConnection() override;

protected:
    /*! FtdiEeprom interface */
    bool loadData() override;
    bool loadDeviceTuple() override;
    bool loadVcOffset() override;
};

#endif // FTDIEEPROMDEMO_H
