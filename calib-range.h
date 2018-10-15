#ifndef _CALIB_RANGE_H_
#define _CALIB_RANGE_H_

/*
This unit defines a CalibRange class which allows user to measure distance
while decelerating from 'step' addr.

 1) The loco spede step is set to 'step'.
 2) After _SP_ADAPT_TIMEOUT the loco`s speed is consireder as stabilized.
    (beware of setting low acceleration!)
 3) When the distance fom WSM is measured, the 'IDLE' signal is sent to loco.
 4) Once ste speed of the WSM decreses to 0 and some other time is elapsed
    (the loco may still very slowly move), the measured() event is called.

When any error happens, on_error event is called.
*/

#include <QObject>
#include <QTimer>

#include "lib/xn/xn.h"
#include "lib/wsm/wsm.h"

namespace Cr {

enum class CrError {
	XnNoResponse,
};

const unsigned _SP_ADAPT_TIMEOUT = 2000; // 2s
const unsigned _STOP_MIN = 10; // we must measure 10 times 0 kmph to determnine that loco has stopped

class CalibRange : public QObject {
	Q_OBJECT

public:
	CalibRange(Xn::XpressNet& xn, Wsm::Wsm& wsm, QObject *parent = nullptr);
	void measure(const unsigned loco_addr, const unsigned step, Xn::XnDirection dir);

private:
	Xn::XpressNet& m_xn;
	Wsm::Wsm& m_wsm;
	Xn::XnDirection m_dir;
	unsigned m_loco_addr;
	unsigned m_step;
	QTimer t_sp_adapt;
	uint32_t m_start_dist;
	uint32_t m_end_dist;
	unsigned m_stop_counter;

	void xn_speed_ok(void*, void*);
	void xn_speed_err(void*, void*);

private slots:
	void wsm_dist_read(double dist, uint32_t dist_raw);
	void wsm_speed_read(double speed, uint16_t speed_raw);
	void wsm_error();
	void t_sp_adapt_tick();

signals:
	void on_error(Cr::CrError, unsigned step);
	void measured(double distance);
};

}//namespace Cr

#endif
