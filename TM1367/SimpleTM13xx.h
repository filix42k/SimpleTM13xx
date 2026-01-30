#pragma once
#include <Arduino.h>

class SimpleTM13xx {
public:
  // clkPin, dioPin: ขาที่ต่อกับโมดูล
  SimpleTM13xx(uint8_t clkPin, uint8_t dioPin);

  void begin();
  void setBrightness(uint8_t level, bool on = true); // level 0..7
  void clear();

  // ส่ง raw segment (4 หลัก) data[0]=digit ซ้ายสุด ... data[3]=ขวาสุด
  void setSegments(const uint8_t data[4]);

  // แสดงเลข 0..9999 (หรือค่าติดลบได้ถ้า allowNegative=true)
  // leadingZero: ถ้า true เติม 0 ด้านหน้า
  void showNumber(int value, bool leadingZero = false, bool allowNegative = true);

  // แสดงเลขแบบกำหนดตำแหน่ง/ความยาว เช่น value=100, length=3, pos=1 -> _100
  void showNumberAt(int value, uint8_t length, uint8_t pos, bool leadingZero = false, bool allowNegative = true);

  // โชว์ HH:MM ถ้าโมดูลมี colon (:) จะติดได้ (ถ้าไม่มี colon ก็จะไม่เห็นผล)
  void showTime(uint8_t hour, uint8_t minute, bool colonOn = true);

private:
  uint8_t _clk, _dio;
  uint8_t _brightnessCmd; // 0x88..0x8F
  bool _displayOn;

  static const uint8_t SEG_DIGITS[10];

  void start();
  void stop();
  bool writeByte(uint8_t b);

  void dioOutput();
  void dioInput();

  void bitDelay();
  void sendCommand(uint8_t cmd);
  void setAddress(uint8_t addr);

  uint8_t encodeDigit(int d);
};
