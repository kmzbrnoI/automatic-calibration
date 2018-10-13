#ifndef _CALIB_OVERVIEW_H_
#define _CALIB_OVERVIEW_H_

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

enum class CoError {
	LargeDiffusion,
	XnNoResponse,
};

class CalibOverview : public QObject {
	Q_OBJECT

public:
	CalibOverview(Xn::XpressNet& xn, Pm::PowerToSpeedMap& pm, Wsm::Wsm& wsm,
	              QObject *parent = nullptr);
	void makeOverview(const unsigned loco_addr);
	void stop();

private:
	Xn::XpressNet& m_xn;
	Pm::PowerToSpeedMap& m_pm;
	Wsm::Wsm& m_wsm;

	unsigned m_loco_addr;
	QTimer t_sp_adapt;
	unsigned m_diff_count;
	unsigned m_last_power;

	static void xns_pom_ok(void*, void*);
	static void xns_pom_err(void*, void*);
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
};

}//end namespace

#endif
