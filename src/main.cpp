#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <EncButton.h>

#include "starburst.h"
#include "udpUtils.h"

byte rgbData[3] =           {0};       // RGB
byte rainbowData[2] =       {5, 1};    // Период; Шаг(не доделано)
byte fireData[2] =          {0, 10};   // Стартовый цвет; Шаг
byte fire1DData[2] =        {0, 15};   // Стартовый цвет; Шаг
byte snowData[2] =          {10, 10};  // Шанс появления; Скорость затемнения


byte curMode = 0;
byte isEnabled = 1;

void fireTick();
void fireTick1D();
inline void updateData(byte *);


uint32_t rainbow_timer, reconnect_timer = 0, sync_timer=0, keep_alive_timer=0, snowTimer=0; // Таймеры
byte cur_rainbow_clr = 0;

byte *modeDataArrayPtrs[] = {rgbData, rainbowData, fireData, fire1DData, snowData, starBurstData}; // Указатели на массивы данных режимов
byte modeDataArrayLengths[] = {3,2,2,2,2,2};  // Длины массивов данных режимов
byte dataToSend[6];


CRGB leds[NUM_LEDS];
const uint16_t active_leds = 256;

EncButton eb(S1_PIN, S2_PIN, KEY_PIN);

IPAddress clientIps[MAX_CLIENTS];         // Массив ip клиентов
IPAddress KeepAliveIpBuffer[MAX_CLIENTS];
byte KeepAliveIpBufferIndex = 0; 
byte client_amount = 0;

void snowTick() {
  if (millis() - snowTimer > 20) {
    snowTimer = millis();
    if (random8(snowData[0]) == 0) leds[random16(NUM_LEDS)] = CRGB::White; // Установка цвета на полностью белый
    fadeToBlackBy(leds, NUM_LEDS, snowData[1]); // Уменьшение яркости всех светодиодов
  }
}

void updateDataToSend() {
  dataToSend[0] = curMode;
  dataToSend[1] = isEnabled;
  dataToSend[2] = FastLED.getBrightness();
  for (byte j = 0; j < modeDataArrayLengths[curMode]; j++) 
    dataToSend[j + 3] = modeDataArrayPtrs[curMode][j];
}

inline void updateData(byte *data)
{
  for (byte j = 0; j < modeDataArrayLengths[curMode]; j++) modeDataArrayPtrs[data[0]][j] = data[j + 3];
}

inline void changeEnabledState(bool newState)
{
  isEnabled = newState;
  updateDataToSend();
  for (int i = 0; i < client_amount; i++) udpSend(dataToSend, modeDataArrayLengths[dataToSend[0]] + 3, clientIps[i]);
  if(!isEnabled) FastLED.clear();
  FastLED.show();
}

inline void changeEnabledState(bool newState, IPAddress authorIp)
{
  isEnabled = newState;
  updateDataToSend();
  for (int i = 0; i < client_amount; i++) if(clientIps[i] != authorIp) udpSend(dataToSend, modeDataArrayLengths[dataToSend[0]] + 3, clientIps[i]);
  if(!isEnabled) FastLED.clear();
  FastLED.show();
}

void setup()
{
  FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, NUM_LEDS);

  Serial.begin(115200);
  delay(10);
  WiFi.begin(SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    DBG_PRINT(".");
  }

  DBG_PRINTLN("");
  DBG_PRINTLN("WiFi connected");
  udpClient.begin(UDP_PORT);
  DBG_PRINTLN("Server started");

  if (numStars > MAX_STARS) numStars = MAX_STARS;
}

