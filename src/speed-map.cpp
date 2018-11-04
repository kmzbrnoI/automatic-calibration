#include <fstream>
#include <string>
#include <sstream>

#include "speed-map.h"

namespace Ssm {

StepsToSpeedMap::StepsToSpeedMap(QObject *parent) : QObject(parent) {
	clear();
}

StepsToSpeedMap::StepsToSpeedMap(const QString& filename, QObject *parent)
	: QObject(parent) {
	clear();
	load(filename);
}

void StepsToSpeedMap::load(const QString& filename) {
	std::ifstream in(filename.toUtf8().data());
	std::string line;
	while (getline(in, line, '\n')) {
		std::istringstream templine(line);
		std::string data;

		unsigned step, speed;
		try {
			getline(templine, data, ';');
			step = std::stoul(data);
			getline(templine, data, ';');
			speed = std::stoul(data);
		}
		catch (const std::invalid_argument& e) {
			continue;
		} catch (const std::out_of_range& e) {
			continue;
		}

		if (step <= 0 || step > 28)
			continue;

		m_map[step-1] = speed;
		onAddOrUpdate(step-1, *at(step-1));
	}
}

void StepsToSpeedMap::save(const QString& filename) {
	std::ofstream out(filename.toUtf8().data());
	out << "0;0" << std::endl;
	for(size_t i = 0; i < STEPS_CNT; i++)
		if (EMPTY_VALUE != m_map[i])
			out << (i+1) << ";" << m_map[i] << std::endl;
}

void StepsToSpeedMap::clear() {
	for(auto& item : m_map)
		item = EMPTY_VALUE;
	onClear();
}

void StepsToSpeedMap::addOrUpdate(const unsigned step, const unsigned speed) {
	m_map[step] = speed;
	onAddOrUpdate(step, speed);
}

unsigned const* StepsToSpeedMap::operator[] (const int index) const {
	return at(index);
}

unsigned const* StepsToSpeedMap::at(const int index) const {
	if (EMPTY_VALUE == m_map[index])
		return nullptr;
	if (m_map[index] >= m_max_speed)
		return &m_max_speed;
	return &m_map[index];
}

unsigned StepsToSpeedMap::maxSpeed() const {
	return m_max_speed;
}

void StepsToSpeedMap::setMaxSpeed(const unsigned new_speed) {
	for(size_t i = 0; i < STEPS_CNT; i++) {
		if (EMPTY_VALUE != m_map[i]) {
			if (m_map[i] > m_max_speed && m_map[i] <= new_speed)
				onAddOrUpdate(i, m_map[i]);
			else if (m_map[i] > new_speed)
				onAddOrUpdate(i, new_speed);
		}
	}
	m_max_speed = new_speed;
}

unsigned StepsToSpeedMap::noDifferentSpeeds() const {
	unsigned last = 0, count = 0;
	for(size_t i = 0; i < STEPS_CNT; i++) {
		if (nullptr != at(i) && last != *at(i)) {
			count++;
			last = *at(i);
		}
	}
	return count;
}

unsigned StepsToSpeedMap::maxSpeedInFile() const {
	for(int i = STEPS_CNT-1; i >= 0; i--)
		if (EMPTY_VALUE != m_map[i])
			return m_map[i];
	return 0;
}

}//namespace Ssm
