#ifndef _POWER_MAP_H_
#define _POWER_MAP_H_

/*
This file defines power-to-speed mapping via PowerToSpeedMap class.

 * It allows to assign 'float' speed to each power step (0-255).
 * It allows to interpolate power based on the current records in the mapping.
 * It allows to add new records to mapping dynamically.
*/

#include <stddef.h>
#include <QObject>
#include <memory>

#include "lib/q-str-exception.h"

namespace Pm {

// This class uses stepindex everywhere!

constexpr size_t _POWER_CNT = 256;

struct ENoMap : public QStrException {
	ENoMap(const QString str) : QStrException(str) {}
};

class PowerToSpeedMap : public QObject {
	Q_OBJECT

public:
	PowerToSpeedMap(QObject *parent = nullptr);

	void clear();
	void addOrUpdate(const unsigned power, const float speed);
	unsigned power(const float speed) const;
	bool isRecord(const unsigned power) const;
	bool isAnyRecord() const;
	float* speed(const unsigned power) const;

	float* at(const int power) const;
	float* operator[] (const int power) const;

signals:
	void onAddOrUpdate(unsigned power, float speed);
	void onClear();

private:
	std::unique_ptr<float> map[_POWER_CNT];
};

}//namespace Pm

#endif