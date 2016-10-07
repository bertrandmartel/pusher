# Pusher

https://akinaru.github.io/pusher

[![Build Status](https://travis-ci.org/akinaru/pusher.svg?branch=master)](https://travis-ci.org/akinaru/pusher)
[![License](http://badge.kloud51.com/pypi/l/html2text.svg)](LICENSE.md)

Mechanical wall switch button pusher securely controlled via Bluetooth LE with RFduino module/LCD panel & via Android smartphone

[![Download Pusher from Google Play](http://www.android.com/images/brand/android_app_on_play_large.png)](https://play.google.com/store/apps/details?id=com.github.akinaru.roboticbuttonpusher)
[![Download latest debug from drone.io](https://raw.githubusercontent.com/kageiit/images-host/master/badges/drone-io-badge.png)](https://drone.io/github.com/akinaru/pusher/files/android/app/build/outputs/apk/app-debug.apk)

![demo](img/demo.gif)

![screenshot](img/lcd.jpg)

<hr/>

Application layer association security  :

* AES 256 encryption
* user code for key exchange
* additional password protection

protected against MIM & Replay attack

![pairing](img/pairing.gif)

## Project structure

* <b>/firmware</b> - microcontroller code written for RFduino
* <b>/android</b> - android application to control device
* <b>/hardware</b> - wiring diagram, schematics & BOM

## Build

### Get source code

```
git clone git@github.com:akinaru/pusher.git
cd pusher
git submodule update --init --recursive
```

### Build Firmware

```
cd firmware
make
```

### Build Android App

```
cd android
./gradlew build
```

## [Firmware specifications](firmware/README.md)

## [Hardware specifications](hardware/README.md)

## External library

### Firmware

* rfduino software : https://github.com/RFduino/RFduino
* LiquidCrystal arduino library : https://github.com/arduino-libraries/LiquidCrystal
* rfduino-makefile : https://github.com/akinaru/rfduino-makefile
* AES library by Brian Gladman

### Android application

* smart Android dot progress bar : https://github.com/silvestrpredko/DotProgressBarExample
* AES library by Brian Gladman
* appcompat-v7, design & recyclerview-v7

## License

```
Copyright (C) 2016  Bertrand Martel

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 3
of the License, or (at your option) any later version.

Pusher is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Pusher.  If not, see <http://www.gnu.org/licenses/>.
```
