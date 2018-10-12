#ifndef _POWER_MAP_H_
#define _POWER_MAP_H_

#include <stddef.h>
#include <QObject>
#include <memory>

#include "lib/q-str-exception.h"

namespace Pm {

// This class uses stepindex everywhere!

const size_t _STEPS_CNT = 256;
struct ENoMap : public QStrException {
	ENoMap(const QString str) : QStrException(str) {}
};

class PowerToSpeedMap : public QObject {
	Q_OBJECT

public:
	PowerToSpeedMap(QObject *parent = nullptr);

	void clear();
	void addOrUpdate(unsigned step, float speed);
	unsigned steps(float speed);
	bool isRecord(unsigned step);
	float* speed(unsigned step);

signals:
	void onAddOrUpdate(unsigned step, float speed);
	void onClear();

private:
	std::unique_ptr<float> map[_STEPS_CNT];
};

}//end namespace

#endif
