#include <cmath>

#include "calib-overview.h"
#include "speed-map.h"

namespace Co {

CalibOverview::CalibOverview(Xn::XpressNet &xn, Pm::PowerToSpeedMap &pm, Wsm::Wsm &wsm,
                             unsigned max_speed, QObject *parent)
    : QObject(parent), max_speed(max_speed), m_xn(xn), m_pm(pm), m_wsm(wsm) {
	t_sp_adapt.setSingleShot(true);
	QObject::connect(&t_sp_adapt, SIGNAL(timeout()), this, SLOT(t_sp_adapt_tick()));
}

std::unique_ptr<unsigned> CalibOverview::next_step() {
	size_t step = overview_start;

	float const *speed = m_pm.speed(step);
	while (nullptr == speed || *speed == 0) {
		if (nullptr == speed)
			return std::make_unique<unsigned>(step);

		step *= 2;
		if (step >= POWER_CNT-1)
			return nullptr;

		speed = m_pm.speed(step);
	}

	constexpr std::array<unsigned, 4> POWERS_TRY = {64, 128, 192, 255};

	for (const unsigned &power : POWERS_TRY) {
		if (!m_pm.isRecord(power))
			return std::make_unique<unsigned>(power);
		if (*m_pm.speed(power) >= max_speed)
			return nullptr;
	}

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
		emit done();
		return;
	}
	m_last_power = *next;
	emit progress_update(m_last_power, POWER_CNT);

	this->pom_write_power(
		m_last_power,
		std::make_unique<Xn::Cb>([this](void *s, void *d) { xn_pom_ok(s, d); }),
		std::make_unique<Xn::Cb>([this](void *s, void *d) { xn_pom_err(s, d); })
	);
}

void CalibOverview::wsm_lt_read(double speed, double diffusion) {
	QObject::disconnect(&m_wsm, SIGNAL(longTermMeasureDone(double, double)), this,
	                    SLOT(wsm_lt_read(double, double)));
	QObject::disconnect(&m_wsm, SIGNAL(speedReceiveTimeout()), this, SLOT(wsm_lt_error()));

	if (speed < min_speed) {
		// When speed is too low, it may happen that it chnges between zero
		// and some non/zero value. This causes low speed, but high diffusion.
		// We ignore those low speeds.
		m_pm.addOrUpdate(m_last_power, 0);
		do_next_step();
		return;
	}

	if (diffusion > max_abs_diffusion && diffusion > speed*max_rel_diffusion) {
		if (m_diff_count >= ADAPT_MAX_TICKS) {
			emit on_error(Co::Error::LargeDiffusion, overview_step);
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
	QObject::connect(&m_wsm, SIGNAL(longTermMeasureDone(double,double)), this,
	                 SLOT(wsm_lt_read(double,double)));
	QObject::connect(&m_wsm, SIGNAL(speedReceiveTimeout()), this, SLOT(wsm_lt_error()));
	m_wsm.startLongTermMeasure(measure_count);
}

void CalibOverview::xn_pom_ok(void *, void *) {
	// Insert 'waiting on mark' here when necessarry
	t_sp_adapt.start(sp_adapt_timeout);
}

void CalibOverview::xn_pom_err(void *, void *) {
	emit on_error(Co::Error::XnNoResponse, overview_step);
}

void CalibOverview::wsm_lt_error() {
	t_sp_adapt.stop();
	QObject::disconnect(&m_wsm, SIGNAL(longTermMeasureDone(double, double)), this,
	                    SLOT(wsm_lt_read(double, double)));
	QObject::disconnect(&m_wsm, SIGNAL(speedReceiveTimeout()), this, SLOT(wsm_lt_error()));
	reset_step();
}

void CalibOverview::reset_step() {
	this->pom_write_power(STEP_RESET_VALUE);
}

void CalibOverview::stop() { wsm_lt_error(); }

void CalibOverview::pom_write_power(unsigned power, std::unique_ptr<Xn::Cb> ok, std::unique_ptr<Xn::Cb> err) {
	if (overview_step > 1) {
		m_xn.pomWriteCv(
			Xn::LocoAddr(m_loco_addr),
			CV_START - 1 + overview_step - 1,
			power
		);
		emit step_power_changed(overview_step-1, power);
	}

	m_xn.pomWriteCv(
		Xn::LocoAddr(m_loco_addr),
		CV_START - 1 + overview_step,
		power,
		std::move(ok),
		std::move(err)
	);
	emit step_power_changed(overview_step, power);

	if (overview_step < Xn::_STEPS_CNT) {
		m_xn.pomWriteCv(
			Xn::LocoAddr(m_loco_addr),
			CV_START - 1 + overview_step + 1,
			power
		);
		emit step_power_changed(overview_step+1, power);
	}
}

} // namespace Co
