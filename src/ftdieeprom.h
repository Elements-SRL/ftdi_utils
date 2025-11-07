#ifndef FTDIEEPROM_H
#define FTDIEEPROM_H

#include <string>

#define NOMINMAX
#ifdef _WIN32 /*! _WIN32 isolates both 32 and 64 bit windows systems, _WIN64 isolates only 64 */
#include <windows.h>
#include "ftd2xx.h"

#else /*! Same FTDI header file name also for linux and macOS */
/*! libftdi must be properly set in system folders (/usr/local/lib ... ecc) */
#include "ftd2xx.h"
#endif

#include "ftdi_utils_global.h"

#define EEPROM_ENCRYPTION_KEY_LENGTH 8

typedef enum {
    FtdiEepromId56,
    FtdiEepromIdDemo
} FtdiEepromId_t;

typedef enum {
    DeviceVersionE16 = 0x03,
    DeviceVersionE4 = 0x04,
    DeviceVersionTestBoard = 0x06,
    DeviceVersionEpatch = 0x07,
    DeviceVersionENPR = 0x08,
    DeviceVersionE1 = 0x09,
    DeviceVersionE4p = 0x0A,
    DeviceVersionE2 = 0x0C,
    DeviceVersionE8p = 0x10,
    DeviceVersionSuperDuck = 0x11,
    DeviceVersionDemo = 0xFD,
    DeviceVersionPrototype = 0xFE,
    DeviceVersionUndefined = 0xFF
} DeviceVersion_t;

typedef enum {
    /*! Subversions used for ver = 03 */
    DeviceSubversionE16FastPulses = 4,
    DeviceSubversionE16n = 5,
    DeviceSubversionE16e = 8,
    DeviceSubversionE16eth = 9,
    DeviceSubversionE16HC_PCBV00 = 10,
    DeviceSubversionE16HC_PCBV01 = 11,
    DeviceSubversionE16eArtix7PCBV01 = 12,
    DeviceSubversionE16e1L = 13,
    DeviceSubversionE16HC_PCBV01_EL06f = 14,
    DeviceSubversionE16HC_PCBV02_EL06f = 15,
    DeviceSubversionE16nEL08a = 16,

    /*! Subversions used for ver = 04 */
    DeviceSubversionE4n = 3,
    DeviceSubversionE4e = 8,
    DeviceSubversionE4nQc01a = 9,

    /*! Subversions used for ver = 06 */
    DeviceSubversionTestBoardDlp = 3,
    DeviceSubversionTestBoardEL06b = 5,
    DeviceSubversionTestBoardEL06c = 6,
    DeviceSubversionTestBoardEL06d = 7,
    DeviceSubversionTestBoardEL06e = 8,
    DeviceSubversionTestBoardEL04d = 9,
    DeviceSubversionTestBoardEL04e = 10,
    DeviceSubversionTestBoardEL04f = 11,
    DeviceSubversionTestBoardEL06f = 12,
    DeviceSubversionTestBoardQC01a_PCBV02 = 15,
    DeviceSubversionTestBoardEL09a_PCBV01 = 16,

    /*! Subversions used for ver = 07 */
    DeviceSubversionEl03D = 1,
    DeviceSubversionEl03F_4D_PCBV02 = 2,
    DeviceSubversionEl03F_4E_PCBV02 = 3,
    DeviceSubversionEl04F = 4,
    DeviceSubversionEl03F_4D_PCBV03 = 5,
    DeviceSubversionEl03F_4E_PCBV03 = 6,
    DeviceSubversionEl03F_4F_PCBV03 = 8,
    DeviceSubversionEl03F_4F_PCBV01_AnalogOut = 9,
    DeviceSubversionEl03F_4F_PCBV02 = 10,
    DeviceSubversionEl03F_4F_PCBV03b = 11,
    DeviceSubversionEl04F2_DacExt = 12,
    DeviceSubversionEl03F_4D_PCBV03b = 13,

    /*! Subversions used for ver = 08 */
    DeviceSubversionENPR = 2,
    DeviceSubversionENPRHC = 8,
    DeviceSubversionE2QcDebug = 10,
    DeviceSubversionE2QcDebugIntVcm = 11,

    /*! Subversions used for ver = 09 */
    DeviceSubversionE1bEL03C = 2,
    DeviceSubversionE1LightEL03C = 4,
    DeviceSubversionE1PlusEL03C = 5,
    DeviceSubversionE1HcEL03C = 6,
    DeviceSubversionE1LightEL03F = 7,
    DeviceSubversionE1PlusEL03F = 8,
    DeviceSubversionE1HcEL03F = 9,
    DeviceSubversionE1ULN = 10,

    /*! Subversions used for ver = 0A */
    DeviceSubversionEl04Fx4 = 1,
    DeviceSubversionEl04Fx4Patch = 2,
    DeviceSubversionEl04Fx8Patch = 3,
    DeviceSubversionEl07ABx4Patch = 5,
    DeviceSubversionEl07ABx8Patch = 6,
    DeviceSubversionEl07ABx8Patch_artix7_PCBV00_1 = 8,
    DeviceSubversionEl07ABx8Patch_artix7_PCBV00_2 = 10,
    DeviceSubversionEl07CDx8Patch_artix7_PCBV00_1 = 12,
    DeviceSubversionEl07CDx8Patch_artix7_PCBV00_2 = 14,
    DeviceSubversionEl07CDx4Patch_artix7_PCBV00_1 = 16,
    DeviceSubversionEl07CDx4Patch_artix7_PCBV00_2 = 17,
    DeviceSubversionEl07CDx4Patch_artix7_PCBV01 = 18,
    DeviceSubversionEl07CDx8Patch_artix7_PCBV01 = 19,
    DeviceSubversionEl07CDx4Patch_artix7_PCBV02 = 21,
    DeviceSubversionEl07CDx8Patch_artix7_PCBV02 = 22,

    /*! Subversions used for ver = 0C */
    DeviceSubversionE2HC = 1,

    /*! Subversions used for ver = 10 */
    DeviceSubversionE8nPatch_artix7_PCBV01 = 1,
    DeviceSubversionE8nPatch_artix7_EL07c_PCBV02 = 2,
    DeviceSubversionE8nPatch_artix7_EL07e_PCBV02 = 3,

    /*! Subversions used for ver = 11 */
    DeviceSubversionSuperDuckPcbV01 = 1,

    /*! Subversions used for ver = FD */
    DeviceSubversionEPatchDemo = 1,
    DeviceSubversionE8PatchDemo = 2,
    DeviceSubversionEnprDemo = 3,

    /*! Subversions used for ver = FE */
    DeviceSubversionProtoE1E4nBased = 1,
    DeviceSubversionProtoEl04E = 9,
    DeviceSubversionProtoEl03F_4E = 10,
    DeviceSubversionProtoE2HCExtAdc = 14,
    DeviceSubversionProtoE2HCIntAdc = 15,
    DeviceSubversionProtoENPRFairyLight = 16,
    DeviceSubversionProtoENPR2Channels = 17,
    DeviceSubversionProtoOrbitMiniSineWave = 18,
    DeviceSubversionProtoE16nSineWave = 19,
    DeviceSubversionProtoENPRNanopipette = 20,
    DeviceSubversionProtoProtoE1ULN = 21,
    DeviceSubversionProtoE4TtlPulseTrain = 22,
    DeviceSubversionProtoProtoE1ULNSplitted = 23,
    DeviceSubversionProtoE2Uln = 24,
    DeviceSubversionProtoE16nRamps = 27,
    DeviceSubversionProtoE4nTriggerIn = 28,


    /*! Subversions used for ver = 0xFF */
    DeviceSubversionUndefined = 0xFF
} DeviceSubversion_t ;

