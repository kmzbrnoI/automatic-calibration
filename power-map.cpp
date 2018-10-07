#include "power-map.h"

PowerToSpeedMap::PowerToSpeedMap() {
	for(size_t i = 0; i < _STEPS_CNT; i++)
		map[i] = 0;
}
