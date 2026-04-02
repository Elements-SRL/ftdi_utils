# REQUIREMENTS
Compatibility has been verified for the following systems:
- Windows 10+
- Linux Arm

Note: Elements only provides free support for Windows 11+.
## C++
ftdi_utils requires c++17 or newer.

# WINDOWS
## Toolchain
Elements builds the ftdi_utils statically using Qt Creator with the Qt libraries 6.7.3 and MSVC 2022 [Qt Community](https://www.qt.io/download-dev)

Note: the Qt Libraries are not used. Qt Creator is used for legacy reasons.

## Drivers
Install EDR4 to make sure all the needed drivers are correctly installed [elements-ic.com](https://elements-ic.com/downloads/)
Disclaimer: EDR4 is officially supported only on windows, hence if you want to build your library for other operating systems/architectures you'll have to manually search and install the needed drivers.

## Dependencies
The ftdi_utils requires the ftd2xx library and the libmpsse, both distributed by FTDI [ftdichip.com](https://www.ftdichip.com)

Disclaimer: since FTDI does not explicitly grant the right to redistribute the source code or modified versions, the programmer has to get the code/binaries independently.

# LINUX
## Drivers
The D2XX drivers used by the commlib are available at the following link: [ftdichip.com](https://ftdichip.com/drivers/d2xx-drivers/)

Choose the compatible library for your processor and follow the instructions on the website.

For the FTDI driver to work correctly you'll also need to permanently override the kernel automatic drivers and use the FTDI's to communicate with usb ports.
To do so you can use the following lines of code.
```
sudo echo "blacklist ftdi_sio" >> /etc/modprobe.d/blacklist-ftdi.conf
sudo echo "blacklist usbserial" >> /etc/modprobe.d/blacklist-usbserial.conf
```

## Dependencies
The ftdi_utils requires the ftd2xx library and the libmpsse, both distributed by FTDI [ftdichip.com](https://www.ftdichip.com)

Disclaimer: since FTDI does not explicitly grant the right to redistribute the source code or modified versions, the programmer has to get the code/binaries independently.

# PREBUILT BINARIES
Prebuilt versions of the library are available here: [link](https://drive.proton.me/urls/N3SMDKDD2G#VplThxx6N5XC)
