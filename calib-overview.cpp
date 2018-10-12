#include <cmath>

#include "calib-overview.h"
#include "speed-map.h"

namespace Co {

CalibOverview::CalibOverview(Xn::XpressNet& xn, Pm::PowerToSpeedMap& pm, Wsm::Wsm& wsm,
                     QObject *parent)
	: QObject(parent), m_xn(xn), m_pm(pm), m_wsm(wsm) {
	t_sp_adapt.setSingleShot(true);
	QObject::connect(&t_sp_adapt, SIGNAL(timeout()), this, SLOT(t_sp_adapt_tick()));
}

std::unique_ptr<unsigned> CalibOverview::next_step() {
	size_t step = _START_STEP;

	float* speed = m_pm.speed(step);
	while (nullptr == speed || *speed == 0) {
		if (nullptr == speed)
			return std::make_unique<unsigned>(step);

		step *= 2;
		if (step >= _STEPS_CNT-1)
			return nullptr;

		speed = m_pm.speed(step);
	}

	if (!m_pm.isRecord(128))
		return std::make_unique<unsigned>(128);
	if (*m_pm.speed(128) >= _SPEED_MAX)
		return nullptr;

	if (!m_pm.isRecord(192))
		return std::make_unique<unsigned>(192);
	if (*m_pm.speed(192) >= _SPEED_MAX)
		return nullptr;

	if (!m_pm.isRecord(255))
		return std::make_unique<unsigned>(255);
	if (*m_pm.speed(255) >= _SPEED_MAX)
		return nullptr;

	return nullptr;
}

void CalibOverview::makeOverview(const unsigned loco_addr) {
	m_loco_addr = loco_addr;
	m_diff_count = 0;

	do_next_step();
}

void CalibOverview::do_next_step() {
	std::unique_ptr<unsigned> next = next_step();
	if (nullptr == next) {
		// Set some "normal" value to step 1
		reset_step();
		done();
		return;
	}
	m_last_power = *next;

	m_xn.PomWriteCv(
		Xn::LocoAddr(m_loco_addr),
		_CV_START - 1 + _OVERVIEW_STEP,
		m_last_power,
		std::make_unique<Xn::XnCb>(&xns_pom_ok, this),
		std::make_unique<Xn::XnCb>(&xns_pom_err, this)
	);
	step_power_changed(_OVERVIEW_STEP, m_last_power);
}

void CalibOverview::wsm_lt_read(double speed, double diffusion) {
	QObject::disconnect(&m_wsm, SIGNAL(longTermMeasureDone(double, double)), this, SLOT(wsm_lt_read(double, double)));
	QObject::disconnect(&m_wsm, SIGNAL(speedReceiveTimeout()), this, SLOT(wsm_lt_error()));

	if (diffusion > _MAX_DIFFUSION) {
		if (m_diff_count >= _ADAPT_MAX_TICKS) {
			diffusion_error();
			return;
		}
		// Wait for speed...
		m_diff_count++;
		t_sp_adapt_tick();
		return;
	}

	m_pm.addOrUpdate(m_last_power, speed);
	do_next_step();
}

void CalibOverview::t_sp_adapt_tick() {
	QObject::connect(&m_wsm, SIGNAL(longTermMeasureDone(double, double)), this, SLOT(wsm_lt_read(double, double)));
	QObject::connect(&m_wsm, SIGNAL(speedReceiveTimeout()), this, SLOT(wsm_lt_error()));
	m_wsm.startLongTermMeasure(_MEASURE_COUNT);
}

void CalibOverview::xns_pom_ok(void* s, void* d) { static_cast<CalibOverview*>(d)->xn_pom_ok(s, d); }
void CalibOverview::xns_pom_err(void* s, void* d) { static_cast<CalibOverview*>(d)->xn_pom_err(s, d); }

void CalibOverview::xn_pom_ok(void*, void*) {
	// Insert 'waiting of mark' here when neccessarry
	t_sp_adapt.start(_SP_ADAPT_TIMEOUT);
}

void CalibOverview::xn_pom_err(void*, void*) {
	xn_error();
}

void CalibOverview::wsm_lt_error() {
	QObject::disconnect(&m_wsm, SIGNAL(longTermMeasureDone(double, double)), this, SLOT(wsm_lt_read(double, double)));
	QObject::disconnect(&m_wsm, SIGNAL(speedReceiveTimeout()), this, SLOT(wsm_lt_error()));
	reset_step();
}

void CalibOverview::reset_step() {
	m_xn.PomWriteCv(
		Xn::LocoAddr(m_loco_addr),
		_CV_START - 1 + _OVERVIEW_STEP,
		_OVERVIEW_DEFAULT
	);
	step_power_changed(_OVERVIEW_STEP, _OVERVIEW_DEFAULT);
}

}//end namespace
