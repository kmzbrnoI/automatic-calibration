#ifndef _WSM_H_
#define _WSM_H_

#include <QByteArray>
#include <QSerialPort>
#include <QTimer>
#include <QDateTime>

#include "../q-str-exception.h"

namespace Wsm {

const unsigned int _BUF_IN_TIMEOUT_MS = 300;
const unsigned int _SPEED_RECEIVE_TIMEOUT = 3000; // 3 s

struct EOpenError : public QStrException {
	EOpenError(const QString str) : QStrException(str) {}
};
struct ELtAlreadyMeasuring : public QStrException {
	ELtAlreadyMeasuring(const QString str) : QStrException(str) {}
};
struct ENoSpeedData : public QStrException {
	ENoSpeedData(const QString str) : QStrException(str) {}
};

class Wsm : public QObject {
	Q_OBJECT

public:
	explicit Wsm(QString portname, unsigned int scale=120,
	                    double wheelDiameter = 8, QObject *parent = nullptr);
	unsigned int scale;
	double wheelDiameter; // unit: mm
	void distanceReset();
	void startLongTermMeasure(unsigned count);
	bool isSpeedOk() const;

private slots:
	void handleReadyRead();
	void handleError(QSerialPort::SerialPortError error);
	void t_speedTimeout();

signals:
	void speedRead(double speed, uint16_t speed_raw);
	void onError(QString error);
	void batteryRead(double voltage, uint16_t voltage_raw);
	void batteryCritical(); // device will automatically disconnect when this event happens
	void distanceRead(double distance, uint32_t distance_raw);
	void longTermMeasureDone(double speed, double diffusion);
	void speedReceiveTimeout();
	void speedReceiveRestore();

private:
	const unsigned int F_CPU = 3686400; // unit: Hz
	const unsigned int PSK = 64;
	const unsigned int HOLE_COUNT = 8;

	QSerialPort m_serialPort;
	QByteArray m_readData;
	QDateTime m_receiveTimeout;
	uint32_t m_distStart;
	uint32_t m_dist;
	QTimer m_speedTimer;
	bool m_speedOk = false;
	bool m_lt_measuring = false;
	double m_lt_sum;
	unsigned m_lt_count;
	unsigned m_lt_count_max;
	double m_lt_min, m_lt_max;

	void parseMessage(QByteArray message);
	void recordLt(double speed);
};

}//end namespace

#endif
