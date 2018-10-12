#ifndef _CALIB_STEP_H_
#define _CALIB_STEP_H_

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
	CalibStep(Xn::XpressNet& xn, Pm::PowerToSpeedMap& pm, Wsm::Wsm& wsm,
	          QObject *parent = nullptr);
	void calibrate(const unsigned loco_addr, const unsigned step,
	               const double speed, const double epsilon = _DEFAULT_EPSILON);

private:
	Xn::XpressNet& m_xn;
	Pm::PowerToSpeedMap& m_pm;
	Wsm::Wsm& m_wsm;

	unsigned m_loco_addr;
	unsigned m_step;
	double m_target_speed;
	double m_epsilon;
	QTimer t_sp_adapt;
	unsigned m_last_power;
	unsigned m_diff_count;

	static void xns_pom_ok(void*, void*);
	static void xns_pom_err(void*, void*);
	void xn_pom_ok(void*, void*);
	void xn_pom_err(void*, void*);

private slots:
	void wsm_lt_read(double speed, double diffusion);
	void wsm_lt_error();
	void t_sp_adapt_tick();

signals:
	void on_error(CsError, unsigned step);
	void done(unsigned step, unsigned power);
	void step_power_changed(unsigned step, unsigned power);
};

}//end namespace

#endif
