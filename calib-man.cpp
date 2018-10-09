#include "calib-man.h"

namespace Cm {

CalibStep::CalibStep(QObject *parent) : QObject(parent) {}

void CalibStep::calibrate(unsigned loco_addr, unsigned step, double speed, double epsilon) {
}

void CalibStep::wsm_lt_read(double speed, double diffusion) {
}

void CalibStep::t_sp_adapt_tick() {
}

}//end namespace
