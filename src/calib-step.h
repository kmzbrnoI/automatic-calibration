#ifndef CALIB_STEP_H
#define CALIB_STEP_H

/*
This file defines a CalibStep class which manages calibration of a single
step. Calibration is started by calling calibrate() function and ends either
by calling done XOR on_error function. The process could be manually stopped
by calling stop() function or stopping the locomotive manually.
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
    (a) When the meaured speed is epsilon-close to target speed,
        end calibration of the step.
    (b) Otherwise, GOTO 1).
*/

#include <QObject>
#include <QTimer>
#include <vector>
#include <functional>

#include "lib/wsm/wsm.h"
#include "lib/xn/xn.h"
#include "power-map.h"

namespace Cs {

constexpr double DEFAULT_ABS_DEVIATION = 0.5; // +- x kmph
constexpr double DEFAULT_REL_DEVIATION = 0.025; // +- x %
constexpr double DEFAULT_MAX_ABS_DIFFUSION = 1; // kmph
constexpr double DEFAULT_MAX_REL_DIFFUSION = 0.06; // 6 %
constexpr size_t DEFAULT_MEASURE_COUNT = 30; // measuring 30 values = 3 s
constexpr unsigned DEFAULT_SP_ADAPT_TIMEOUT = 2000; // ms

constexpr unsigned CV_START = 67; // cv 67 = step 1
constexpr unsigned ADAPT_MAX_TICKS = 3; // maximum adaptation ticks
constexpr unsigned OSC_MAX_COUNT = 3; // frame length for oscilation detection

enum class CsError {
	LargeDiffusion,
	XnNoResponse,
	LocoStopped,
	NoStep,
	Oscilation,
	WsmError,
};

using NeighAsker = std::function<unsigned(unsigned midldeStep, unsigned neighStep)>;
using SetPower = std::function<unsigned(unsigned step)>;

class CalibStep : public QObject {
	Q_OBJECT

public:
	double abs_deviation = DEFAULT_ABS_DEVIATION; // this OR rel_deviation must match
	double rel_deviation = DEFAULT_REL_DEVIATION; // this OR abs_deviation must match
	double max_abs_diffusion = DEFAULT_MAX_ABS_DIFFUSION;
	double max_rel_diffusion = DEFAULT_MAX_REL_DIFFUSION;
	unsigned measure_count = DEFAULT_MEASURE_COUNT;
	unsigned sp_adapt_timeout = DEFAULT_SP_ADAPT_TIMEOUT;

	CalibStep(Xn::XpressNet &xn, Pm::PowerToSpeedMap &pm, Wsm::Wsm &wsm,
		const NeighAsker &neighAsker, const SetPower &setPower, QObject *parent = nullptr);
	void calibrate(unsigned loco_addr, unsigned step, double speed);
	void stop();

private:
	Xn::XpressNet &m_xn;
	Pm::PowerToSpeedMap &m_pm;
	Wsm::Wsm &m_wsm;
	unsigned m_loco_addr;
	unsigned m_step;
	double m_target_speed;
	const NeighAsker neighAsker;
	const SetPower setPower;

	QTimer t_sp_adapt;
	unsigned m_last_power;
	unsigned m_diff_count;

	std::vector<unsigned> power_history;

	void xn_pom_ok(void *, void *);
	void xn_pom_err(void *, void *);
	bool is_oscilating() const;
	void set_power(unsigned power);
	void pom_write_step(unsigned step, unsigned power, Xn::UPCb ok = {}, Xn::UPCb err = {});

private slots:
	void wsm_lt_read(double speed, double diffusion);
	void wsm_lt_error();
	void wsm_lt_done();
	void t_sp_adapt_tick();

signals:
	void on_error(Cs::CsError, unsigned step);
	void done(unsigned step, unsigned power);
	void step_power_changed(unsigned step, unsigned power);
};

} // namespace Cs

#endif
