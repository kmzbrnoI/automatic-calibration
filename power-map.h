#ifndef _POWER_MAP_H_
#define _POWER_MAP_H_

#include <stddef.h>
#include <QObject>
#include <memory>

namespace Pm {

const size_t _STEPS_CNT = 256;

class PowerToSpeedMap : public QObject {
	Q_OBJECT

public:
	PowerToSpeedMap(float max_speed = 120, QObject *parent = nullptr);
	float m_max_speed;

	void clear();
	void addOrUpdate(unsigned step, float speed);
	unsigned steps(float speed);

signals:
	void onAddOrUpdate(unsigned step, float speed);
	void onClear();

private:
	std::unique_ptr<float> map[_STEPS_CNT];
};

}//end namespace

#endif
