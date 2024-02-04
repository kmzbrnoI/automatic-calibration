#ifndef CALIB_OVERVIEW_H
#define CALIB_OVERVIEW_H

/*
This file defines a CalibOverview class which manages creating of a basic
overview of power-to-speed mapping. This overview is generated at beginning
of the calibration process to make some basic knowledge of power-to-speed
mapping of a specific vehicle. The process is started by callong makeOverview
function. It could be manually stopped anytime by calling stop() function.
The process ends either by calling done() XOR on_error() event.

The whole process goes as followed:

 1) Set power.
 2) If speed > threshold, create new entry in power-to-speed graph.
 3) GOTO 1) (for another power).

Powers are tested from lowest to highest based on this procedure:

 1) Start with START_STEP and wait for speed to be >= MIN_SPEED.
    If speed is low, increase power OVERVIEW_STEP-times.
 2) Once the power-to-speed map for low speed is measured, go to higher speeds.
 3) Test 64, 128, 192 and 255 power when the speed is < max_speed (we do not
    need info about power-to-speed mapping for higher speeds [and it might
    be dangerous to run vehicle too fast]).

Conclusion: at the end of this procedure, we know the power of minimum speed
of the loco and the power of maximum speed of the loco.
*/

#include <QObject>
#include <QTimer>

#include "lib/wsm/wsm.h"
#include "lib/xn/xn.h"
#include "power-map.h"

namespace Co {

constexpr unsigned DEFAULT_SP_ADAPT_TIMEOUT = 2000; // speed timeout, 2 s
constexpr double DEFAULT_MAX_ABS_DIFFUSION = 1; // kmph
constexpr double DEFAULT_MAX_REL_DIFFUSION = 0.06; // 6 %
constexpr unsigned DEFAULT_MEASURE_COUNT = 30; // measuring 30 values = 3 s
constexpr unsigned DEFAULT_SPEED_MAX = 120;
constexpr unsigned DEFAULT_OVERVIEW_STEP = 2;
constexpr unsigned DEFAULT_OVERVIEW_START = 10;
constexpr unsigned DEFAULT_MIN_SPEED = 5; // kmph

constexpr unsigned STEP_RESET_VALUE = 10;
constexpr unsigned ADAPT_MAX_TICKS = 3; // maximum adaptation ticks
constexpr unsigned POWER_CNT = 256;
constexpr unsigned CV_START = 67; // cv 67 = step 1

enum class Error {
	LargeDiffusion,
	XnNoResponse,
};

class CalibOverview : public QObject {
	Q_OBJECT

public:
	unsigned sp_adapt_timeout = DEFAULT_SP_ADAPT_TIMEOUT;
	double max_abs_diffusion = DEFAULT_MAX_ABS_DIFFUSION;
	double max_rel_diffusion = DEFAULT_MAX_REL_DIFFUSION;
	unsigned measure_count = DEFAULT_MEASURE_COUNT;
	unsigned overview_step = DEFAULT_OVERVIEW_STEP;
	unsigned overview_start = DEFAULT_OVERVIEW_START;
	unsigned min_speed = DEFAULT_MIN_SPEED;

	CalibOverview(Xn::XpressNet &xn, Pm::PowerToSpeedMap &pm, Wsm::Wsm &wsm,
	              unsigned max_speed = DEFAULT_SPEED_MAX, QObject *parent = nullptr);
	void makeOverview(unsigned loco_addr);
	void stop();

	unsigned max_speed;

private:
	Xn::XpressNet &m_xn;
	Pm::PowerToSpeedMap &m_pm;
	Wsm::Wsm &m_wsm;

	unsigned m_loco_addr;
	QTimer t_sp_adapt;
	unsigned m_diff_count;
	unsigned m_last_power;

	void xn_pom_ok(void *, void *);
	void xn_pom_err(void *, void *);

	std::unique_ptr<unsigned> next_step();

	void do_next_step();
	void reset_step();

private slots:
	void wsm_lt_read(double speed, double diffusion);
	void wsm_lt_error();
	void t_sp_adapt_tick();

signals:
	void on_error(Co::Error, unsigned step);
	void done();
	void step_power_changed(unsigned step, unsigned power);
	void progress_update(size_t progress, size_t max);
};

} // namespace Co

#endif
