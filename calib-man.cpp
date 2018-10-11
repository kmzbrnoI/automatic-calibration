#include <vector>

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
	onStepPowerChanged(step, power);
}

///////////////////////////////////////////////////////////////////////////////

std::unique_ptr<unsigned> CalibMan::nextStep() {
	std::vector<unsigned> used_steps; // map of indexes of steps
	int last = -1;
	for(size_t i = 0; i < Xn::_STEPS_CNT; i++)
		if (nullptr != m_ssm.map[i] && static_cast<int>(*m_ssm.map[i]) != last) {
			used_steps.push_back(i);
			last = *m_ssm.map[i]; // avoid duplicate speds
		}

	if (used_steps.size() == 0) // no available steps
		return nullptr;

	// Check if min step calibred
	if (state[used_steps[0]] == StepState::Uncalibred)
		return std::make_unique<unsigned>(used_steps[0]);

	// Check if max step calibred
	if (state[used_steps[used_steps.size()-1]] == StepState::Uncalibred)
		return std::make_unique<unsigned>(used_steps[used_steps.size()-1]);

	// Binary search next step; interval=[left, right)
	return std::move(nextStepBin(used_steps, 0, used_steps.size()));
}

std::unique_ptr<unsigned> CalibMan::nextStepBin(const std::vector<unsigned>& used_steps,
                                                const size_t left, const size_t right) {
	size_t middle = ((right-left) / 2) + left;
	if (state[used_steps[middle]] == StepState::Uncalibred)
		return std::make_unique<unsigned>(middle);

	std::unique_ptr<unsigned> res;

	res = std::move(nextStepBin(used_steps, left, middle));
	if (nullptr != res)
		return res;

	res = std::move(nextStepBin(used_steps, middle, right));
	if (nullptr != res)
		return res;

	return nullptr;
}

///////////////////////////////////////////////////////////////////////////////

void CalibMan::calibrateAll(unsigned locoAddr) {
	m_locoAddr = locoAddr;
	calibrateNextStep();
}

void CalibMan::calibrateNextStep() {
	std::unique_ptr<unsigned> next = nextStep();
	if (nullptr == next) {
		// No more steps to calibrate
		onDone();
		return;
	}

	csSigConnect();
	cs.calibrate(m_locoAddr, (*next) + 1, *(m_ssm.map[*next]));
}

void CalibMan::csSigConnect() {
	QObject::connect(&cs, SIGNAL(diffusion_error()), this, SLOT(csError()));
	QObject::connect(&cs, SIGNAL(loco_stopped()), this, SLOT(csError()));
	QObject::connect(&cs, SIGNAL(xn_error()), this, SLOT(csError()));
	QObject::connect(&cs, SIGNAL(done()), this, SLOT(csDone()));
	QObject::connect(&cs, SIGNAL(step_power_changed(unsigned, unsigned)),
	                 this, SLOT(csStepPowerChanged(unsigned, unsigned)));
}

void CalibMan::csSigDisconnect() {
	QObject::disconnect(&cs, SIGNAL(diffusion_error()), this, SLOT(csError()));
	QObject::disconnect(&cs, SIGNAL(loco_stopped()), this, SLOT(csError()));
	QObject::disconnect(&cs, SIGNAL(xn_error()), this, SLOT(csError()));
	QObject::disconnect(&cs, SIGNAL(done()), this, SLOT(csDone()));
	QObject::disconnect(&cs, SIGNAL(step_power_changed(unsigned, unsigned)),
	                 this, SLOT(csStepPowerChanged(unsigned, unsigned)));
}

}//end namespace
