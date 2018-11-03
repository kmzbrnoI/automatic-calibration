#include "power-map.h"

namespace Pm {

PowerToSpeedMap::PowerToSpeedMap(QObject *parent)
	: QObject(parent) {
	map[0] = std::make_unique<float>(0);
}

void PowerToSpeedMap::clear() {
	for(auto& item : map)
		item = nullptr;
	map[0] = std::make_unique<float>(0);
	onClear();
	onAddOrUpdate(0, 0);
}

void PowerToSpeedMap::addOrUpdate(const unsigned power, const float speed) {
	map[power] = std::make_unique<float>(speed);
	onAddOrUpdate(power, speed);
}

unsigned PowerToSpeedMap::power(const float speed) const {
	size_t last = 0;
	for(size_t i = 0; i < _POWER_CNT; i++) {
		if (nullptr != map[i]) {
			if (*map[i] == speed)
				return i;
			if (*map[i] > speed) {
				float deltaSp = speed - *map[last];
				float distSp = *map[i] - *map[last];
				return ((deltaSp / distSp) * (i - last)) + last;
			}
			last = i;
		}
	}

	throw ENoMap("No map data for this speed!");
}

bool PowerToSpeedMap::isRecord(const unsigned power) const {
	return (nullptr != map[power]);
}

float *PowerToSpeedMap::speed(const unsigned power) const {
	return map[power].get();
}

bool PowerToSpeedMap::isAnyRecord() const {
	for(size_t i = 1; i < _POWER_CNT; i++)
		if (nullptr != map[i])
			return true;
	return false;
}

float* PowerToSpeedMap::at(const int power) const {
	return map[power].get();
}

float* PowerToSpeedMap::operator[] (const int power) const {
	return at(power);
}

}//namespace Pm