# Automatic Calibration SW

Automatic Calibration SW is a desktop application which allows you to
automatically calibrate your model train vehicle. It requires a [Wireless
Speedometer](https://github.com/kmzbrnoI/wsm-pcb) and [XpressNET command
station](https://dccwiki.com/XpressNet_Protocol) connected to PC via
[LI](https://dccwiki.com/Computer_Interface_List) and (virtual) serial port.

**Calibrating** = assigning power to each of 28 speed steps so specific speed
steps fit specific real speed. E. g. step 15 = 40 km/h, step 17 = 50 km/h etc.
This assignment is defined by user and this application loads it from file
'speed.csv'. The main aim of this application is to do this 'assignment'
automatically.

How does it work? Application assigns *power* to each speed step of the DCC
decoder in the train and then measures real speed of the train. It repeats this
process until the correct speed is assigned to the speed step. And the again
for different speed step and different real speed...

This application is developed in [QT](https://www.qt.io/) v5 or v6.

## Resources

 * [WSM PCB](https://github.com/kmzbrnoI/wsm-pcb)
 * [WSM FW](https://github.com/kmzbrnoI/wsm-fw)
 * [WSM Speed Reader](https://github.com/kmzbrnoI/wsm-speed-reader)

## Building & toolkit

This SW was developed in `vim` using `qmake` & `make`. Downloads are available
in *Releases* section. It is suggested to use `clang` as a compiler, because
then you may use `clang-tools` during development process (see below).

### Prerequisities

 * Qt 5/6
 * Qt's `serialport`
 * Qt's `charts`
 * Optional: clang build tools (see below)
 * Optional for clang: [Bear](https://github.com/rizsotto/Bear)

### Example: toolchain setup on Debian 12 Bookworm

```bash
$ apt install qt6-base-dev qt6-charts-dev qt6-serialport-dev
$ apt install clang clang-tools clang-tidy clang-format
$ apt install bear
```

### Build

Clone this repository (including submodules!):

```bash
$ git clone --recurse-submodules https://github.com/kmzbrnoI/automatic-calibration
```

And then build:

```bash
$ mkdir build
$ cd build
$ qmake6 -spec linux-clang ..
$ bear -- make
```

To make debug binary, run:

```bash
$ qmake6 -spec linux-clang CONFIG+=debug ..
$ make
```

You may use [this script](https://serverfault.com/questions/61659/can-you-get-any-program-in-linux-to-print-a-stack-trace-if-it-segfaults) to debug segfaults.

## Connecting to WSM

 * Windows: pair it with HC-05 module, serial port should be added
   automatically
 * Linux: pair it with HC-05 module and map it to new serial device:

    ```bash
    $ rfcomm connect /dev/rfcomm0 hc-05-hw-address 2
    ```

## Project description

This project consists of several C++ classes defined in header files and
its implementations defined in `.cpp` files. All classes are described in
diagram below:

![Class diagram](doc/ac-class-structure.png)

Squares are classes (in this project singletons), the arrow represents
relationship *owns*. Red classes are Window classes, `MainWindow` basically
owns everything, yellow classes are libraries, blue classes are managers and
green classes are helpers.

Basically all the managers use all the libraries and helpers. Helpers are
connected to GUI to visualize its states. Libs and helpers are passed to
managers as references.

`CalibMan` manages whole process of calibration divided into two phases
(`CalibOverview`, `CalibStep`), see [`calib-man.h`](calib-man.h) for more
information.

All visualized classes inherit from `QObject` and usually communicate with
*outer world* by receiving function calls and calling Qt's signals.

Each header file contains a docstring, so see it for more information.

## Configuration

The binary loads general configuration from `config.ini` file located in the
directory where the binary is executed from. This file is created once the
application is run for the first time. Inspect it and configure the app
according to your requirements. Configuration cannot be done from GUI, it
is done just through the config file. Config could be reloaded at runtime.

Loco-specific configuration could be loaded from & saved to `xml` file
according to the format of loco of [JMRI](http://jmri.sourceforge.net/).

Speed table is loaded from `speed.csv` file, where each line is of format
`step;speed`:

```
6;10
15;40
17;50
...
```

## Style checking

```bash
$ clang-tidy -p build src/*.cpp
$ clang-format *.cpp *.h
```

## Authors

 * Jan Horacek ([jan.horacek@kmz-brno.cz](mailto:jan.horacek@kmz-brno.cz))

## License

This application is released under the [Apache License v2.0
](https://www.apache.org/licenses/LICENSE-2.0).
