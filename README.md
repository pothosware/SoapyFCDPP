# SoapyFCDPP

Soapy SDR plugin FUNcube Dongle Pro+

https://github.com/pothosware/SoapyFCDPP/wiki

## What is this thing?

This is my SoapySDR driver for the FUNcube dongle pro+. I have tested it on Raspberry Pi. I use it for streaming IQ data to GQRX.

Unlike the gr-osmosdr it's doesn't depend on the gr-fcdproplus block but is standalone depending on libhidapi and ALSA. I believe this makes it a bit more approachable for hacking.

## Dependencies

* SoapySDR
* libasound2 (ALSA)
* libhidapi
* meson and ninja for building

### Ubuntu

```bash
sudo apt-get install libhidapi-dev libasound2-dev
```

## Build with cmake

```bash
# build
git clone https://github.com/ast/SoapyFCDPP.git
cd SoapyFCDPP
mkdir build; cd build
cmake ../
make && sudo make install
```

## Build with meson

I have only tested with the latest SoapySDR / SoapyRemote.

```bash
# build
git clone https://github.com/ast/SoapyFCDPP.git
cd SoapyFCDPP/SoapyFCDPP
meson build && cd build
ninja install
# Will put the driver in /usr/local/lib/SoapySDR/modules0.7

# Should then appear in the list:
SoapySDRUtil --info
```

```
# Run server
SoapySDRServer --bind="0.0.0.0:1234"

# Device string in GQRX
soapy=0,remote=hostname.local:1234,remote:driver=fcdpp
```

* Input rate: 192000

To access USB without being root you need this:

```
# Udev rules for the Funcube Dongle Pro+ (0xfb31)
# Put this in:
# /etc/udev/rules.d/81-funcube.rules

# Udev rule for the Funcube Dongle to be used with libusb
SUBSYSTEMS=="usb", ATTRS{idVendor}=="04d8", ATTRS{idProduct}=="fb31", GROUP="audio", MODE="0666", SYMLINK+="usbfcd"
```

## License

Boost Software License 1.0 (BSL-1.0)
