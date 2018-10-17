#include <cmath>

#include "calib-range.h"

namespace Cr {

CalibRange::CalibRange(Xn::XpressNet& xn, Wsm::Wsm& wsm, QObject *parent)
	: QObject(parent), m_xn(xn), m_wsm(wsm) {
	t_sp_adapt.setSingleShot(true);
	QObject::connect(&t_sp_adapt, SIGNAL(timeout()), this, SLOT(t_sp_adapt_tick()));
}

void CalibRange::measure(const unsigned loco_addr, const unsigned step,
                         Xn::XnDirection dir) {
	m_loco_addr = loco_addr;
	m_step = step;
	m_dir = dir;

	QObject::connect(&m_wsm, SIGNAL(speedReceiveTimeout()), this, SLOT(wsm_error()));

	m_xn.setSpeed(
		Xn::LocoAddr(loco_addr),
		step,
		dir,
		std::make_unique<Xn::XnCb>([this](void *s, void *d) { xn_speed_ok(s, d); }),
		std::make_unique<Xn::XnCb>([this](void *s, void *d) { xn_speed_err(s, d); })
	);
}

void CalibRange::wsm_dist_read(double, uint32_t dist_raw) {
	// Once the distance in read, stop the loco and record distance
	QObject::disconnect(&m_wsm, SIGNAL(distanceRead(double, uint32_t)), this, SLOT(wsm_dist_read(double, uint32_t)));
	QObject::connect(&m_wsm, SIGNAL(speedRead(double, uint16_t)), this, SLOT(wsm_speed_read(double, uint16_t)));

	m_xn.setSpeed(
		Xn::LocoAddr(m_loco_addr),
		0,
		m_dir,
		nullptr,
		std::make_unique<Xn::XnCb>([this](void *s, void *d) { xn_speed_err(s, d); })
	);
	m_start_dist = dist_raw;
}

void CalibRange::wsm_speed_read(double speed, uint16_t) {
	// Reding distance when decelarating
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
	QObject::disconnect(&m_wsm, SIGNAL(speedRead(double, uint16_t)), this, SLOT(wsm_speed_read(double, uint16_t)));
	QObject::disconnect(&m_wsm, SIGNAL(speedReceiveTimeout()), this, SLOT(wsm_error()));

	measured(m_wsm.calcDist(m_wsm.distRaw() - m_start_dist));
}

void CalibRange::t_sp_adapt_tick() {
	QObject::connect(&m_wsm, SIGNAL(distanceRead(double, uint32_t)), this, SLOT(wsm_dist_read(double, uint32_t)));
}

void CalibRange::xn_speed_ok(void*, void*) {
	// Insert 'waiting of mark' here when neccessarry
	t_sp_adapt.start(sp_adapt_timeout);
}

void CalibRange::xn_speed_err(void*, void*) {
	on_error(CrError::XnNoResponse, m_step);
}

void CalibRange::wsm_error() {
	QObject::disconnect(&m_wsm, SIGNAL(speedRead(double, uint16_t)), this, SLOT(wsm_speed_read(double, uint16_t)));
	QObject::disconnect(&m_wsm, SIGNAL(speedReceiveTimeout()), this, SLOT(wsm_error()));
}

}//namespace Cr
