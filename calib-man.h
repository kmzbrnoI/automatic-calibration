#ifndef _CALIB_M_H_
#define _CALIB_M_H_

#include <QObject>
#include <QTimer>

namespace Cm {

const double _DEFAULT_EPSILON = 0.5; // +- 0.7 kmph

class CalibStep : public QObject {
	Q_OBJECT

public:
	CalibStep(QObject *parent = nullptr);
	void calibrate(unsigned loco_addr, unsigned step,
	               double speed, double epsilon = _DEFAULT_EPSILON);

private:
	unsigned m_loco_addr;
	unsigned step;
	double m_target_speed;
	double m_epsilon;
	QTimer t_sp_adapt;

private slots:
	void wsm_lt_read(double speed, double diffusion);
	void t_sp_adapt_tick();

signals:
	void diffusion_error();
	void loco_stopped();
	void done();
};

}//end namespace

#endif
