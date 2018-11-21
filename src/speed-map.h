#ifndef SPEED_MAP_H
#define SPEED_MAP_H

/*
This file defines StepsToSpeedMap class which maps speed steps (0-28) to
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

constexpr size_t STEPS_CNT = 28;
constexpr size_t SPEED_MAX = 120;
constexpr unsigned EMPTY_VALUE = 0;

class StepsToSpeedMap : public QObject {
	Q_OBJECT

private:
	unsigned m_max_speed = SPEED_MAX;
	std::array<unsigned, STEPS_CNT> m_map;

public:
	StepsToSpeedMap(QObject *parent = nullptr);
	StepsToSpeedMap(const QString& filename, QObject *parent = nullptr);

	void load(const QString& filename);
	void save(const QString& filename);
	void clear();
	void addOrUpdate(const unsigned step, const unsigned speed);
	unsigned noDifferentSpeeds() const;

	unsigned maxSpeed() const;
	void setMaxSpeed(const unsigned new_speed);
	unsigned maxSpeedInFile() const;

	unsigned const* at(const int index) const;
	unsigned const* operator[] (const int index) const;

signals:
	void onAddOrUpdate(unsigned step, unsigned speed);
	void onClear();
};

}//namespace Ssm

#endif
