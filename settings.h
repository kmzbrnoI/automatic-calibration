#ifndef _SETTINGS_H_
#define _SETTINGS_H_

#include <QSerialPort>
#include <QSettings>

/* This file defines global settings of the application */

struct WsmSettings {
	QString portname;
	unsigned int scale;
	double wheelDiameter; // unit: mm

	void load(QSettings&);
	void save(QSettings&);
};

struct XnSettings {
	QString portname;
	uint32_t br;
	QSerialPort::FlowControl fc;
	uint32_t loglevel;

	void load(QSettings&);
	void save(QSettings&);
};

class Settings {
public:
	WsmSettings wsm;
	XnSettings xn;

	Settings(QString filename);
	void load(QString filename);
	void save(QString filename);
private:
};

#endif
