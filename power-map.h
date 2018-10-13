#ifndef _POWER_MAP_H_
#define _POWER_MAP_H_

#include <stddef.h>
#include <QObject>
#include <memory>

#include "lib/q-str-exception.h"

namespace Pm {

// This class uses stepindex everywhere!

const size_t _POWER_CNT = 256;
struct ENoMap : public QStrException {
	ENoMap(const QString str) : QStrException(str) {}
};

class PowerToSpeedMap : public QObject {
	Q_OBJECT

public:
	PowerToSpeedMap(QObject *parent = nullptr);

	void clear();
	void addOrUpdate(unsigned power, float speed);
	unsigned power(float speed);
	bool isRecord(unsigned power);
	float* speed(unsigned power);

signals:
	void onAddOrUpdate(unsigned power, float speed);
	void onClear();

private:
	std::unique_ptr<float> map[_POWER_CNT];
};

}//end namespace

#endif
