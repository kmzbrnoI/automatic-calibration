#include <cmath>

#include "calib-overview.h"
#include "speed-map.h"

namespace Co {

CalibOverview::CalibOverview(Xn::XpressNet& xn, Pm::PowerToSpeedMap& pm, Wsm::Wsm& wsm,
                             unsigned max_speed, QObject *parent)
	: QObject(parent), max_speed(max_speed), m_xn(xn), m_pm(pm), m_wsm(wsm) {
	t_sp_adapt.setSingleShot(true);
	QObject::connect(&t_sp_adapt, SIGNAL(timeout()), this, SLOT(t_sp_adapt_tick()));
}

std::unique_ptr<unsigned> CalibOverview::next_step() {
	size_t step = _START_STEP;

	float *speed = m_pm.speed(step);
	while (nullptr == speed || *speed == 0) {
		if (nullptr == speed)
			return std::make_unique<unsigned>(step);

		step *= 2;
		if (step >= _POWER_CNT-1)
			return nullptr;

		speed = m_pm.speed(step);
	}

	if (!m_pm.isRecord(64))
		return std::make_unique<unsigned>(64);
	if (*m_pm.speed(64) >= max_speed)
		return nullptr;

	if (!m_pm.isRecord(128))
		return std::make_unique<unsigned>(128);
	if (*m_pm.speed(128) >= max_speed)
		return nullptr;

	if (!m_pm.isRecord(192))
		return std::make_unique<unsigned>(192);
	if (*m_pm.speed(192) >= max_speed)
		return nullptr;

	if (!m_pm.isRecord(255))
		return std::make_unique<unsigned>(255);
	if (*m_pm.speed(255) >= max_speed)
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

	m_xn.pomWriteCv(
		Xn::LocoAddr(m_loco_addr),
		_CV_START - 1 + _OVERVIEW_STEP,
		m_last_power,
		std::make_unique<Xn::XnCb>([this](void *s, void *d) { xn_pom_ok(s, d); }),
		std::make_unique<Xn::XnCb>([this](void *s, void *d) { xn_pom_err(s, d); })
	);
	step_power_changed(_OVERVIEW_STEP, m_last_power);
}

void CalibOverview::wsm_lt_read(double speed, double diffusion) {
	QObject::disconnect(&m_wsm, SIGNAL(longTermMeasureDone(double, double)), this, SLOT(wsm_lt_read(double, double)));
	QObject::disconnect(&m_wsm, SIGNAL(speedReceiveTimeout()), this, SLOT(wsm_lt_error()));

	if (speed < _MIN_SPEED) {
		// When speed is too low, it may happen that it chnges between zero
		// and some non/zero value. This causes low speed, but high diffusion.
		// We ignore those low speeds.
		do_next_step();
		return;
	}

	if (diffusion > _MAX_DIFFUSION) {
		if (m_diff_count >= _ADAPT_MAX_TICKS) {
			on_error(CoError::LargeDiffusion, _OVERVIEW_STEP);
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

void CalibOverview::xn_pom_ok(void*, void*) {
	// Insert 'waiting of mark' here when neccessarry
	t_sp_adapt.start(_SP_ADAPT_TIMEOUT);
}

void CalibOverview::xn_pom_err(void*, void*) {
	on_error(CoError::XnNoResponse, _OVERVIEW_STEP);
}

void CalibOverview::wsm_lt_error() {
	t_sp_adapt.stop();
	QObject::disconnect(&m_wsm, SIGNAL(longTermMeasureDone(double, double)), this, SLOT(wsm_lt_read(double, double)));
	QObject::disconnect(&m_wsm, SIGNAL(speedReceiveTimeout()), this, SLOT(wsm_lt_error()));
	reset_step();
}

void CalibOverview::reset_step() {
	m_xn.pomWriteCv(
		Xn::LocoAddr(m_loco_addr),
		_CV_START - 1 + _OVERVIEW_STEP,
		_OVERVIEW_DEFAULT
	);
	step_power_changed(_OVERVIEW_STEP, _OVERVIEW_DEFAULT);
}

void CalibOverview::stop() {
	wsm_lt_error();
}

}//namespace Co
