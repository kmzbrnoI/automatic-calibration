#ifndef CALIB_RANGE_H
#define CALIB_RANGE_H

/*
This unit defines a CalibRange class which allows user to measure distance
while decelerating from 'step' addr.

 1) The loco speed step is set to 'step'.
 2) Application waits till loco reaches stable 'spkmph' speed.
 3) Right when next distance fom WSM is measured, the 'IDLE' signal is sent to loco.
 4) Once the speed of the WSM decreses to 0 and some other time is elapsed
    (the loco may still very slowly move), the measured() event is called.

When any error happens, on_error event is called.
*/

#include <QObject>
#include <QTimer>

#include "lib/wsm/wsm.h"
#include "lib/xn/xn.h"

namespace Cr {

enum class CrError {
	XnNoResponse,
	WsmNoResponse,
	SpeedMeasure,
	WsmCannotStartLt,
};

constexpr double EPSILON = 3; // +- 3 kmph
constexpr double MAX_DIFFUSION = 5; // 5 kmph
constexpr size_t MEASURE_COUNT = 30; // measuring 30 values = 3 s
constexpr unsigned ADAPT_MAX_TICKS = 4; // maximum speed adaptation ticks (~15 s)
constexpr unsigned DEFAULT_STOP_MIN =
    10; // we must measure 10 times 0 kmph to determnine that loco has stopped

class CalibRange : public QObject {
	Q_OBJECT

public:
	unsigned stop_min = DEFAULT_STOP_MIN;

	CalibRange(Xn::XpressNet &xn, Wsm::Wsm &wsm, QObject *parent = nullptr);
	void measure(unsigned loco_addr, unsigned step, Xn::Direction dir, unsigned spkmph);

private:
	Xn::XpressNet &m_xn;
	Wsm::Wsm &m_wsm;
	Xn::Direction m_dir;
	unsigned m_loco_addr;
	unsigned m_step;
	uint32_t m_start_dist;
	uint32_t m_end_dist;
	unsigned m_stop_counter;
	unsigned m_speed_err_count;
	unsigned m_expected_speed_kmph;

	void xn_speed_ok(void *, void *);
	void xn_speed_err(void *, void *);

	void loco_go();
	void loco_stop();

	void start_lt();
	void disconnect_signals();

private slots:
	void wsm_dist_read(double dist, uint32_t dist_raw);
	void wsm_speed_read(double speed, uint16_t speed_raw);
	void wsm_error();
	void wsm_lt_read(double speed, double diffusion);

signals:
	void on_error(Cr::CrError, unsigned step, const QString&);
	void measured(double distance);
};

} // namespace Cr

#endif
