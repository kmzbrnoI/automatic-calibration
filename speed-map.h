#ifndef _SPEED_MAP_H_
#define _SPEED_MAP_H_

#include <stddef.h>
#include <QObject>
#include <memory>

namespace Ssm {

const size_t _STEPS_CNT = 28;

class StepsToSpeedMap : public QObject {
	Q_OBJECT

public:
	std::unique_ptr<unsigned> map[_STEPS_CNT];

	StepsToSpeedMap(QObject *parent = nullptr);
	StepsToSpeedMap(QString filename, QObject *parent = nullptr);

	void load(QString filename);
	void save(QString filename);
	void clear();
	void addOrUpdate(unsigned step, unsigned speed);

signals:
	void onAddOrUpdate(unsigned step, unsigned speed);
	void onClear();
};

}//end namespace

#endif
