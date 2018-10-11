#include "calib-man.h"

namespace Cm {

CalibMan::CalibMan(Xn::XpressNet& xn, Pm::PowerToSpeedMap& pm, Wsm::Wsm& wsm,
                   Ssm::StepsToSpeedMap& ssm, QObject *parent)
	: QObject(parent), cs(xn, pm, wsm), m_ssm(ssm), m_xn(xn) {
}

///////////////////////////////////////////////////////////////////////////////
// Calibration Step events:

void CalibMan::csDone() {
}

void CalibMan::csError() {
}

void CalibMan::csStepPowerChanged(unsigned step, unsigned power) {
}

///////////////////////////////////////////////////////////////////////////////

}//end namespace
