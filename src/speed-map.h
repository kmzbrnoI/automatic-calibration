#ifndef _SPEED_MAP_H_
#define _SPEED_MAP_H_

/*
This file defines StepsToSpeedMap class which maps spped steps (0-28) to
real speed of the train. It defines the goal of the calibration, it defines
which real speed to assign to which speed step.

The dependency could be loaded form a csv file in a format
'step;speed' (speed is in kmph as plain number)
(This is the same format as hJOPserver uses.)

The mapping could me only patrial, unused mappings are intended to be
interpolated.
*/

#include <stddef.h>
#include <QObject>
#include <memory>

namespace Ssm {

// Uses step index everywhere

constexpr size_t _STEPS_CNT = 28;
constexpr size_t _SPEED_MAX = 120;

class StepsToSpeedMap : public QObject {
	Q_OBJECT

private:
	unsigned m_max_speed = _SPEED_MAX;
	std::unique_ptr<unsigned> m_map[_STEPS_CNT];

public:
	StepsToSpeedMap(QObject *parent = nullptr);
	StepsToSpeedMap(const QString& filename, QObject *parent = nullptr);

	void load(const QString& filename);
	void save(const QString& filename);
	void clear();
	void addOrUpdate(const unsigned step, const unsigned speed);
	unsigned noDifferentSpeeds();

	unsigned maxSpeed() const;
	void setMaxSpeed(const unsigned new_speed);
	unsigned maxSpeedInFile() const;

	unsigned* at(const int index);
	unsigned* operator[] (const int index);

signals:
	void onAddOrUpdate(unsigned step, unsigned speed);
	void onClear();
};

}//namespace Ssm

#endif
