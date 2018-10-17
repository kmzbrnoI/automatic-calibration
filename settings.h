#ifndef _SETTINGS_H_
#define _SETTINGS_H_

#include <QSerialPort>
#include <QSettings>
#include <map>

/* This file defines global settings of the application */

const std::map<QString, std::map<QString, QVariant>> _DEFAULTS {
	std::pair<QString, std::map<QString, QVariant>>("WSM",
	{
		std::pair<QString, QVariant>("scale", 120),
		std::pair<QString, QVariant>("wheelDiameter", 8.0),
	}),
	std::pair<QString, std::map<QString, QVariant>>("XN",
	{
		std::pair<QString, QVariant>("baudrate", 19200),
		std::pair<QString, QVariant>("flowcontrol", 1),
		std::pair<QString, QVariant>("loglevel", 1),
		std::pair<QString, QVariant>("port", "/dev/ttyUSB0"),
	}),
};

class Settings {
public:
	std::map<QString, std::map<QString, QVariant>> data;

	Settings(QString filename);
	void load(QString filename);
	void save(QString filename);

	std::map<QString, QVariant>& at(const QString g);
	std::map<QString, QVariant>& operator[] (const QString g);
private:
};

#endif
