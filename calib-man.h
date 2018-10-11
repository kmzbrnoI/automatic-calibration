#ifndef _CALIB_MAN_H_
#define _CALIB_MAN_H_

#include <QObject>

#include "lib/xn/xn.h"
#include "lib/wsm/wsm.h"
#include "power-map.h"
#include "speed-map.h"
#include "calib-step.h"

namespace Cm {

enum class StepState {
	Calibred,
	Uncalibred,
};

class CalibMan : public QObject {
	Q_OBJECT

public:
	CalibStep cs;

	CalibMan(Xn::XpressNet& xn, Pm::PowerToSpeedMap& pm, Wsm::Wsm& wsm,
	         Ssm::StepsToSpeedMap& ssm, QObject *parent = nullptr);

private:
	Ssm::StepsToSpeedMap& m_ssm;
	Xn::XpressNet& m_xn;

private slots:
	void csDone();
	void csError();
	void csStepPowerChanged(unsigned step, unsigned power);

signals:
	void onStepDone(unsigned step);
	void onStepStart(unsigned step);
	void onStepError(unsigned step);
	void onLocoSetSpeed(unsigned step);
	void onSetStep(unsigned step);
	void onDone();
	void onBreak();
	void onStepPowerChanged(unsigned step, unsigned power);
};

}//end namespace

#endif
