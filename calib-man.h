#ifndef _CALIB_MAN_H_
#define _CALIB_MAN_H_

#include <QObject>
#include <memory>
#include <vector>

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

	void calibrateAll();

private:
	Ssm::StepsToSpeedMap& m_ssm;
	Xn::XpressNet& m_xn;
	StepState state[Xn::_STEPS_CNT]; // step index used as index

	std::unique_ptr<unsigned> nextStep(); // returns step index
	void calibrateNextStep();
	std::unique_ptr<unsigned> nextStepBin(const std::vector<unsigned>& used_steps,
	                                      const size_t left, const size_t right);

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
