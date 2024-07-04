#pragma once
#include <FastLED.h>
#include <Arduino.h>
#include "color.h"
#include "settings.h"

extern uint8_t numStars;
extern byte starBurstData[2];

typedef struct particle
{
    CRGB color;
    uint32_t birth;
    uint32_t lastUpdate;
    float vel;
    uint16_t pos;
    float fragment[STARBURST_MAX_FRAG];
} star;

void starburstTick(CRGB* leds);
