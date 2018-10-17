#ifndef _CALIB_STEP_H_
#define _CALIB_STEP_H_

/*
This fule defines a CalibStep class which manages calibration of a single
step. Calibration is started by calling calibrate() function and ends either
by calling done XOR on_error function. The process could be manually stopped
wither by calling stop() function or stopping the locomotive manually.
Once speed=0 is measured, the process is interrupted and on_error event is
called.

The whole process is based on on-line updating of a speed-to-power graph
(represented by Pm::PowerToSpeedMap instance).

This class does NOT set speed step of a loco. Caller must set the calibrated
speed step.

 1) Set power based on intended speed and power-to-speed graph.
 2) Wait for speed adaptation for some constant time.
 3) Wait for low-diffusion of a measured speed.
 4) Once diffusion is low, add new entry to power-to-speed graph.
    (a) When the meaured speed is epsilon-close to target speed, end calibration.
    (b) Otherwise, GOTO 1).
*/

#include <QObject>
#include <QTimer>

#include "lib/xn/xn.h"
#include "lib/wsm/wsm.h"
#include "power-map.h"

namespace Cs {

const double _DEFAULT_EPSILON = 1; // +- 1 kmph
const unsigned _SP_ADAPT_TIMEOUT = 2000; // 2 s
const double _MAX_DIFFUSION = 3; // 3 kmph
const unsigned _CV_START = 67; // cv 67 = step 1
const unsigned _MEASURE_COUNT = 30; // measuring 30 values = 3 s
const unsigned _ADAPT_MAX_TICKS = 3; // maximum adaptation ticks

enum class CsError {
	LargeDiffusion,
	XnNoResponse,
	LocoStopped,
	NoStep,
};

class CalibStep : public QObject {
	Q_OBJECT

public:
	double epsilon = _DEFAULT_EPSILON;
	double max_diffusion = _MAX_DIFFUSION;

	CalibStep(Xn::XpressNet& xn, Pm::PowerToSpeedMap& pm, Wsm::Wsm& wsm,
	          QObject *parent = nullptr);
	void calibrate(const unsigned loco_addr, const unsigned step,
	               const double speed);
	void stop();

private:
	Xn::XpressNet& m_xn;
	Pm::PowerToSpeedMap& m_pm;
	Wsm::Wsm& m_wsm;
	unsigned m_loco_addr;
	unsigned m_step;
	double m_target_speed;

	QTimer t_sp_adapt;
	unsigned m_last_power;
	unsigned m_diff_count;

	void xn_pom_ok(void*, void*);
	void xn_pom_err(void*, void*);

private slots:
	void wsm_lt_read(double speed, double diffusion);
	void wsm_lt_error();
	void t_sp_adapt_tick();

signals:
	void on_error(Cs::CsError, unsigned step);
	void done(unsigned step, unsigned power);
	void step_power_changed(unsigned step, unsigned power);
};

}//namespace Cs

#endif
