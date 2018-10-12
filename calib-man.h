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
#include "calib-overview.h"

namespace Cm {

enum class StepState {
	Calibred,
	Uncalibred,
};

enum class CmError {
	LargeDiffusion,
	XnNoResponse,
	LocoStopped,
	NoStep,
};

class CalibMan : public QObject {
	Q_OBJECT

public:
	Cs::CalibStep cs;
	Co::CalibOverview co;
	Xn::XnDirection direction;

	CalibMan(Xn::XpressNet& xn, Pm::PowerToSpeedMap& pm, Wsm::Wsm& wsm,
	         Ssm::StepsToSpeedMap& ssm, QObject *parent = nullptr);

	void calibrateAll(unsigned locoAddr,  Xn::XnDirection dir);
	void reset();

private:
	Ssm::StepsToSpeedMap& m_ssm;
	Xn::XpressNet& m_xn;
	StepState state[Xn::_STEPS_CNT]; // step index used as index
	unsigned m_locoAddr;
	unsigned m_step_writing;
	unsigned m_step_power;

	std::unique_ptr<unsigned> nextStep(); // returns step index
	void calibrateNextStep();
	std::unique_ptr<unsigned> nextStepBin(const std::vector<unsigned>& used_steps,
	                                      const size_t left, const size_t right);
	void csSigConnect();
	void csSigDisconnect();

	void xnStepWritten(void*, void*);
	void xnStepWriteError(void*, void*);
	static void xnsStepWritten(void*, void*);
	static void xnsStepWriteError(void*, void*);

private slots:
	void csDone(unsigned step, unsigned power);
	void csError(Cs::CsError, unsigned step);

	void coDone();
	void coError(Co::CoError, unsigned step);

	void cStepPowerChanged(unsigned step, unsigned power);

signals:
	void onStepStart(unsigned step);
	void onStepDone(unsigned step, unsigned power);
	void onStepError(Cm::CmError, unsigned step);

	void onLocoSpeedChanged(unsigned step);
	void onDone();
	void onStepPowerChanged(unsigned step, unsigned power);
};

}//end namespace

#endif
