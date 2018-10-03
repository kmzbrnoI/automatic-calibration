#include "settings.h"

Settings::Settings(QString filename) {
	load(filename);
}

void Settings::load(QString filename) {
	QSettings s(filename, QSettings::IniFormat);

	s.beginGroup("WSM");
	wsm.load(s);
	s.endGroup();

	s.beginGroup("XN");
	xn.load(s);
	s.endGroup();
}

void Settings::save(QString filename) {
	QSettings s(filename, QSettings::IniFormat);
	s.beginGroup("WSM");
	wsm.save(s);
	s.endGroup();

	s.beginGroup("XN");
	xn.save(s);
	s.endGroup();
}


void WsmSettings::load(QSettings& s) {
	portname = s.value("port", "").toString();
	scale = s.value("scale", 120).toInt();
	wheelDiameter = s.value("wheelDiameter", 8.0).toDouble();
}

void WsmSettings::save(QSettings& s) {
	s.setValue("port", portname);
	s.setValue("scale", scale);
	s.setValue("wheelDiameter", wheelDiameter);
}


void XnSettings::load(QSettings& s) {
	portname = s.value("port", "").toString();
	br = s.value("baudrate", 9600).toInt();
	fc = static_cast<QSerialPort::FlowControl>(s.value("flowcontrol", 1).toInt());
	loglevel = s.value("loglevel", 1).toInt();
}

void XnSettings::save(QSettings& s) {
	s.setValue("port", portname);
	s.setValue("baudrate", br);
	s.setValue("flowcontrol", static_cast<int>(fc));
	s.setValue("loglevel", loglevel);
}
