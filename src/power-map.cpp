#include "power-map.h"

namespace Pm {

PowerToSpeedMap::PowerToSpeedMap(QObject *parent) : QObject(parent) { clear(); }

void PowerToSpeedMap::clear() {
	for (auto &item : map)
		item = EMPTY_VALUE;
	map[0] = 0;
    emit onClear();
    emit onAddOrUpdate(0, 0);
}

void PowerToSpeedMap::addOrUpdate(const unsigned power, const float speed) {
	map[power] = speed;
    emit onAddOrUpdate(power, speed);
}

unsigned PowerToSpeedMap::power(const float speed) const {
	size_t last = 0;
	for (size_t i = 0; i < POWER_CNT; i++) {
		if (EMPTY_VALUE != map[i]) {
			if (map[i] == speed)
				return i;
			if (map[i] > speed) {
				float deltaSp = speed - map[last];
				float distSp = map[i] - map[last];
				return ((deltaSp / distSp) * (i - last)) + last;
			}
			last = i;
		}
	}

	throw ENoMap("No map data for this speed!");
}

bool PowerToSpeedMap::isRecord(const unsigned power) const { return (EMPTY_VALUE != map[power]); }

float const *PowerToSpeedMap::speed(const unsigned power) const {
	return map[power] != EMPTY_VALUE ? &map[power] : nullptr;
}

bool PowerToSpeedMap::isAnyRecord() const {
	for (size_t i = 1; i < POWER_CNT; i++)
		if (EMPTY_VALUE != map[i])
			return true;
	return false;
}

float const *PowerToSpeedMap::at(const int power) const { return speed(power); }

float const *PowerToSpeedMap::operator[](const int power) const { return speed(power); }

} // namespace Pm
