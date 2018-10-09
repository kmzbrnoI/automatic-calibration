#include <cmath>

#include "calib-man.h"

namespace Cm {

CalibStep::CalibStep(Xn::XpressNet& xn, Pm::PowerToSpeedMap& pm, QObject *parent)
	: QObject(parent), m_xn(xn), m_pm(pm) {
	t_sp_adapt.setSingleShot(true);
	QObject::connect(&t_sp_adapt, SIGNAL(timeout()), this, SLOT(t_sp_adapt_tick()));
}

void CalibStep::calibrate(const unsigned loco_addr, const unsigned step,
                          const double speed, const double epsilon) {
	m_loco_addr = loco_addr;
	m_step = step;
	m_target_speed = speed;
	m_epsilon = epsilon;

	m_last_power = m_pm.steps(m_target_speed);

	m_xn.PomWriteCv(
		Xn::LocoAddr(m_loco_addr),
		_CV_START - 1 + m_step,
		m_last_power,
		std::make_unique<Xn::XnCb>(&xns_pom_ok, this),
		std::make_unique<Xn::XnCb>(&xns_pom_err, this)
	);
}

void CalibStep::wsm_lt_read(double speed, double diffusion) {
	if (diffusion > _MAX_DIFFUSION) {
		diffusion_error();
		return;
	}
	if (speed == 0) {
		loco_stopped();
		return;
	}

	if (std::abs(speed-m_target_speed) < m_epsilon) {
		done();
		return;
	}

	m_pm.addOrUpdate(m_last_power, speed);
	m_last_power = m_pm.steps(m_target_speed);

	m_xn.PomWriteCv(
		Xn::LocoAddr(m_loco_addr),
		_CV_START - 1 + m_step,
		m_last_power,
		std::make_unique<Xn::XnCb>(&xns_pom_ok, this),
		std::make_unique<Xn::XnCb>(&xns_pom_err, this)
	);
}

void CalibStep::t_sp_adapt_tick() {
	// TODO: call WSM long time measuring init
}

void CalibStep::xns_pom_ok(void* s, void* d) { static_cast<CalibStep*>(d)->xn_pom_ok(s, d); }
void CalibStep::xns_pom_err(void* s, void* d) { static_cast<CalibStep*>(d)->xn_pom_err(s, d); }

void CalibStep::xn_pom_ok(void* source, void* data) {
	(void)source;
	(void)data;

	// Insert 'waiting of mark' here when neccessarry
	t_sp_adapt.start(_SP_ADAPT_TIMEOUT);
}

void CalibStep::xn_pom_err(void* source, void* data) {
	(void)source;
	(void)data;
	xn_error();
}

}//end namespace
