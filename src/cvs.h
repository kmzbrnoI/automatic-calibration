#ifndef CVS_H
#define CVS_H

constexpr unsigned CV_ADDR_SHORT = 1;
constexpr unsigned CV_START_VOLTAGE = 2;
constexpr unsigned CV_ACCEL = 3;
constexpr unsigned CV_DECEL = 4;
constexpr unsigned CV_VMAX = 5;
constexpr unsigned CV_MEDIUM_SPEED = 6;
constexpr unsigned CV_RESET = 8;
constexpr unsigned CV_ADDR_HI = 17;
constexpr unsigned CV_ADDR_LO = 18;
constexpr unsigned CV_BASIC_CONFIG = 29;
constexpr unsigned CV_UREF = 57;
constexpr unsigned CV_CURVE_START = 67; // cv 67 = step 1

constexpr unsigned CV_CONFIG_BIT_DIRECTION = 0;
constexpr unsigned CV_CONFIG_BIT_SPEED_STEPS = 1;
constexpr unsigned CV_CONFIG_BIT_ANALOG = 2;
constexpr unsigned CV_CONFIG_BIT_RAILCOM = 3;
constexpr unsigned CV_CONFIG_BIT_SPEED_TABLE = 4;
constexpr unsigned CV_CONFIG_BIT_EXTENDED_ADDR = 5;

constexpr unsigned CV_RESET_RESET = 8; // value to reset decoder

#endif