void loop()
{
  // Переподключение к WI-FI
  if ((WiFi.status() != WL_CONNECTED) && (millis() - reconnect_timer >= RECONNECT_DELAY))
  {
    DBG_PRINTLN("Reconnecting to WIFI network");
    WiFi.disconnect();
    WiFi.reconnect();
    reconnect_timer = millis();
  }

  int packetSize = udpClient.parsePacket();
  if (packetSize) 
  {                                             // Получение данных от клиента
    
    IPAddress remoteIP = udpClient.remoteIP();
    byte packetBuffer[32];
    int len = udpClient.read(packetBuffer, 32);
    if (len > 0) {
      packetBuffer[len] = 0;
    }

    if     (packetBuffer[1] & 0b00100000) 
    {                                           // Keep alive пакет
      if(isIpInArray(remoteIP, clientIps, client_amount) && !isIpInArray(remoteIP, KeepAliveIpBuffer, KeepAliveIpBufferIndex)){

      KeepAliveIpBuffer[KeepAliveIpBufferIndex++] = remoteIP;
      }

    }

    else if(packetBuffer[1] & 0b00010000)       
    {                                           // Запрос на создание канала связи
      if (!(isIpInArray(remoteIP, clientIps, client_amount) || isIpInArray(remoteIP, KeepAliveIpBuffer, KeepAliveIpBufferIndex)))
      {
      updateDataToSend();
      udpSend(dataToSend, modeDataArrayLengths[dataToSend[0]] + 3, remoteIP);
        clientIps[client_amount++] = remoteIP;
      KeepAliveIpBuffer[KeepAliveIpBufferIndex++] = remoteIP;
      DBG_PRINTLN("Запрос на создание канала связи");
      }
      else
      {
        DBG_PRINTLN("Попытка дублировать соединение");
      } 
    }

    else if(packetBuffer[1] & 0b00000100)
    {                                           // Запрос на смену режима
      curMode = packetBuffer[0];
      updateDataToSend();
      for (int i = 0; i < client_amount; i++) 
        udpSend(dataToSend, modeDataArrayLengths[dataToSend[0]] + 3, clientIps[i]);
      DBG_PRINTLN("Запрос на смену режима");
    }

    else if(packetBuffer[1] & 0b01000000)
    {                                           // Запрос на измененние яркости
      updateDataToSend();
      for (int i = 0; i < client_amount; i++) if(clientIps[i] != remoteIP) udpSend(dataToSend, modeDataArrayLengths[dataToSend[0]] + 3, clientIps[i]);
      FastLED.setBrightness(packetBuffer[2]);
      FastLED.show();
      DBG_PRINTLN("Запрос на измененние яркости");
    }

    else if(packetBuffer[1] & 0b00000010)
    {                                           // Запрос на измененние состояния вкл/выкл
      changeEnabledState(packetBuffer[1] & 0b00000001, remoteIP);
      DBG_PRINTLN("Запрос на измененние состояния вкл/выкл");
    }

    else if(packetBuffer[1] & 0b00001000)
    {                                           // Запрос текущего состояния
      updateDataToSend();
      udpSend(dataToSend, modeDataArrayLengths[dataToSend[0]] + 3, remoteIP);
      DBG_PRINTLN("Запрос текущего состояния");
    }

    else if((packetBuffer[0] != curMode) || ((packetBuffer[1] & 1) != isEnabled)) 
    {                                           // Не совпадает режим или сост. вкл/выкл
      updateDataToSend();
      udpSend(dataToSend, modeDataArrayLengths[dataToSend[0]] + 3, remoteIP);     // Отправляем данные о текущем состоянии
    }
    else
    {
      updateData(packetBuffer);                 // Применяем принятые данные

      if (millis() - sync_timer > SYNC_DELAY)
      {                                         // Синхронизация с телефоном
        sync_timer = millis();
        updateDataToSend();
        for (int i = 0; i < client_amount; i++) if (clientIps[i] != remoteIP) udpSend(dataToSend, modeDataArrayLengths[dataToSend[0]] + 3, clientIps[i]);
      }
    }
  }

  if(millis() - keep_alive_timer > KEEP_ALIVE_SEND)
  {                                             // Рассылка KEEP_ALIVE
    for(byte i = 0; i < KeepAliveIpBufferIndex; i++) 
    {
      clientIps[i] = KeepAliveIpBuffer[i];
    }
    client_amount = KeepAliveIpBufferIndex;
    KeepAliveIpBufferIndex = 0;
    static byte keep_alive_packet = 0b00100000;
    for (int i = 0; i < client_amount; i++) udpSend(&keep_alive_packet, 1U, clientIps[i]);
    keep_alive_timer = millis();
    DBG_PRINTLN(client_amount);
  }

  eb.tick();

  if (eb.turn()) 
  {
    /* 
      DBG_PRINT("turn: dir ");
      DBG_PRINT(eb.dir());
      DBG_PRINT(", fast ");
      DBG_PRINT(eb.fast());
      DBG_PRINT(", hold ");
      DBG_PRINTLN(eb.pressing());
      DBG_PRINT(", counter ");
      DBG_PRINT(eb.counter);
      DBG_PRINT(", clicks ");
      DBG_PRINTLN(eb.getClicks());
    */
    
    if (eb.pressing())
    {
      // if (eb.getClicks())
      // {
      //   modeDataArrayPtrs[curMode][constrain(eb.getClicks() - 1, 0, modeDataArrayLengths[curMode] - 1)] += eb.dir() * 10;
      // } else
        curMode = (curMode + eb.dir() + MODE_AMOUNT) % MODE_AMOUNT; // TODO: Добавить рассылку-синхронизацию
    } else 
    {
      FastLED.setBrightness(constrain(FastLED.getBrightness() + eb.dir() * 10, 0, 255));     // TODO: Добавить рассылку-синхронизацию
    }
  }

  // кнопка
  // if (eb.press()) DBG_PRINTLN("press");
  if (eb.click()) changeEnabledState(!isEnabled);

  if (isEnabled)
  {
    switch (curMode)
    {
    case 0:
      fill_solid(leds, active_leds,CRGB(rgbData[0], rgbData[1], rgbData[2]));
      FastLED.show();
      break;
    case 1:
      if (millis() - rainbow_timer > rainbowData[0])
      {
        rainbow_timer = millis();
        cur_rainbow_clr++;
      }
      fill_solid(leds, active_leds,CHSV(cur_rainbow_clr, 255, 255));
      FastLED.show();
      break;
    case 2:
      fireTick();
      break;
    case 3:
      fireTick1D();
      FastLED.show();
      break;
    case 4:
      snowTick();
      FastLED.show();
      break;
    case 5:
      starburstTick(leds);
      FastLED.show();
      break;
    }
  }
}

