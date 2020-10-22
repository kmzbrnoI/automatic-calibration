#include <cmath>

#include "calib-range.h"

namespace Cr {

CalibRange::CalibRange(Xn::XpressNet &xn, Wsm::Wsm &wsm, QObject *parent)
    : QObject(parent), m_xn(xn), m_wsm(wsm) {}

void CalibRange::measure(const unsigned loco_addr, const unsigned step, Xn::Direction dir, unsigned spkmph) {
	m_loco_addr = loco_addr;
	m_step = step;
	m_dir = dir;
	m_expected_speed_kmph = spkmph;

	loco_go();
}

void CalibRange::loco_go() {
	m_xn.setSpeed(
		Xn::LocoAddr(m_loco_addr),
		m_step,
		m_dir,
		std::make_unique<Xn::Cb>([this](void *s, void *d) { xn_speed_ok(s, d); }),
		std::make_unique<Xn::Cb>([this](void *s, void *d) { xn_speed_err(s, d); })
	);
}

void CalibRange::loco_stop() {
	m_xn.setSpeed(
		Xn::LocoAddr(m_loco_addr),
		0,
		m_dir,
		nullptr,
		std::make_unique<Xn::Cb>([this](void *s, void *d) { xn_speed_err(s, d); })
	);
}

void CalibRange::wsm_dist_read(double, uint32_t dist_raw) {
	// Once the distance in read, stop the loco and record distance
	QObject::disconnect(&m_wsm, SIGNAL(distanceRead(double, uint32_t)), this, SLOT(wsm_dist_read(double, uint32_t)));
	QObject::connect(&m_wsm, SIGNAL(speedRead(double, uint16_t)), this, SLOT(wsm_speed_read(double, uint16_t)));

	loco_stop();
	m_start_dist = dist_raw;
}

void CalibRange::wsm_speed_read(double speed, uint16_t) {
	// Reading distance when decelarating
	if (speed > 0 || m_end_dist != m_wsm.distRaw()) {
		m_stop_counter = 0;
		m_end_dist = m_wsm.distRaw();
		return;
	}
	if (m_stop_counter < stop_min) {
		m_stop_counter++;
		return;
	}

	// Loco stopped
	QObject::disconnect(&m_wsm, SIGNAL(speedRead(double, uint16_t)), this,
	                    SLOT(wsm_speed_read(double, uint16_t)));
	QObject::disconnect(&m_wsm, SIGNAL(speedReceiveTimeout()), this, SLOT(wsm_error()));

	measured(m_wsm.calcDist(m_wsm.distRaw() - m_start_dist));
}

void CalibRange::wsm_lt_read(double speed, double diffusion) {
	if (diffusion > MAX_DIFFUSION || std::abs(speed - m_expected_speed_kmph) > EPSILON) {
		if (m_speed_err_count >= ADAPT_MAX_TICKS ||
				(m_speed_err_count > 0 && speed < 10)) {
			disconnect_signals();
			loco_stop();
			on_error(CrError::SpeedMeasure, m_step);
			return;
		}
		m_speed_err_count++;
		start_lt();
		return;
	}

	// Speed ok -> stop & measure distance
	QObject::connect(&m_wsm, SIGNAL(distanceRead(double, uint32_t)), this,
	                 SLOT(wsm_dist_read(double, uint32_t)));
}

void CalibRange::start_lt() {
	try {
		m_wsm.startLongTermMeasure(MEASURE_COUNT);
	}
	catch (const Wsm::QStrException& e) {
		wsm_error();
	}
}

void CalibRange::xn_speed_ok(void *, void *) {
	// Insert 'waiting of mark' here when neccessarry
	QObject::connect(&m_wsm, SIGNAL(longTermMeasureDone(double, double)), this,
	                 SLOT(wsm_lt_read(double, double)));
	QObject::connect(&m_wsm, SIGNAL(speedReceiveTimeout()), this, SLOT(wsm_error()));
	m_speed_err_count = 0;
	start_lt();
}

void CalibRange::xn_speed_err(void *, void *) { on_error(CrError::XnNoResponse, m_step); }

void CalibRange::wsm_error() {
	disconnect_signals();
	loco_stop();
	on_error(CrError::WsmNoResponse, m_step);
}

void CalibRange::disconnect_signals() {
	QObject::disconnect(&m_wsm, SIGNAL(distanceRead(double, uint32_t)), this, SLOT(wsm_dist_read(double, uint32_t)));
	QObject::disconnect(&m_wsm, SIGNAL(longTermMeasureDone(double, double)), this,
	                    SLOT(wsm_lt_read(double, double)));
	QObject::disconnect(&m_wsm, SIGNAL(speedRead(double, uint16_t)), this,
	                    SLOT(wsm_speed_read(double, uint16_t)));
	QObject::disconnect(&m_wsm, SIGNAL(speedReceiveTimeout()), this, SLOT(wsm_error()));
}

} // namespace Cr
