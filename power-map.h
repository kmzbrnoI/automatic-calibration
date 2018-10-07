#ifndef _POWER_MAP_H_
#define _POWER_MAP_H_

#include <stddef.h>
#include <QObject>

const size_t _STEPS_CNT = 256;

class PowerToSpeedMap : public QObject {
	Q_OBJECT

public:
	PowerToSpeedMap();
	void clear();
	void add(unsigned step, float speed);
	unsigned steps(float speed);

signals:
	void onAdd(unsigned step, float speed);

private:
	float map[_STEPS_CNT];
};

#endif