void fireTick()
{
  static uint32_t firePrevTime, firePrevTime2;
  static byte fireRnd = 0;
  static float fireValue = 0;

  // задаём направление движения огня
  if (millis() - firePrevTime > 100)
  {
    firePrevTime = millis();
    fireRnd = random16(10);
  }
  // двигаем пламя
  if (millis() - firePrevTime2 > 20)
  {
    firePrevTime2 = millis();
    fireValue = (float)fireValue * (1 - SMOOTH_K) + (float)fireRnd * 10 * SMOOTH_K;
    fill_solid(leds, active_leds, CHSV(
        constrain(map(fireValue, 20, 60, fireData[0], fireData[0] + fireData[1]), 0, 255), // H
        constrain(map(fireValue, 20, 60, MAX_SAT, MIN_SAT), 0, 255),                       // S
        constrain(map(fireValue, 20, 60, MIN_BRIGHT, MAX_BRIGHT), 0, 255)                  // V
    ));
    FastLED.show();
  }
}

CHSV getFireColor1d(int val) 
{
  // чем больше val, тем сильнее сдвигается цвет, падает насыщеность и растёт яркость
  return CHSV(
           fire1DData[0] + map(val, 0, 255, 0, fire1DData[1]),                          // H
           constrain(map(val, 0, 255, MAX_SAT_FIRE_1D, MIN_SAT_FIRE_1D), 0, 255),       // S
           constrain(map(val, 0, 255, MIN_BRIGHT_FIRE_1D, MAX_BRIGHT_FIRE_1D), 0, 255)  // V
         );
}
void fireTick1D() 
{
  static int counter = 0;
  static uint32_t prevTime;
  // двигаем пламя
  if (millis() - prevTime > 20) {
    prevTime = millis();
    // int thisPos = 0, lastPos = 0;
    for(int i = 0; i < NUM_LEDS; i++) {
      leds[i] = getFireColor1d(inoise8(i * STEP_FIRE_1D, counter));
    }
    counter += 20;
  }
}

