#pragma once

// =================== PINS =================== //
#define LED_PIN 32
#define S1_PIN 5
#define S2_PIN 4
#define KEY_PIN 2

// =================== WI-FI =================== //
#define SSID "Kerosinovka"
#define WIFI_PASSWORD "c588tb36"
#define RECONNECT_DELAY 10000
#define UDP_PORT 1679
#define MAX_CLIENTS 5
#define SYNC_DELAY 100
#define KEEP_ALIVE_SEND 10000
#define KEEP_ALIVE_RESPOSE 1000

// =================== OTHER =================== //
#define MODE_AMOUNT 6
#define NUM_LEDS 256

// =================== FIRE =================== //
#define SMOOTH_K 0.15   // коэффициент плавности огня
#define MIN_BRIGHT 80   // мин. яркость огня
#define MAX_BRIGHT 255  // макс. яркость огня
#define MIN_SAT 255     // мин. насыщенность
#define MAX_SAT 255     // макс. насыщенность

// =================== FIRE 1D =================== //
#define MIN_BRIGHT_FIRE_1D 70  // мин. яркость огня
#define MAX_BRIGHT_FIRE_1D 255 // макс. яркость огня
#define MIN_SAT_FIRE_1D 245    // мин. насыщенность
#define MAX_SAT_FIRE_1D 255    // макс. насыщенность
#define HUE_GAP_1D_FIRE 21
#define STEP_FIRE_1D 15

// =================== STARBURST =================== //
#define STARBURST_MAX_FRAG 8
#define MAX_STARS 21

// =================== DEBUG =================== //
#define DEBUG_EN 1

#if (DEBUG_EN)
#define DBG_PRINT(x)          (Serial.print(x))
#define DBG_PRINTLN(x)        (Serial.println(x))
#define DBG_UDP(x, size)      (udpSend(x, size, IPAddress(192,168,1,3)))
#else
#define DBG_PRINT(x)
#define DBG_PRINTLN(x)
#define DBG_UDP(x, size)
#endif