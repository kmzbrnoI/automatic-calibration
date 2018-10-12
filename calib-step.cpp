#include <cmath>

#include "calib-step.h"

namespace Cs {

CalibStep::CalibStep(Xn::XpressNet& xn, Pm::PowerToSpeedMap& pm, Wsm::Wsm& wsm,
                     QObject *parent)
	: QObject(parent), m_xn(xn), m_pm(pm), m_wsm(wsm) {
	t_sp_adapt.setSingleShot(true);
	QObject::connect(&t_sp_adapt, SIGNAL(timeout()), this, SLOT(t_sp_adapt_tick()));
}

void CalibStep::calibrate(const unsigned loco_addr, const unsigned step,
                          const double speed, const double epsilon) {
	m_loco_addr = loco_addr;
	m_step = step;
	m_target_speed = speed;
	m_epsilon = epsilon;
	m_diff_count = 0;

	try {
		m_last_power = m_pm.steps(m_target_speed);
	}
	catch (const Pm::ENoMap&) {
		on_error(CsError::NoStep, m_step);
		return;
	}

	m_xn.PomWriteCv(
		Xn::LocoAddr(m_loco_addr),
		_CV_START - 1 + m_step,
		m_last_power,
		std::make_unique<Xn::XnCb>(&xns_pom_ok, this),
		std::make_unique<Xn::XnCb>(&xns_pom_err, this)
	);
	step_power_changed(m_step, m_last_power);
}

void CalibStep::wsm_lt_read(double speed, double diffusion) {
	QObject::disconnect(&m_wsm, SIGNAL(longTermMeasureDone(double, double)), this, SLOT(wsm_lt_read(double, double)));
	QObject::disconnect(&m_wsm, SIGNAL(speedReceiveTimeout()), this, SLOT(wsm_lt_error()));

	if (diffusion > _MAX_DIFFUSION) {
		if (m_diff_count >= _ADAPT_MAX_TICKS) {
			on_error(CsError::LargeDiffusion, m_step);
			return;
		}
		// Wait for speed...
		m_diff_count++;
		t_sp_adapt_tick();
		return;
	}
	if (speed == 0) {
		on_error(CsError::LocoStopped, m_step);
		return;
	}

	m_pm.addOrUpdate(m_last_power, speed);

	if (std::abs(speed-m_target_speed) < m_epsilon) {
		done(m_step, m_last_power);
		return;
	}

	unsigned new_power;
	try {
		new_power = m_pm.steps(m_target_speed);
	}
	catch (const Pm::ENoMap&) {
		on_error(CsError::NoStep, m_step);
		return;
	}

	// Manually increase step when step too small
	if (std::abs(static_cast<int>(m_last_power)-static_cast<int>(new_power)) < 3) {
		if (new_power < m_last_power)
			new_power = m_last_power - 3;
		else
			new_power = m_last_power + 3;
	}
	m_last_power = new_power;

	m_xn.PomWriteCv(
		Xn::LocoAddr(m_loco_addr),
		_CV_START - 1 + m_step,
		m_last_power,
		std::make_unique<Xn::XnCb>(&xns_pom_ok, this),
		std::make_unique<Xn::XnCb>(&xns_pom_err, this)
	);

	step_power_changed(m_step, m_last_power);
}

void CalibStep::t_sp_adapt_tick() {
	QObject::connect(&m_wsm, SIGNAL(longTermMeasureDone(double, double)), this, SLOT(wsm_lt_read(double, double)));
	QObject::connect(&m_wsm, SIGNAL(speedReceiveTimeout()), this, SLOT(wsm_lt_error()));
	m_wsm.startLongTermMeasure(_MEASURE_COUNT);
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
	on_error(CsError::XnNoResponse, m_step);
}

void CalibStep::wsm_lt_error() {
	QObject::disconnect(&m_wsm, SIGNAL(longTermMeasureDone(double, double)), this, SLOT(wsm_lt_read(double, double)));
	QObject::disconnect(&m_wsm, SIGNAL(speedReceiveTimeout()), this, SLOT(wsm_lt_error()));
}

}//end namespace