typedef struct DeviceTuple {
    DeviceVersion_t version = DeviceVersionUndefined;
    DeviceSubversion_t subversion = DeviceSubversionUndefined;
    uint32_t fwVersion = 0;
} DeviceTuple_t;

inline bool operator == (const DeviceTuple_t &a, const DeviceTuple_t &b) {
    return ((a.version == b.version) && (a.subversion == b.subversion) && (a.fwVersion == b.fwVersion));
}

inline bool operator != (const DeviceTuple_t &a, const DeviceTuple_t &b) {
    return !(a == b);
}


class FTDIUTILSSHARED_EXPORT FtdiEeprom {
public:
    FtdiEeprom(std::string deviceId);

    static FtdiEepromId_t getFtdiEepromId(std::string deviceId);
    virtual bool openConnection(char channel = 'A');
    virtual bool closeConnection();
    DeviceTuple_t getDeviceTuple();
    uint16_t getVcOffset();
    bool readEepromWord(DWORD address, LPWORD result);

protected:
    std::string deviceId;
    char communicationChannel;
    std::string communicationSerial;
    uint16_t encryptionKey[EEPROM_ENCRYPTION_KEY_LENGTH];
    DeviceTuple_t deviceTuple;
    uint16_t vcOffset;
    bool connectionOpened = false;
    FT_HANDLE handler;

    void calculateEncryptionKey();

    virtual bool loadData() = 0;
    virtual bool loadDeviceTuple() = 0;
    virtual bool loadVcOffset() = 0;
};

#endif // FTDIEEPROM_H
