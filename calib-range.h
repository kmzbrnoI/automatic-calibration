#ifndef _CALIB_RANGE_H_
#define _CALIB_RANGE_H_

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

}//end namespace

#endif
