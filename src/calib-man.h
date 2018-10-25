#ifndef _CALIB_MAN_H_
#define _CALIB_MAN_H_

/*
This file defines CalibMan class, which serves as a Calibration Manager
singleton. It manages the whole proess of calibration. It uses several
calibration helper classes.

 * It is intentionally separated from GUI and communicates with outer
   environment with calls and events.
 * The whole process is started by calling calibrateAll function.
 * The process is divided into multiple stages, at the end exactly one of
    (a) onDone or
    (b) onError
   events is always called.
 * Calibration process could be stopped manually by calling stop() function.
 * When calibration is run repeatedly, already-done parts are skipped, so you
   could "resume" calibration by calling calibrateAll function again.
 * reset() function causes the next call of calibrateAll() to do the whole
   process again and not take already-done steps into account.

Calibration stages:

 1) Set important CVs to default (accel, devel, Vmax, ...).
 2) Make "basci overview" of power-to-speed mapping = try some powers and
    measure speed (calib-overview.h)
 3) Calibrate speed steps (calib-step.h).
 4) Interpolate the rest of the steps.

All programming is done via POM (it is fast!).
*/

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
	Oscilation,
};

enum class CalibState {
	Stopped,
	InitProg,
	Overview,
	Steps,
	Interpolation,
};

constexpr unsigned _CV_CONFIG = 29;
constexpr unsigned _CV_CONFIG_BIT_SPEED_TABLE = 4;
constexpr bool _CV_CONFIG_SPEED_TABLE_VALUE = true;
constexpr unsigned _CV_ACCEL = 3;
constexpr unsigned _CV_DECEL = 4;
constexpr unsigned _DEFAULT_VMAX = 255;
constexpr unsigned _VMAX_CV = 5;

class CalibMan : public QObject {
	Q_OBJECT

public:
	Cs::CalibStep cs;
	Co::CalibOverview co;
	Xn::XnDirection direction;
	unsigned vmax = _DEFAULT_VMAX;

	CalibMan(Xn::XpressNet& xn, Pm::PowerToSpeedMap& pm, Wsm::Wsm& wsm,
	         Ssm::StepsToSpeedMap& ssm, QObject *parent = nullptr);

	void calibrateAll(const unsigned locoAddr,  Xn::XnDirection dir);
	void stop();
	void reset();
	void interpolateAll();
	void setStepManually(const unsigned step, unsigned power);
	void unsetStep(const unsigned step);
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
	unsigned m_no_calibrated;

	using CVsConfig = std::map<unsigned, unsigned>;
	CVsConfig init_cvs = { // (cv, value)
		{2, 1}, {3, 0}, {4, 0}, {_VMAX_CV, _DEFAULT_VMAX}, {6, 60},
	};
	CVsConfig::iterator m_init_cv_iterator;
	unsigned m_init_cv_index;

	std::unique_ptr<unsigned> nextStep(); // returns step index
	void calibrateNextStep();
	std::unique_ptr<unsigned> nextStepBin(const std::vector<unsigned>& used_steps,
	                                      const size_t left, const size_t right);
	void csSigConnect();
	void csSigDisconnect();
	void updateProg(const CalibState cs, const size_t progress, const size_t max);
	size_t getProgress(const CalibState cs, const size_t progress, const size_t max);

	void xnStepWritten(void*, void*);
	void xnStepWriteError(void*, void*);

	// Steps interpolation = IP
	unsigned m_thisIPleft;
	unsigned m_thisIPstep;
	unsigned m_thisIPright;
	unsigned getIPpower(const unsigned left, const unsigned right, const unsigned step);

	void interpolateNext();
	void xnIPWritten(void*, void*);
	void xnIPError(void*, void*);

	void done();
	void error(const Cm::CmError, const unsigned step);

	void initCVs();
	void initCVsOk(void*, void*);
	void initCVsError(void*, void*);
	void initCVsWriteNext();
	void initSTWritten(void*, void*);

private slots:
	void csDone(unsigned step, unsigned power);
	void csError(Cs::CsError, unsigned step);

	void coDone();
	void coError(Co::CoError, unsigned step);
	void coProgressUpdate(size_t progress, size_t max);

	void cStepPowerChanged(unsigned step, unsigned power);

signals:
	void onStepStart(unsigned step);
	void onStepDone(unsigned step, unsigned power);

	void onDone();
	void onError(Cm::CmError, unsigned step);

	void onLocoSpeedChanged(unsigned step);
	void onStepPowerChanged(unsigned step, unsigned power);
	void onAccelChanged(unsigned accel);
	void onDecelChanged(unsigned decel);

	void onProgressUpdate(size_t val); // value 0-100
};

}//namespace Cm

#endif
