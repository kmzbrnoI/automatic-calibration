# Automatic Calibration SW

Automatic Calibration SW is a desktop application which allows you to
automatically calibrate your model train vehicle.

This application is developed in [QT](https://www.qt.io/).

*More information will be added here when the project reaches alpha state.*

## Resources

 * [WSM PCB](https://github.com/kmzbrnoI/wsm-pcb)
 * [WSM FW](https://github.com/kmzbrnoI/wsm-fw)
 * [WSM Speed Reader](https://github.com/kmzbrnoI/wsm-speed-reader)

## Building & toolkit

This SW was developed in `vim` using `make`. Downloads are available in
*Releases* section.

Howto build:

```
$ uic main-window.ui > ui_main-window.h
$ mkdir build
$ cd build
$ qmake ..
$ make
```

## Connecting to WSM

 * Windows: pair it with HC-05 module, serial port should be added
   automatically
 * Linux: pair it with HC-05 module and map it to new serial device:

    ```
    rfcomm connect /dev/rfcomm0 hc-05-hw-address 2
    ```

## Authors

 * Jan Horacek ([jan.horacek@kmz-brno.cz](mailto:jan.horacek@kmz-brno.cz))

## License

This application is released under the [Apache License v2.0
](https://www.apache.org/licenses/LICENSE-2.0).
