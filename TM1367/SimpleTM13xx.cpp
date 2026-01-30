#include "SimpleTM13xx.h"

// รูปแบบ segment มาตรฐาน 7-seg: 0b0GFEDCBA (DP อยู่บิต 7)
// ค่านี้ใช้ได้กับโมดูลส่วนใหญ่ (ถ้ากลับด้าน/ตัวเลขเพี้ยน ค่อยสลับ mapping ทีหลังได้)
const uint8_t SimpleTM13xx::SEG_DIGITS[10] = {
  0x3F, // 0
  0x06, // 1
  0x5B, // 2
  0x4F, // 3
  0x66, // 4
  0x6D, // 5
  0x7D, // 6
  0x07, // 7
  0x7F, // 8
  0x6F  // 9
};

SimpleTM13xx::SimpleTM13xx(uint8_t clkPin, uint8_t dioPin)
: _clk(clkPin), _dio(dioPin), _brightnessCmd(0x8F), _displayOn(true) {}

void SimpleTM13xx::begin() {
  pinMode(_clk, OUTPUT);
  pinMode(_dio, OUTPUT);
  digitalWrite(_clk, HIGH);
  digitalWrite(_dio, HIGH);
  setBrightness(7, true);
  clear();
}

void SimpleTM13xx::setBrightness(uint8_t level, bool on) {
  if (level > 7) level = 7;
  _displayOn = on;
  // 0x88 = display off + brightness 0, 0x8F = display on + brightness 7
  _brightnessCmd = (on ? 0x88 : 0x80) | level;
  sendCommand(_brightnessCmd);
}

void SimpleTM13xx::clear() {
  uint8_t z[4] = {0, 0, 0, 0};
  setSegments(z);
}

void SimpleTM13xx::setSegments(const uint8_t data[4]) {
  // 0x40: auto-increment mode
  sendCommand(0x40);

  // 0xC0: set address to 0
  start();
  writeByte(0xC0);
  for (int i = 0; i < 4; i++) writeByte(data[i]);
  stop();

  // brightness/display control
  sendCommand(_brightnessCmd);
}

uint8_t SimpleTM13xx::encodeDigit(int d) {
  if (d < 0 || d > 9) return 0x00;
  return SEG_DIGITS[d];
}

void SimpleTM13xx::showNumber(int value, bool leadingZero, bool allowNegative) {
  showNumberAt(value, 4, 0, leadingZero, allowNegative);
}

void SimpleTM13xx::showNumberAt(int value, uint8_t length, uint8_t pos, bool leadingZero, bool allowNegative) {
  if (length > 4) length = 4;
  if (pos > 3) pos = 3;
  if (pos + length > 4) length = 4 - pos;

  uint8_t seg[4] = {0,0,0,0};

  bool neg = false;
  long v = value;

  if (v < 0) {
    if (allowNegative) { neg = true; v = -v; }
    else v = 0;
  }

  // เติมช่องว่างก่อน
  for (int i=0; i<4; i++) seg[i] = 0x00;

  // ใส่เลขจากขวาไปซ้ายในช่วง [pos .. pos+length-1]
  for (int i = pos + length - 1; i >= (int)pos; i--) {
    int digit = (int)(v % 10);
    v /= 10;

    if (!leadingZero && v == 0 && digit == 0 && i != (int)(pos + length - 1)) {
      // ถ้ายังไม่ถึงหลักสุดท้าย และไม่ต้องการ leading zero -> เป็นช่องว่าง
      seg[i] = 0x00;
    } else {
      seg[i] = encodeDigit(digit);
    }
  }

  // ถ้าเลขมากกว่าความยาวที่ให้ -> แสดง ---- เป็นสัญญาณล้น
  if (v > 0) {
    for (int i = pos; i < (int)(pos + length); i++) seg[i] = 0x40; // '-'
  }

  // แสดงเครื่องหมายลบ ถ้าต้องการและยังมีพื้นที่
  if (neg) {
    // วาง '-' ไว้ที่ digit ซ้ายสุดของช่วง
    seg[pos] = 0x40;
  }

  setSegments(seg);
}

void SimpleTM13xx::showTime(uint8_t hour, uint8_t minute, bool colonOn) {
  // แสดง HHMM และใช้ DP บิต 7 ของ digit ที่ 2 เป็น colon ในบางบอร์ด
  // หมายเหตุ: บางโมดูล colon ถูกผูกไว้กับบิต DP ของ digit ที่ 1 หรือ 2
  // ที่นี่ลองใช้ digit[1] เป็น colon (ปรับได้ถ้าไม่ตรง)
  uint8_t seg[4];

  seg[0] = encodeDigit(hour / 10);
  seg[1] = encodeDigit(hour % 10);
  seg[2] = encodeDigit(minute / 10);
  seg[3] = encodeDigit(minute % 10);

  if (colonOn) {
    seg[1] |= 0x80; // เปิด DP เป็นตัวแทน colon (ถ้าโมดูลรองรับ)
  }

  setSegments(seg);
}

// ---------- Low-level protocol ----------

void SimpleTM13xx::bitDelay() {
  // ปรับได้ ถ้าโมดูลเรื่องมากเพิ่มเป็น 5-10 us
  delayMicroseconds(3);
}

void SimpleTM13xx::dioOutput() {
  pinMode(_dio, OUTPUT);
}

void SimpleTM13xx::dioInput() {
  pinMode(_dio, INPUT_PULLUP);
}

void SimpleTM13xx::start() {
  dioOutput();
  digitalWrite(_dio, HIGH);
  digitalWrite(_clk, HIGH);
  bitDelay();
  digitalWrite(_dio, LOW);
  bitDelay();
  digitalWrite(_clk, LOW);
  bitDelay();
}

void SimpleTM13xx::stop() {
  dioOutput();
  digitalWrite(_clk, LOW);
  digitalWrite(_dio, LOW);
  bitDelay();
  digitalWrite(_clk, HIGH);
  bitDelay();
  digitalWrite(_dio, HIGH);
  bitDelay();
}

bool SimpleTM13xx::writeByte(uint8_t b) {
  // LSB first
  for (int i = 0; i < 8; i++) {
    digitalWrite(_clk, LOW);
    bitDelay();
    digitalWrite(_dio, (b & 0x01) ? HIGH : LOW);
    bitDelay();
    digitalWrite(_clk, HIGH);
    bitDelay();
    b >>= 1;
  }

  // ACK bit
  digitalWrite(_clk, LOW);
  dioInput();
  bitDelay();

  digitalWrite(_clk, HIGH);
  bitDelay();
  bool ack = (digitalRead(_dio) == LOW); // ACK = 0
  digitalWrite(_clk, LOW);
  bitDelay();

  dioOutput();
  return ack;
}

void SimpleTM13xx::sendCommand(uint8_t cmd) {
  start();
  writeByte(cmd);
  stop();
}

void SimpleTM13xx::setAddress(uint8_t addr) {
  sendCommand(0xC0 | (addr & 0x03));
}
