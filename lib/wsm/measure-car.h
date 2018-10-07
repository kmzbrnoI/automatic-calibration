#ifndef _MEASURE_CAR_H_
#define _MEASURE_CAR_H_

#include <QByteArray>
#include <QSerialPort>
#include <QTimer>
#include <QDateTime>

#include "../q-str-exception.h"

namespace Wsm {

const unsigned int _BUF_IN_TIMEOUT_MS = 300;

class EOpenError : public QStrException {
public:
	EOpenError(const QString str) : QStrException(str) {}
};

class MeasureCar : public QObject {
	Q_OBJECT

public:
	explicit MeasureCar(QString portname, unsigned int scale=120,
	                    double wheelDiameter = 8, QObject *parent = nullptr);
	unsigned int scale;
	double wheelDiameter; // unit: mm
	void distanceReset();

private slots:
	void handleReadyRead();
	void handleError(QSerialPort::SerialPortError error);

signals:
	void speedRead(double speed, uint16_t speed_raw);
	void onError(QString error);
	void batteryRead(double voltage, uint16_t voltage_raw);
	void batteryCritical(); // device will automatically disconnect when this event happens
	void distanceRead(double distance, uint32_t distance_raw);

private:
	const unsigned int F_CPU = 3686400; // unit: Hz
	const unsigned int PSK = 64;
	const unsigned int HOLE_COUNT = 8;

	QSerialPort m_serialPort;
	QByteArray m_readData;
	QDateTime m_receiveTimeout;
	uint32_t m_distStart;
	uint32_t m_dist;

	void parseMessage(QByteArray message);
};

}//end namespace

#endif
