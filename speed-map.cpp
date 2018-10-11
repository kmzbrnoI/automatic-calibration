#include <fstream>
#include <string>
#include <sstream>

#include "speed-map.h"

namespace Ssm {

StepsToSpeedMap::StepsToSpeedMap(QObject *parent) : QObject(parent) {}

StepsToSpeedMap::StepsToSpeedMap(QString filename, QObject *parent)
	: QObject(parent) {
	load(filename);
}

void StepsToSpeedMap::load(QString filename) {
	std::ifstream in(filename.toLatin1().data());
	std::string line;
	while (getline(in, line, '\n')) {
		std::istringstream templine(line);
		std::string data;
		getline(templine, data, ';');
		unsigned step = stoul(data);
		getline(templine, data, ';');
		unsigned speed = stoul(data);

		if (step <= 0 || step > 28)
			continue;

		m_map[step-1] = std::make_unique<unsigned>(speed);
		onAddOrUpdate(step-1, speed);
	}
}

void StepsToSpeedMap::save(QString filename) {
	std::ofstream out(filename.toLatin1().data());
	out << "0;0" << std::endl;
	for(size_t i = 0; i < _STEPS_CNT; i++)
		if (nullptr != m_map[i])
			out << (i+1) << ";" << *m_map[i] << std::endl;
}

void StepsToSpeedMap::clear() {
	for(auto& item : m_map)
		item = nullptr;
	onClear();
}

void StepsToSpeedMap::addOrUpdate(unsigned step, unsigned speed) {
	m_map[step] = std::make_unique<unsigned>(speed);
	onAddOrUpdate(step, speed);
}

unsigned* StepsToSpeedMap::operator[] (const int index) {
	return at(index);
}

unsigned* StepsToSpeedMap::at(const int index) {
	if (nullptr == m_map[index])
		return m_map[index].get();
	if (*m_map[index] >= m_max_speed)
		return &m_max_speed;
	return m_map[index].get();
}

unsigned StepsToSpeedMap::maxSpeed() const {
	return m_max_speed;
}

void StepsToSpeedMap::setMaxSpeed(unsigned new_speed) {
	for(size_t i = 0; i < _STEPS_CNT; i++)
		if (nullptr != m_map[i] && *m_map[i] > m_max_speed && *m_map[i] <= new_speed)
			onAddOrUpdate(i, *m_map[i]);
	for(size_t i = 0; i < _STEPS_CNT; i++)
		if (nullptr != m_map[i] && *m_map[i] > new_speed)
			onAddOrUpdate(i, new_speed);

	m_max_speed = new_speed;
}

}//end namespace
