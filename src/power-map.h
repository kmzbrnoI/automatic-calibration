#ifndef POWER_MAP_H
#define POWER_MAP_H

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

constexpr size_t POWER_CNT = 256;
constexpr float EMPTY_VALUE = -1;

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
	float const* speed(const unsigned power) const;

	float const* at(const int power) const;
	float const* operator[] (const int power) const;

signals:
	void onAddOrUpdate(unsigned power, float speed);
	void onClear();

private:
	std::array<float, POWER_CNT> map;
};

}//namespace Pm

#endif
