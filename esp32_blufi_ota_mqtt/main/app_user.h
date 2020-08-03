#pragma once

#include "stdint.h"

extern uint16_t lightChannelValue;

void app_bell_contorl(uint16_t duty);

void app_relay_contorl(uint8_t channel, bool state);

void app_user_init(void);