#ifndef _CALIB_M_H_
#define _CALIB_M_H_

#include <QObject>
#include <QTimer>

#include "lib/xn/xn.h"
#include "lib/wsm/measure-car.h"
#include "power-map.h"

namespace Cm {

const double _DEFAULT_EPSILON = 0.5; // +- 0.7 kmph
const unsigned _SP_ADAPT_TIMEOUT = 2000; // 2 s
const double _MAX_DIFFUSION = 3; // 3 kmph
const unsigned _CV_START = 67; // cv 67 = step 1

class CalibStep : public QObject {
	Q_OBJECT

public:
	CalibStep(Xn::XpressNet& xn, Pm::PowerToSpeedMap& pm, QObject *parent = nullptr);
	void calibrate(const unsigned loco_addr, const unsigned step,
	               const double speed, const double epsilon = _DEFAULT_EPSILON);

private:
	Xn::XpressNet& m_xn;
	Pm::PowerToSpeedMap& m_pm;

	unsigned m_loco_addr;
	unsigned m_step;
	double m_target_speed;
	double m_epsilon;
	QTimer t_sp_adapt;
	unsigned m_last_power;

	static void xns_pom_ok(void*, void*);
	static void xns_pom_err(void*, void*);
	void xn_pom_ok(void*, void*);
	void xn_pom_err(void*, void*);

private slots:
	void wsm_lt_read(double speed, double diffusion);
	void t_sp_adapt_tick();


signals:
	void diffusion_error();
	void loco_stopped();
	void done();
	void xn_error();
};

}//end namespace

#endif
