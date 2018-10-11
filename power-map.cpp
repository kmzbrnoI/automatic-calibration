#include "power-map.h"

namespace Pm {

PowerToSpeedMap::PowerToSpeedMap(float max_speed, QObject *parent)
	: QObject(parent), m_max_speed(max_speed) {
	map[0] = std::make_unique<float>(0);
	map[_STEPS_CNT-1] = std::make_unique<float>(max_speed);
}

void PowerToSpeedMap::clear() {
	for(auto& item : map)
		item = nullptr;
	map[0] = std::make_unique<float>(0);
	map[_STEPS_CNT-1] = std::make_unique<float>(m_max_speed);
	onClear();
	onAddOrUpdate(0, 0);
	onAddOrUpdate(_STEPS_CNT-1, m_max_speed);
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

}//end namespace
