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

void PowerToSpeedMap::addOrUpdate(unsigned step, float speed) {
	map[step] = std::make_unique<float>(speed);
	onAddOrUpdate(step, speed);
}

unsigned PowerToSpeedMap::steps(float speed) {
	size_t last = 0;
	for(size_t i = 0; i < _STEPS_CNT; i++) {
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

bool PowerToSpeedMap::isRecord(unsigned step) {
	return (nullptr != map[step]);
}

float* PowerToSpeedMap::speed(unsigned step) {
	return map[step].get();
}

}//end namespace
