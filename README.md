# SoapyFCDPP

Soapy SDR plugin FUNcube Dongle Pro+

https://github.com/pothosware/SoapyFCDPP/wiki

## What is this thing?

This is a SoapySDR driver for the FUNcube dongle pro+. It has been tested it on Raspberry Pi 3 & Orange Pi Zero LTS, typically used for streaming IQ data to GQRX.

Unlike the gr-osmosdr it's doesn't depend on the gr-fcdproplus block but is standalone depending on libhidapi and ALSA. I believe this makes it a bit more approachable for hacking.

If you intend to use this as a remote front-end, remember to build _on your target system_, not your local machine!

## Dependencies

* SoapySDR
* libasound2 (ALSA)
* libhidapi
* cmake or meson and ninja for building

### Ubuntu / Debian

(if you don't already have SoapySDR installed from source)
```bash
sudo apt-get install libsoapysdr-dev soapysdr-tools
```
(other dependencies)
```bash
sudo apt-get install libhidapi-dev libasound2-dev
```

## Build with cmake

Tested on Debian 10 with out-of-the-box SoapySDR and other dependencies.

```bash
# build
git clone https://github.com/pothosware/SoapyFCDPP.git
cd SoapyFCDPP
mkdir build; cd build
cmake ../
make && sudo make install
# Will put the driver in /usr/local/lib/SoapySDR/module0.6
```

## Build with meson

Only tested with the latest SoapySDR / SoapyRemote from source.

```bash
# build
git clone https://github.com/pothosware/SoapyFCDPP.git
cd SoapyFCDPP/SoapyFCDPP
meson build && cd build
ninja install
# Will put the driver in /usr/local/lib/SoapySDR/modules0.7
```

## Testing

This driver should now appear in the driver list as `fcdpp`:

```bash
SoapySDRUtil --info
```

### Local use

You can access the driver directly from a SoapySDR client, a typical device string is:
```bash
driver=fcdpp,period=19200
```
where `period` is optional but specifies the sample count per ALSA period (defaults to sample rate / 4 => 250msec latency).
Adjust if you want lower latency at the cost of higher context switch rates (eg: an RPi3 can tolerate down to 2048 samples,
however an OrangePi Zero LTS may kernel panic / become unstable below 9600, hence the default!)

### Remote use

You can also use SoapyRemote (or another SoapySDR remoting solution) to operate this driver as a headless front-end.

On the remote system:
```bash
SoapySDRServer --bind="0.0.0.0:1234"
```

On the local system (eg: GQRX SoapySDR client):

 * Device string in GQRX: `soapy=0,remote=hostname.local:1234,remote:driver=fcdpp`
 * Input rate: 192000 (if you have FCD Pro+, otherwise 96000)
 * All other settings default (no decimation, zero bandwidth, LO frequency)

## Permissions

To access an FCD USB device for tuning/gain control etc. without being root you need this:

```
# Udev rules for the Funcube Dongle Pro (0xfb56) & Pro+ (0xfb31)
# Put this in:
# /etc/udev/rules.d/81-funcube.rules

# Udev rule for the Funcube Dongle to be used with libusb (NB: SYMLINK is not necessary but convenient)
SUBSYSTEMS=="usb", ATTRS{idVendor}=="04d8", ATTRS{idProduct}=="fb56", GROUP="audio", MODE="0666", SYMLINK+="usbfcd1"
SUBSYSTEMS=="usb", ATTRS{idVendor}=="04d8", ATTRS{idProduct}=="fb31", GROUP="audio", MODE="0666", SYMLINK+="usbfcd2"
```

NB: If you have GNU radio support for Funcube Dongles installed you may have what you need in `/lib/udev/rules.d/60-gr-fcdproplus.rules`
or `/lib/udev/rules.d/60-libgnuradio-fcd*`.

## License

Boost Software License 1.0 (BSL-1.0)
