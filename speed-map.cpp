#include <fstream>
#include <string>
#include <sstream>

#include "speed-map.h"

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

		map[step] = std::make_unique<unsigned>(speed);
		onAddOrUpdate(step, speed);
	}
}

void StepsToSpeedMap::save(QString filename) {
	std::ofstream out(filename.toLatin1().data());
	out << "0;0" << std::endl;
	for(size_t i = 0; i < _STEPS_CNT; i++)
		if (nullptr != map[i])
			out << (i+1) << ";" << *map[i] << std::endl;
}

void StepsToSpeedMap::clear() {
	for(auto& item : map)
		item = nullptr;
	onClear();
}

void StepsToSpeedMap::addOrUpdate(unsigned step, unsigned speed) {
	map[step] = std::make_unique<unsigned>(speed);
	onAddOrUpdate(step, speed);
}
