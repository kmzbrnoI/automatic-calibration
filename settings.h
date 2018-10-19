#ifndef _SETTINGS_H_
#define _SETTINGS_H_

#include <QSerialPort>
#include <QSettings>
#include <map>

/* This file defines global settings of the application */

const std::map<QString, std::map<QString, QVariant>> _DEFAULTS {
	{"WSM", {
		{"scale", 120},
		{"wheelDiameter", 8.0},
	}},
	{"XN", {
		{"baudrate", 19200},
		{"flowcontrol", 1},
		{"loglevel", 1},
		{"port", "/dev/ttyUSB0"},
		{"logfile", ""},
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
	std::map<QString, std::map<QString, QVariant>> data;

	void load(const QString& filename);
	void save(const QString& filename);

	std::map<QString, QVariant>& at(const QString& g);
	std::map<QString, QVariant>& operator[] (const QString& g);

	static void cfgToUnsigned(std::map<QString, QVariant>& cfg, const QString& section,
	                          unsigned& target);
	static void cfgToDouble(std::map<QString, QVariant>& cfg, const QString& section,
	                        double& target);
	static void cfgToQString(std::map<QString, QVariant>& cfg, const QString& section,
	                        QString& target);
private:
};

#endif
