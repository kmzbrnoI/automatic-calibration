#ifndef _CALIB_OVERVIEW_H_
#define _CALIB_OVERVIEW_H_

/*
This file defines a CalibOverview class which manages creating of a basic
overview of power-to-speed mapping. This overview is generated at beginnging
of the calibration process to make some basic knowledge of power-to-speed
mapping of a specific vehicle. The process is started by callong makeOverview
function. It could be manually stopped anytime by calling stop() function.
The process ends either by calling done() XOR on_error() event.

The whole process goes as followed:

 1) Set power.
 2) If spped > threshold, create new entry in power-to-speed graph.
 3) GOTO 1) (for another power).

Powers are tested from lowest to highest based on this procedure:

 1) Start with _START_STEP and wait for speed to be >= _MIN_SPEED.
    If speed is low, increase power _OVERVIEW_STEP-times.
 2) Once the power-to-speed map for low speed is measured, go to higher speeds.
 3) Test 64, 128, 192 and 255 power when the speed is < max_speed (we do not
    need info about power-to-speed mapping for higher speeds [and it might
    be dangerous to run vehicle too fast]).

Conclusion: at the end of this procedure, we know the power of minimum speed
of the loco and the power of maximum speed of the loco.
*/

#include <QObject>
#include <QTimer>

#include "lib/xn/xn.h"
#include "lib/wsm/wsm.h"
#include "power-map.h"

namespace Co {

const unsigned _SP_ADAPT_TIMEOUT = 2000; // 2 s
const double _MAX_DIFFUSION = 3; // 3 kmph
const unsigned _MEASURE_COUNT = 30; // measuring 30 values = 3 s
const unsigned _ADAPT_MAX_TICKS = 3; // maximum adaptation ticks
const unsigned _OVERVIEW_STEP = 2;
const unsigned _OVERVIEW_DEFAULT = 10;
const unsigned _SPEED_MAX = 120;
const unsigned _START_STEP = 10;
const unsigned _POWER_CNT = 256;
const unsigned _CV_START = 67; // cv 67 = step 1
const unsigned _MIN_SPEED = 5; // 5 kmph

enum class CoError {
	LargeDiffusion,
	XnNoResponse,
};

class CalibOverview : public QObject {
	Q_OBJECT

public:
	CalibOverview(Xn::XpressNet& xn, Pm::PowerToSpeedMap& pm, Wsm::Wsm& wsm,
	              unsigned max_speed = _SPEED_MAX, QObject *parent = nullptr);
	void makeOverview(const unsigned loco_addr);
	void stop();

	unsigned max_speed;

private:
	Xn::XpressNet& m_xn;
	Pm::PowerToSpeedMap& m_pm;
	Wsm::Wsm& m_wsm;

	unsigned m_loco_addr;
	QTimer t_sp_adapt;
	unsigned m_diff_count;
	unsigned m_last_power;

	void xn_pom_ok(void*, void*);
	void xn_pom_err(void*, void*);

	std::unique_ptr<unsigned> next_step();

	void do_next_step();
	void reset_step();

private slots:
	void wsm_lt_read(double speed, double diffusion);
	void wsm_lt_error();
	void t_sp_adapt_tick();

signals:
	void on_error(Co::CoError, unsigned step);
	void done();
	void step_power_changed(unsigned step, unsigned power);
	void progress_update(size_t progress, size_t max);
};

}//namespace Co

#endif
