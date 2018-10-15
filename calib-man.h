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
	SetManually,
};

enum class CmError {
	LargeDiffusion,
	XnNoResponse,
	LocoStopped,
	NoStep,
};

enum class CalibState {
	Stopped,
	InitProg,
	Overview,
	Steps,
	Interpolation,
};

const unsigned _CV_CONFIG = 29;
const unsigned _CV_CONFIG_BIT_SPEED_TABLE = 4;
const bool _CV_CONFIG_SPEED_TABLE_VALUE = true;
const unsigned _CV_ACCEL = 3;
const unsigned _CV_DECEL = 4;

class CalibMan : public QObject {
	Q_OBJECT

public:
	Cs::CalibStep cs;
	Co::CalibOverview co;
	Xn::XnDirection direction;

	CalibMan(Xn::XpressNet& xn, Pm::PowerToSpeedMap& pm, Wsm::Wsm& wsm,
	         Ssm::StepsToSpeedMap& ssm, QObject *parent = nullptr);

	void calibrateAll(unsigned locoAddr,  Xn::XnDirection dir);
	void stop();
	void reset();
	void interpolateAll();
	void setStepManually(unsigned step, unsigned power);
	void unsetStep(unsigned step);
	bool inProgress() const;
	CalibState progress() const;

private:
	Ssm::StepsToSpeedMap& m_ssm;
	Xn::XpressNet& m_xn;
	StepState state[Xn::_STEPS_CNT]; // step index used as index
	unsigned power[Xn::_STEPS_CNT]; // power assigned to steps after calibration
	unsigned m_locoAddr = 3;
	unsigned m_step_writing;
	unsigned m_step_power;
	CalibState m_progress = CalibState::Stopped;

	std::unique_ptr<unsigned> nextStep(); // returns step index
	void calibrateNextStep();
	std::unique_ptr<unsigned> nextStepBin(const std::vector<unsigned>& used_steps,
	                                      const size_t left, const size_t right);
	void csSigConnect();
	void csSigDisconnect();
	void updateProg(CalibState cs, size_t progress, size_t max);
	size_t getProgress(CalibState cs, size_t progress, size_t max);

	void xnStepWritten(void*, void*);
	void xnStepWriteError(void*, void*);

	// Steps interpolation = IP
	unsigned m_thisIPleft;
	unsigned m_thisIPstep;
	unsigned m_thisIPright;
	unsigned getIPpower(unsigned left, unsigned right, unsigned step);

	void interpolateNext();
	void xnIPWritten(void*, void*);
	void xnIPError(void*, void*);

	void done();
	void error(Cm::CmError, unsigned step);

	void useSpeedTable();
	void xnSTWritten(void*, void*);
	void xnSTError(void*, void*);

	void adToZero();
	void adError(void*, void*);
	void accelWritten(void*, void*);
	void decelWritten(void*, void*);

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

	void onProgressUpdate(size_t val); // value 0-100
};

}//namespace Cm

#endif
