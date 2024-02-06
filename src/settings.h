#ifndef SETTINGS_H
#define SETTINGS_H

#include <QSerialPort>
#include <QSettings>
#include <map>

/* This file defines global settings of the application */

using Config = std::map<QString, std::map<QString, QVariant>>;

const Config DEFAULTS {
	{"WSM", {
		{"scale", 120},
		{"wheelDiameter", 8.0},
		{"ticksPerRevolution", 8},
		{"port", "/dev/ttyUSB0"},
	}},
	{"XN", {
		{"baudrate", 19200},
		{"flowcontrol", 1},
		{"loglevel", 1},
		{"port", "auto"},
		{"logfile", ""},
		{"interface", "uLI"},
		{"outIntervalMs", 50},
	}},
	{"Speed", {
		{"file", "speed.csv"},
	}},
	{"Logging", {
		{"file", ""},
	}},
};

class Settings {
public:
	Config data;

	void load(const QString &filename);
	void save(const QString &filename);

	std::map<QString, QVariant> &at(const QString &g);
	std::map<QString, QVariant> &operator[](const QString &g);

	static void cfgToUnsigned(std::map<QString, QVariant> &cfg, const QString &section,
	                          unsigned &target);
	static void cfgToDouble(std::map<QString, QVariant> &cfg, const QString &section,
	                        double &target);
	static void cfgToQString(std::map<QString, QVariant> &cfg, const QString &section,
	                         QString &target);

private:
};

#endif
