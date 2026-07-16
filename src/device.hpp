#pragma once

#include <cstdint>

namespace device 
{

void init();
void update();
uint16_t getDeviceId();
uint16_t getAddress();

} // namespace device