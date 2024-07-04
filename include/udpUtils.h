#pragma once

#include <Arduino.h>
#include <WiFiUdp.h>
#include "settings.h"

extern WiFiUDP udpClient;

void udpSend(const uint8_t *buffer, size_t size, IPAddress remoteIp);
#if DEBUG_EN
void udpSend(const char *buffer, size_t size, IPAddress remoteIp);
#endif
bool isIpInArray(IPAddress address, IPAddress* array, byte size);
