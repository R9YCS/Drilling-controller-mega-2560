#include <Arduino.h>
#include "EEPROM.h"

class EepromCell
{
public:
  EepromCell(int32_t value, int address);
  void read();
  void write();
  void setValue(int32_t value);
  int32_t value();

private:
  int32_t m_value;
  uint32_t* m_address;
};

EepromCell::EepromCell(int32_t value, int address) {
  value = m_value;
  m_address = (uint32_t*)address;
}

void EepromCell::read() {
  m_value = eeprom_read_dword(m_address);
}

void EepromCell::write() {
  eeprom_update_dword(m_address, m_value);
}

void EepromCell::setValue(int32_t value) {
  m_value = value; 
}

int32_t EepromCell::value() {
  return m_value;
}