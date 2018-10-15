#include <vector>

#include "calib-man.h"

namespace Cm {

CalibMan::CalibMan(Xn::XpressNet& xn, Pm::PowerToSpeedMap& pm, Wsm::Wsm& wsm,
                   Ssm::StepsToSpeedMap& ssm, QObject *parent)
	: QObject(parent), cs(xn, pm, wsm), co(xn, pm, wsm), m_ssm(ssm), m_xn(xn) {
	reset();
}

void CalibMan::reset() {
	for(auto& s : state)
		s = StepState::Uncalibred;
	for(auto& s : power)
		s = 0;
}

bool CalibMan::inProgress() const {
	return m_progress != CalibState::Stopped;
}

CalibState CalibMan::progress() const {
	return m_progress;
}

void CalibMan::done() {
	updateProg(CalibState::Stopped, 1, 1);
	onDone();
}

void CalibMan::error(Cm::CmError e, unsigned step) {
	updateProg(CalibState::Stopped, 0, 1);
	onStepError(e, step);
}

void CalibMan::updateProg(CalibState cs, size_t progress, size_t max) {
	m_progress = cs;
	onProgressUpdate(getProgress(cs, progress, max));
}

size_t CalibMan::getProgress(CalibState cs, size_t progress, size_t max) {
	if (cs == CalibState::InitProg) // 0-10
		return 10 * progress / max;
	else if (cs == CalibState::Overview) // 10-40
		return (30 * progress / max) + 10;
	else if (cs == CalibState::Steps) // 40-90
		return (50 * progress / max) + 40;
	else if (cs == CalibState::Steps) // 90-100
		return (10 * progress / max) + 90;
	else
		return 0;
}

///////////////////////////////////////////////////////////////////////////////
// Calibration Step events:

void CalibMan::csDone(unsigned step, unsigned power) {
	onStepDone(step, power);
	step = step - 1; // convert step to step index
	state[step] = StepState::Calibred;

	for(size_t i = 0; i < Xn::_STEPS_CNT; i++) {
		if (nullptr != m_ssm[i] && i != step && *(m_ssm[i]) == *(m_ssm[step]) &&
		    state[i] == StepState::Uncalibred) {
			m_step_writing = i;
			m_step_power = power;
			m_xn.pomWriteCv(
				Xn::LocoAddr(m_locoAddr),
				Cs::_CV_START + i,
				power,
				std::make_unique<Xn::XnCb>([this](void *s, void *d) { xnStepWritten(s, d); }),
				std::make_unique<Xn::XnCb>([this](void *s, void *d) { xnStepWriteError(s, d); })
			);
			this->power[i] = m_step_power;
			onStepPowerChanged(i+1, power);
			return;
		}
	}

	calibrateNextStep();
}

void CalibMan::csError(Cs::CsError cs, unsigned step) {
	m_xn.setSpeed(Xn::LocoAddr(m_locoAddr), 0, direction);
	csSigDisconnect();

	if (cs == Cs::CsError::LargeDiffusion)
		error(CmError::LargeDiffusion, step);
	else if (cs == Cs::CsError::XnNoResponse)
		error(CmError::XnNoResponse, step);
	else if (cs == Cs::CsError::LocoStopped)
		error(CmError::LocoStopped, step);
	else if (cs == Cs::CsError::NoStep)
		error(CmError::NoStep, step);
}

void CalibMan::coError(Co::CoError co, unsigned step) {
	m_xn.setSpeed(Xn::LocoAddr(m_locoAddr), 0, direction);
	csSigDisconnect();

	if (co == Co::CoError::LargeDiffusion)
		error(CmError::LargeDiffusion, step);
	else if (co == Co::CoError::XnNoResponse)
		error(CmError::XnNoResponse, step);
}

void CalibMan::cStepPowerChanged(unsigned step, unsigned power) {
	this->power[step-1] = power;
	onStepPowerChanged(step, power);
}

void CalibMan::xnStepWritten(void*, void*) {
	state[m_step_writing] = StepState::Calibred;
	onStepDone(m_step_writing+1, m_step_power);

	for(size_t i = m_step_writing+1; i < Xn::_STEPS_CNT; i++) {
		if (nullptr != m_ssm[i] && *(m_ssm[i]) == *(m_ssm[m_step_writing]) &&
			state[i] == StepState::Uncalibred) {
			m_step_writing = i;
			m_xn.pomWriteCv(
				Xn::LocoAddr(m_locoAddr),
				Cs::_CV_START + i,
				m_step_power,
				std::make_unique<Xn::XnCb>([this](void *s, void *d) { xnStepWritten(s, d); }),
				std::make_unique<Xn::XnCb>([this](void *s, void *d) { xnStepWriteError(s, d); })
			);
			power[i] = m_step_power;
			onStepPowerChanged(i+1, m_step_power);
			return;
		}
	}

	calibrateNextStep();
}

void CalibMan::xnStepWriteError(void*, void*) {
	error(CmError::XnNoResponse, m_step_writing);
}

void CalibMan::coDone() {
	// Phase 2: move from phase "Getting basic data" to phase "Calibration"
	updateProg(CalibState::Steps, 0, 1);
	calibrateNextStep();
}

///////////////////////////////////////////////////////////////////////////////

std::unique_ptr<unsigned> CalibMan::nextStep() {
	std::vector<unsigned> used_steps; // map of indexes of steps
	int last = -1;
	for(size_t i = 0; i < Xn::_STEPS_CNT; i++)
		if (nullptr != m_ssm[i] && static_cast<int>(*m_ssm[i]) != last) {
			used_steps.push_back(i);
			last = *m_ssm[i]; // avoid duplicate speds
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
	if (right <= left)
		return nullptr;

	size_t middle = ((right-left) / 2) + left;
	if (state[used_steps[middle]] == StepState::Uncalibred)
		return std::make_unique<unsigned>(used_steps[middle]);

	std::unique_ptr<unsigned> res;

	res = std::move(nextStepBin(used_steps, left, middle));
	if (nullptr != res)
		return res;

	res = std::move(nextStepBin(used_steps, middle+1, right));
	if (nullptr != res)
		return res;

	return nullptr;
}

///////////////////////////////////////////////////////////////////////////////

void CalibMan::calibrateAll(unsigned locoAddr, Xn::XnDirection dir) {
	m_locoAddr = locoAddr;
	direction = dir;
	csSigConnect();

	// Phase 0: set to use speed table
	updateProg(CalibState::InitProg, 1, 4);
	useSpeedTable();
}

void CalibMan::stop() {
	if (m_progress == CalibState::Overview) {
		co.stop();
	} else if (m_progress == CalibState::Steps) {
		cs.stop();
	} else if (m_progress == CalibState::Interpolation) {
		m_thisIPstep = Xn::_STEPS_CNT;
	}

	csSigDisconnect();
	m_xn.setSpeed(Xn::LocoAddr(m_locoAddr), 0, direction);
	onLocoSpeedChanged(0);
	updateProg(CalibState::Stopped, 0, 1);
}

void CalibMan::calibrateNextStep() {
	std::unique_ptr<unsigned> next = nextStep();
	if (nullptr == next) {
		// No more steps to calibrate
		csSigDisconnect();
		m_xn.setSpeed(Xn::LocoAddr(m_locoAddr), 0, direction);
		onLocoSpeedChanged(0);

		// Phase 3: Interpolate the rest of the steps
		updateProg(CalibState::Interpolation, 0, 1);
		interpolateAll();

		return;
	}

	onStepStart((*next) + 1);
	m_xn.setSpeed(Xn::LocoAddr(m_locoAddr), (*next) + 1, direction);
	onLocoSpeedChanged((*next) + 1);
	cs.calibrate(m_locoAddr, (*next) + 1, *(m_ssm[*next]));
}

void CalibMan::csSigConnect() {
	QObject::connect(&cs, SIGNAL(on_error(Cs::CsError, unsigned)), this, SLOT(csError(Cs::CsError, unsigned)));
	QObject::connect(&cs, SIGNAL(done(unsigned, unsigned)), this, SLOT(csDone(unsigned, unsigned)));
	QObject::connect(&cs, SIGNAL(step_power_changed(unsigned, unsigned)),
	                 this, SLOT(cStepPowerChanged(unsigned, unsigned)));

	QObject::connect(&co, SIGNAL(on_error(Co::CoError, unsigned)), this, SLOT(coError(Co::CoError, unsigned)));
	QObject::connect(&co, SIGNAL(done()), this, SLOT(coDone()));
	QObject::connect(&co, SIGNAL(step_power_changed(unsigned, unsigned)),
	                 this, SLOT(cStepPowerChanged(unsigned, unsigned)));
}

void CalibMan::csSigDisconnect() {
	QObject::disconnect(&cs, SIGNAL(on_error(Cs::CsError, unsigned)), this, SLOT(csError(Cs::CsError, unsigned)));
	QObject::disconnect(&cs, SIGNAL(done(unsigned, unsigned)), this, SLOT(csDone(unsigned, unsigned)));
	QObject::disconnect(&cs, SIGNAL(step_power_changed(unsigned, unsigned)),
	                    this, SLOT(cStepPowerChanged(unsigned, unsigned)));

	QObject::disconnect(&co, SIGNAL(on_error(Co::CoError, unsigned)), this, SLOT(coError(Co::CoError, unsigned)));
	QObject::disconnect(&co, SIGNAL(done()), this, SLOT(coDone()));
	QObject::disconnect(&co, SIGNAL(step_power_changed(unsigned, unsigned)),
	                    this, SLOT(cStepPowerChanged(unsigned, unsigned)));
}

///////////////////////////////////////////////////////////////////////////////
// Steps interpolation

unsigned CalibMan::getIPpower(unsigned left, unsigned right, unsigned step) {
	// Get power for step 'step' interpolated from speed [left -- right]
	// Input condition: step \in [left--right]

	float deltaPower = static_cast<int>(power[right]) - static_cast<int>(power[left]);
	float distPower = right - left;
	return power[left] + ((deltaPower / distPower) * (step - left));
}

void CalibMan::interpolateAll() {
	// Find first interval to interpolate.
	m_thisIPleft = 0;
	while (m_thisIPleft < Xn::_STEPS_CNT-1 && (state[m_thisIPleft] == StepState::Uncalibred ||
	       state[m_thisIPleft+1] != StepState::Uncalibred))
		m_thisIPleft++;
	if (m_thisIPleft == Xn::_STEPS_CNT-1) {
		done();
		return;
	}
	m_thisIPstep = m_thisIPleft + 1;
	m_thisIPright = m_thisIPstep;
	while (m_thisIPright < Xn::_STEPS_CNT && state[m_thisIPright] == StepState::Uncalibred)
		m_thisIPright++;

	if (m_thisIPright == Xn::_STEPS_CNT) {
		// Cannot interpolate without right boundary
		done();
		return;
	}

	interpolateNext();
}

void CalibMan::xnIPWritten(void*, void*) {
	state[m_thisIPstep] = StepState::Calibred;

	if (m_thisIPstep < Xn::_STEPS_CNT) {
		m_thisIPstep++;
		interpolateNext();
	}
}

void CalibMan::interpolateNext() {
	// Find next number or interval to interpolate speed.
	if (m_thisIPstep == m_thisIPright) {
		// Find next interval
		while (m_thisIPstep < Xn::_STEPS_CNT-1 && state[m_thisIPstep] != StepState::Uncalibred) {
			m_thisIPleft = m_thisIPstep;
			m_thisIPstep++;
		}

		if (m_thisIPstep == Xn::_STEPS_CNT-1) {
			done();
			return;
		}

		m_thisIPright = m_thisIPstep;
		while (m_thisIPright < Xn::_STEPS_CNT && state[m_thisIPright] == StepState::Uncalibred)
			m_thisIPright++;

		if (m_thisIPright == Xn::_STEPS_CNT) {
			// Cannot interpolate without right boundary
			done();
			return;
		}
	}

	unsigned power = getIPpower(m_thisIPleft, m_thisIPright, m_thisIPstep);

	m_xn.pomWriteCv(
		Xn::LocoAddr(m_locoAddr),
		Cs::_CV_START + m_thisIPstep,
		power,
		std::make_unique<Xn::XnCb>([this](void *s, void *d) { xnIPWritten(s, d); }),
		std::make_unique<Xn::XnCb>([this](void *s, void *d) { xnIPError(s, d); })
	);
	this->power[m_thisIPstep] = power;
	onStepPowerChanged(m_thisIPstep+1, power);
}

void CalibMan::xnIPError(void*, void*) {
	error(CmError::XnNoResponse, m_thisIPstep);
}

///////////////////////////////////////////////////////////////////////////////

void CalibMan::setStepManually(unsigned step, unsigned power) {
	state[step] = StepState::SetManually;
	this->power[step] = power;

	// Set interpolated steps to Uncalibred
	int i = step+1;
	while (i < static_cast<int>(Xn::_STEPS_CNT) && nullptr == m_ssm[i] && state[i] != StepState::SetManually) {
		state[i] = StepState::Uncalibred;
		i++;
	}

	i = step-1;
	while (i >= 0 && nullptr == m_ssm[i] && state[i] != StepState::SetManually) {
		state[i] = StepState::Uncalibred;
		i--;
	}
}

void CalibMan::unsetStep(unsigned step) {
	state[step] = StepState::Uncalibred;
}

///////////////////////////////////////////////////////////////////////////////
// Speed Table

void CalibMan::useSpeedTable() {
	m_xn.pomWriteBit(
		Xn::LocoAddr(m_locoAddr),
		_CV_CONFIG,
		_CV_CONFIG_BIT_SPEED_TABLE,
		_CV_CONFIG_SPEED_TABLE_VALUE,
		std::make_unique<Xn::XnCb>([this](void *s, void *d) { xnSTWritten(s, d); }),
		std::make_unique<Xn::XnCb>([this](void *s, void *d) { xnSTError(s, d); })
	);
}

void CalibMan::xnSTWritten(void*, void*) {
	// Phase 0.2: set accel & decel to zero
	updateProg(CalibState::InitProg, 2, 4);
	adToZero();
}

void CalibMan::xnSTError(void*, void*) {
	error(CmError::XnNoResponse, 0);
}

///////////////////////////////////////////////////////////////////////////////
// Acceleration & deceleration

void CalibMan::adToZero() {
	// Set acceleration & deceleration to zero
	m_xn.pomWriteCv(
		Xn::LocoAddr(m_locoAddr), _CV_ACCEL, 0,
		std::make_unique<Xn::XnCb>([this](void *s, void *d) { accelWritten(s, d); }),
		std::make_unique<Xn::XnCb>([this](void *s, void *d) { adError(s, d); })
	);
}

void CalibMan::adError(void*, void*) {
}

void CalibMan::accelWritten(void*, void*) {
	updateProg(CalibState::InitProg, 3, 4);
	m_xn.pomWriteCv(
		Xn::LocoAddr(m_locoAddr), _CV_DECEL, 0,
		std::make_unique<Xn::XnCb>([this](void *s, void *d) { decelWritten(s, d); }),
		std::make_unique<Xn::XnCb>([this](void *s, void *d) { adError(s, d); })
	);
}

void CalibMan::decelWritten(void*, void*) {
	// Phase 1: make an overview of mapping steps to speed
	updateProg(CalibState::Overview, 0, 1);
	m_xn.setSpeed(Xn::LocoAddr(m_locoAddr), Co::_OVERVIEW_STEP, direction);
	onLocoSpeedChanged(Co::_OVERVIEW_STEP);
	co.makeOverview(m_locoAddr);
}


}//namespace Cm
