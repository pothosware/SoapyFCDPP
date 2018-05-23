# SoapyFCDPP

Soapy SDR plugin FUNcube Dongle Pro+

## What is this thing?

This is my SoapySDR driver for the FUNcube dongle pro+. I have tested it on Raspberry Pi. I use it for streaming IQ data to GQRX.

Unlike the gr-osmosdr it's doesn't depend on the gr-fcdproplus block but is standalone depending on libhidapi and ALSA. I believe this makes it a bit more approachable for hacking.

## Dependencies

* GNUradio
* SoapySDR
* SoapyRemote
* libasound2 (ALSA)
* libhidapi
* meson and ninja for building

## How do I use it?

```bash
git clone https://github.com/ast/SoapyFCDPP.git
cd SoapyFCDPP/SoapyFCDPP
meson build
ninja -C build

# Then copy build/libsoapyfcdpp.so to /usr/local/lib/SoapySDR/modules0.7
# Should appear in the list
SoapySDRUtil --info

```

To access USB without being root you need this:

```
# Udev rules for the Funcube Dongle Pro+ (0xfb31)
# Put this in:
# /etc/udev/rules.d/81-funcube.rules

# HIDAPI/hidraw:
KERNEL=="hidraw", ATTRS{idVendor}=="04d8", ATTRS{idProduct}=="fb31", GROUP="audio", MODE="0666", SIMLINK+="hidfcd"

# Udev rule for the Funcube Dongle to be used with libusb
SUBSYSTEMS=="usb", ATTRS{idVendor}=="04d8", ATTRS{idProduct}=="fb31", GROUP="audio", MODE="0666", SYMLINK+="usbfcd"
```

## License

Copyright 2018 Albin Stigö

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

