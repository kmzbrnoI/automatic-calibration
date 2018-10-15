#ifndef _SPEED_MAP_H_
#define _SPEED_MAP_H_

#include <stddef.h>
#include <QObject>
#include <memory>

namespace Ssm {

// Uses step index everywhere

const size_t _STEPS_CNT = 28;
const size_t _SPEED_MAX = 120;

class StepsToSpeedMap : public QObject {
	Q_OBJECT

private:
	unsigned m_max_speed = _SPEED_MAX;
	std::unique_ptr<unsigned> m_map[_STEPS_CNT];

public:
	StepsToSpeedMap(QObject *parent = nullptr);
	StepsToSpeedMap(QString filename, QObject *parent = nullptr);

	void load(QString filename);
	void save(QString filename);
	void clear();
	void addOrUpdate(unsigned step, unsigned speed);
	unsigned noDifferentSpeeds() const;

	unsigned maxSpeed() const;
	void setMaxSpeed(unsigned new_speed);

	unsigned* at(const int index);
	unsigned* operator[] (const int index);

signals:
	void onAddOrUpdate(unsigned step, unsigned speed);
	void onClear();
};

}//namespace Ssm

#endif
